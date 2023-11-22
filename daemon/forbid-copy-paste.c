//
// Created by dingjing on 23-11-22.
//

#include "forbid-copy-paste.h"

#include <glib.h>
#include <stdbool.h>

#include "common-x.h"


static bool             gIsRunning = false;
static GThread*         gThread = NULL;
G_LOCK_DEFINE_STATIC(gLocker);

static GSList*          gSList = NULL;
G_LOCK_DEFINE_STATIC(gSListLocker);


static gpointer listen_event (gpointer udata);
static char* get_focused_progress(Display* dsp);

void forbid_copy_paste_start()
{
    if (G_TRYLOCK(gLocker)) {
        if (!gThread) {
            gIsRunning = true;
            gThread = g_thread_new ("forbid-copy-paste", listen_event, NULL);
        }
        G_UNLOCK(gLocker);
    }
}

void forbid_copy_paste_stop()
{
    while (true) {
        if (G_TRYLOCK(gLocker)) {
            gIsRunning = false;
            G_UNLOCK(gLocker);
            break;
        }
    }
}

void forbid_copy_paste_add_process(const char *programName)
{
    G_LOCK(gSListLocker);
    if (NULL == g_slist_find_custom (gSList, programName, (GCompareFunc) g_strcmp0)) {
        char* str = g_strdup(programName);
        gSList = g_slist_insert_sorted (gSList, str, (GCompareFunc) g_strcmp0);
    }
    G_UNLOCK(gSListLocker);
}

void forbid_copy_paste_del_process(const char *programName)
{
    G_LOCK(gSListLocker);
    GSList* node = g_slist_find_custom (gSList, programName, (GCompareFunc) g_strcmp0);
    if (NULL != node) {
        gSList = g_slist_remove_link (gSList, node);
        g_slist_free_full(node, g_free);
    }
    G_UNLOCK(gSListLocker);
}

static gpointer listen_event (gpointer udata)
{
    Window win = None;
    GC ctx = NULL;
    Display* dsp = XOpenDisplay (NULL);
    if (NULL == dsp) {
        goto out;
    }

    win = XDefaultRootWindow (dsp);
    if (0 == XSelectInput (dsp, win, SubstructureNotifyMask)) {
        g_print ("XSelectInput error\n");
        goto out;
    }

    ctx = XCreateGC (dsp, win, 0, NULL);
    if (NULL == ctx) {
        g_print ("XCreateGC error\n");
        goto out;
    }

    while (true) {
        G_LOCK(gLocker);
        bool running = gIsRunning;
        G_UNLOCK(gLocker);
        if (!running) {
            break;
        }

        XEvent ev;
        XNextEvent (dsp, &ev);
        switch (ev.type) {
            default:
            case UnmapNotify:
            case DestroyNotify:
            case ReparentNotify:
            case SelectionNotify:
            case ConfigureNotify:
            case SelectionRequest: {
                break;
            }
            case KeyPress:
            case MapNotify:
            case KeyRelease:
            case ButtonPress:
            case CreateNotify:
            case ClientMessage: {
                g_autofree char* progressName = get_focused_progress (dsp);
                if (!progressName) { continue; }
                G_LOCK(gSListLocker);
                for (GSList* l = gSList; l; l = l->next) {
                    const char* k = l->data;
                    if (strstr (progressName, k)) {
                        // clipboard
                        g_autofree char* cStr = get_clipboard_string (dsp);
                        if (cStr) {
                            if (g_str_has_prefix(cStr, "/") || g_str_has_prefix(cStr, "file:///")) {
                                g_print ("clear file: %s\n", cStr);
                                clear_clipboard (dsp);
                            }
                        }
                        break;
                    }
                }
                G_UNLOCK(gSListLocker);
                break;
            }
        }
    }

out:
    if (ctx) {
        XFreeGC (dsp, ctx);
    }

    if (dsp) {
        XCloseDisplay (dsp);
    }
    g_print ("end!\n");
}

static char* get_focused_progress(Display* dsp)
{
    if (!dsp) {
        return NULL;
    }
    Window win;
    int revert;

    XGetInputFocus (dsp, &win, &revert);
    if (BadWindow == win) {
        return NULL;
    }

    pid_t pidT = get_pid_by_wid (dsp, win);
    if (0 >= pidT) {
        return NULL;
    }

    return get_program_path_by_pid (pidT);
}
