//
// Created by dingjing on 23-11-22.
//

#include "common-x.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define XCLIB_XCOUT_NONE                0   // no context
#define XCLIB_XCOUT_SENTCONVSEL         1   // sent a request
#define XCLIB_XCOUT_BAD_TARGET          2   // given target failed


static size_t mach_item_size(int format);
static char* clipboard_string(Display* dsp, Atom selType, unsigned char* selBuf, size_t selLen);
static int xcout (Display* dsp, Window win, XEvent event, Atom sel, Atom target, Atom* type, unsigned char** txt, unsigned long* len, unsigned int* ctx);


char* get_program_path_by_pid(pid_t pid)
{
    g_autofree char* cmdPath = g_strdup_printf ("/proc/%d/exe", pid);

    // @note need free
    return g_file_read_link (cmdPath, NULL);
}

pid_t get_pid_by_wid(Display* dsp, Window win)
{
    g_return_val_if_fail(dsp && (BadWindow != win), 0);

    Atom type;
    pid_t pidT = 0;
    int format = 0;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char* propData= NULL;

    Atom pidAtom = XInternAtom (dsp, "_NET_WM_PID", false);
    g_return_val_if_fail(None != pidAtom, 0);
    XGetWindowProperty (dsp, win, pidAtom, 0, 1, false, XA_CARDINAL, &type, &format, &nItems, &bytesAfter, &propData);
    if (NULL != propData) {
        if (32 == format && XA_CARDINAL == type && 1 == nItems) {
            pidT = (pid_t)*((unsigned long*)propData);
            XFree (propData);
        }
    }

    return pidT;
}

char *get_clipboard_string(Display *dsp)
{
    char* clipboardStr = NULL;

    do {
        Window win = XCreateSimpleWindow (dsp, DefaultRootWindow(dsp), 0, 0, 1, 1, 0, 0, 0);

        XSelectInput(dsp, win, PropertyChangeMask);

        Atom selType = None;
        unsigned char* selBuf = NULL;
        unsigned long selLen = 0;
        XEvent event;
        unsigned int context = XCLIB_XCOUT_NONE;

        Atom target = XA_UTF8_STRING(dsp);
        Atom clipboard = XInternAtom (dsp, "CLIPBOARD", False);

        while (1) {
            if (context == XCLIB_XCOUT_BAD_TARGET) { break; }
            if (context != XCLIB_XCOUT_NONE) { XNextEvent(dsp, &event); }

            xcout (dsp, win, event, clipboard, target, &selType, &selBuf, &selLen, &context);
            if (XCLIB_XCOUT_BAD_TARGET == context) {
                if (XA_UTF8_STRING(dsp) == target) {
                    context = XCLIB_XCOUT_NONE;
                    target = XA_STRING;
                    continue;
                }
                else {
                    XSetSelectionOwner (dsp, selLen, None, CurrentTime);
                    memset (selBuf, 0, selLen);
                    free (selBuf);
                }
            }
            if (XCLIB_XCOUT_NONE == context) { break; }
        }

        if (selLen > 0) {
            clipboardStr = clipboard_string (dsp, selType, selBuf, selLen);
            free (selBuf);
        }
    } while (false);

    return clipboardStr;
}

void clear_clipboard(Display *dsp)
{
    int x11Fd = ConnectionNumber(dsp);
    Window win = XCreateSimpleWindow (dsp, DefaultRootWindow(dsp), 0, 0, 1, 1, 0, 0, 0);
    Atom clip = XA_CLIPBOARD(dsp);
    Atom atom = XInternAtom (dsp, "XCLIP_OUT", false);
    XConvertSelection (dsp, clip, XA_STRING, atom, win, CurrentTime);
    XSetSelectionOwner (dsp, clip, None, CurrentTime);
    XFlush (dsp);
}

static size_t mach_item_size(int format)
{
    if (format == 8) {
        return sizeof(char);
    }
    if (format == 16) {
        return sizeof(short);
    }
    if (format == 32) {
        return sizeof(long);
    }
    return 0;
}

