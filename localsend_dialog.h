/*
 * LocalSend Lite - Device Selection Dialog
 * Device selection dialog for LocalSend
 */

#ifndef __LOCALSEND_DIALOG_H__
#define __LOCALSEND_DIALOG_H__

#include "localsend.h"

#define LOCALSEND_DIALOG_RESULT_NONE 0
#define LOCALSEND_DIALOG_RESULT_SELECTED 1
#define LOCALSEND_DIALOG_RESULT_CANCELLED 2

enum LocalSendDialogStatus {
    LOCALSEND_DIALOG_CLOSED,
    LOCALSEND_DIALOG_CLOSING,
    LOCALSEND_DIALOG_OPENED,
    LOCALSEND_DIALOG_OPENING,
};

extern int localsend_dialog_sel;

// Function prototypes
int getLocalSendDialogStatus();
void setLocalSendDialogStatus(int status);
int getLocalSendDialogResult();
void setLocalSendDialogResult(int result);
void initLocalSendDialog(LocalSendDevice* devices, int device_count);
void finishLocalSendDialog();
void localSendDialogCtrl();
void drawLocalSendDialog();

#endif // __LOCALSEND_DIALOG_H__