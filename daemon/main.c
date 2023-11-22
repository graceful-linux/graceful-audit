//
// Created by dingjing on 23-11-22.
//
#include <glib.h>
#include <stdbool.h>

#include "forbid-drag.h"
#include "forbid-copy-paste.h"


int main (int argc, char* argv[])
{
    GMainLoop* main = g_main_loop_new (NULL, false);

    forbid_drag_add_process ("/qq");
    forbid_drag_add_process ("/kim");
    forbid_drag_add_process ("/zhixin");
    forbid_drag_add_process ("/weixin");
    forbid_drag_add_process ("/browser");
    forbid_drag_add_process ("/firefox");
    forbid_drag_add_process ("/qaxbrowser");
    forbid_drag_add_process ("/uosbrowser");
    forbid_drag_add_process ("/firefox-esr");
    forbid_drag_add_process ("/baidunetdisk");
    forbid_drag_add_process ("/qaxbrowser-safe");
    forbid_drag_add_process ("/qaxbrowser-sandbox");
    forbid_drag_start();

    forbid_copy_paste_add_process ("/qq");
    forbid_copy_paste_add_process ("/kim");
    forbid_copy_paste_add_process ("/zhixin");
    forbid_copy_paste_add_process ("/weixin");
    forbid_copy_paste_add_process ("/browser");
    forbid_copy_paste_add_process ("/firefox");
    forbid_copy_paste_add_process ("/qaxbrowser");
    forbid_copy_paste_add_process ("/uosbrowser");
    forbid_copy_paste_add_process ("/firefox-esr");
    forbid_copy_paste_add_process ("/baidunetdisk");
    forbid_copy_paste_add_process ("/qaxbrowser-safe");
    forbid_copy_paste_add_process ("/qaxbrowser-sandbox");
    forbid_copy_paste_start();

    g_main_loop_run (main);

    return 0;
}