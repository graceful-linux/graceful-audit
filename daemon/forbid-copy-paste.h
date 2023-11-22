//
// Created by dingjing on 23-11-22.
//

#ifndef GRACEFUL_AUDIT_FORBID_COPY_PASTE_H
#define GRACEFUL_AUDIT_FORBID_COPY_PASTE_H

void forbid_copy_paste_add_process(const char* programName);
void forbid_copy_paste_del_process(const char* programName);
void forbid_copy_paste_start();
void forbid_copy_paste_stop();

#endif //GRACEFUL_AUDIT_FORBID_COPY_PASTE_H
