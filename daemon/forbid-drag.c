//
// Created by dingjing on 23-11-22.
//

#include "forbid-drag.h"

#include <glib.h>
#include <stdbool.h>

#include "common-x.h"


static GSList*  gSList = NULL;
static bool     gIsRunning = true;


static gboolean forbid_drag (gpointer udata);
static void forbid_drag_by_window(Display* dsp, Window win);
static void forbid_drag_recursion(Display* dsp, Window win);

void forbid_drag_add_process(const char* programName)
{
    if (NULL == g_slist_find_custom (gSList, programName, (GCompareFunc) g_strcmp0)) {
        char* str = g_strdup(programName);
        gSList = g_slist_insert_sorted (gSList, str, (GCompareFunc) g_strcmp0);
    }
}

void forbid_drag_del_process(const char *programName)
{
    GSList* node = g_slist_find_custom (gSList, programName, (GCompareFunc) g_strcmp0);
    if (NULL != node) {
        gSList = g_slist_remove_link (gSList, node);
        g_slist_free_full(node, g_free);
    }
}

void forbid_drag_start()
{
    gIsRunning = true;
    g_timeout_add(5000, forbid_drag, NULL);
}

void forbid_drag_stop()
{
    gIsRunning = false;
}

static gboolean forbid_drag (gpointer udata)
{
    Display* dsp = XOpenDisplay (NULL);
    Window win = DefaultRootWindow(dsp);

    forbid_drag_recursion (dsp, win);

    return (gIsRunning ? true : false);
}

static void forbid_drag_recursion(Display* dsp, Window win)
{
    Window rootRet;
    Window parentRet;
    Window* allWindow;
    unsigned int allWindowNum = 0;

    forbid_drag_by_window (dsp, win);

    do {
        XQueryTree (dsp, win, &rootRet, &parentRet, &allWindow, &allWindowNum);
        if (0 == allWindowNum || NULL == allWindow) {
            break;
        }
        for (int i = 0; i < allWindowNum; ++i) {
            Window child = allWindow[i];
            forbid_drag_recursion (dsp, child);
        }
    } while (0);

    if (allWindow) {
        XFree (allWindow);
    }
}

static void forbid_drag_by_window(Display* dsp, Window win)
{
    Atom xdndAtom = XInternAtom (dsp, "XdndAware", true);
    if (None != xdndAtom) {
        Atom type;
        int format = 0;
        unsigned long nItems;
        unsigned long bytesAfter;
        unsigned char* propData= NULL;
        Atom pidAtom = XInternAtom (dsp, "_NET_WM_PID", false);
        int status = XGetWindowProperty (dsp, win, pidAtom, 0, 1, false, XA_CARDINAL, &type, &format, &nItems, &bytesAfter, &propData);
        if (Success == status && NULL != propData) {
            if (32 == format && XA_CARDINAL == type && 1 == nItems) {
                pid_t pidT = (pid_t)*((unsigned long*)propData);
                XFree (propData);
                g_autofree char* program = get_program_path_by_pid (pidT);
                if (program) {
//                    g_print ("wid: %9d, pid: %9d, program: %s\n", win, pidT, program);
                    for (GSList* ls = gSList; NULL != ls; ls = ls->next) {
                        const char* w = (const char*) (ls->data);
                        if (strstr (program, w)) {
                            XDeleteProperty (dsp, win, xdndAtom);
                        }
                    }
                }
            }
        }
    }
}

