//
// Created by dingjing on 23-11-22.
//

#ifndef GRACEFUL_AUDIT_COMMON_X_H
#define GRACEFUL_AUDIT_COMMON_X_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

char* get_program_path_by_pid(pid_t pid);
char* get_clipboard_string(Display* dsp);
pid_t get_pid_by_wid(Display* dsp, Window win);

void clear_clipboard(Display* dsp);

#endif //GRACEFUL_AUDIT_COMMON_X_H