static int xcout (Display* dsp, Window win, XEvent event, Atom sel, Atom target, Atom* type, unsigned char** txt, unsigned long* len, unsigned int* ctx)
{
    static Atom pty;
    static Atom inc;

    int ptyFormat;

    // 将 XGetWindowProperty 的数据 dump 到下边 buffer 中
    unsigned char* buffer;
    unsigned long ptySize;
    unsigned long ptyItems;
    unsigned long ptyMachsize;

    unsigned char* lTxt = *txt;

    if (!pty) {
        pty = XInternAtom (dsp, "XCLIP_OUT", False);
    }

    if (!inc) {
        inc = XInternAtom (dsp, "INCR", False);
    }

    switch (*ctx) {
        case XCLIB_XCOUT_NONE: {
            if (*len > 0) {
                free (*txt);
                *len = 0;
            }
            XConvertSelection (dsp, sel, target, pty, win, CurrentTime);
            *ctx = XCLIB_XCOUT_SENTCONVSEL;
            return 0;
        }
        case XCLIB_XCOUT_SENTCONVSEL: {
            if (event.type != SelectionNotify) {
                return 0;
            }
            if (event.xselection.property == None) {
                *ctx = XCLIB_XCOUT_BAD_TARGET;
                return 0;
            }
            XGetWindowProperty (dsp, win, pty, 0, 0, False, AnyPropertyType, type, &ptyFormat, &ptyItems, &ptySize, &buffer);
            XFree (buffer);
            if (*type == inc) {
                XDeleteProperty(dsp, win, pty);
                XFlush(dsp);
                return 1;
            }
            XGetWindowProperty(dsp, win, pty, 0, (long) ptySize, False, AnyPropertyType, type, &ptyFormat, &ptyItems, &ptySize, &buffer);
            XDeleteProperty(dsp, win, pty);
            ptyMachsize = ptyItems * mach_item_size(ptyFormat);

            // FIXME://
            lTxt = (unsigned char*) malloc (ptyMachsize);
            if (!lTxt) {
                return 1;
            }
            memset (lTxt, 0, ptyMachsize);
            memcpy (lTxt, buffer, ptyMachsize);

            *len = ptyMachsize;
            *txt = lTxt;
            XFree (buffer);
            *ctx = XCLIB_XCOUT_NONE;
            return 1;
        }
    }

    return 0;
}

static char* clipboard_string(Display* dsp, Atom selType, unsigned char* selBuf, size_t selLen)
{
    if (selType == XA_INTEGER) {
        int len = 0;
        long* longBuf = (long*) selBuf;
        size_t longLen = selLen / sizeof (long);
        while (longLen--) {
            char bufTmp[32] = {0};
            snprintf(bufTmp, sizeof(bufTmp)/sizeof(char), "%ld", *longBuf++);
            len += strlen(bufTmp) + 1;
        }

        if (len <= 0) return NULL;
        char* buf = (char*) malloc (len);
        if (!buf) return NULL;

        int curLen = 0;
        while (longLen--) {
            char bufTmp[32] = {0};
            snprintf(bufTmp, sizeof(bufTmp)/sizeof(char), "%ld", *longBuf++);
            int aLen = strlen(bufTmp);
            strncpy (buf + curLen, bufTmp, aLen);
            strncpy (buf + curLen + aLen + 1, "\t", 1);
            curLen += (aLen + 1);
        }
        buf[curLen - 1] = '\0';

        return buf;
    }

    if (selType == XA_ATOM) {
        int len = 0;
        Atom* atomBuf = (Atom*) selBuf;
        size_t atomLen = selLen / sizeof(Atom);
        while (atomLen--) {
            char* atomName = XGetAtomName (dsp, *atomBuf++);
            len += strlen(atomName) + 1;
            XFree(atomName);
        }
        if (len <= 0) return NULL;
        char* buf = (char*) malloc (len);
        if (!buf) return NULL;

        int curLen = 0;
        while (atomLen--) {
            char* atomName = XGetAtomName (dsp, *atomBuf++);
            int aLen = strlen(atomName);
            strncpy (buf + curLen, atomName, aLen);
            strncpy (buf + curLen + aLen + 1, "\t", 1);
            curLen += (aLen + 1);
            XFree(atomName);
        }
        buf[curLen - 1] = '\0';

        return buf;
    }

    int len = sizeof (char) * (selLen + 1);
    char* buf = (char*) malloc (len);
    if (!len) {
        return NULL;
    }
    memset (buf, 0, len);
    strncpy (buf, (const char*) selBuf, (unsigned long) selLen);

    return buf;
}