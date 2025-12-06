/*
 * LocalSend Lite - Device Selection Dialog
 * Device selection dialog implementation for LocalSend
 */

#include "main.h"
#include "localsend_dialog.h"
#include "message_dialog.h"
#include "language.h"

#define DEVICE_LIST_MAX 10

static int localsend_dialog_status = LOCALSEND_DIALOG_CLOSED;
static int localsend_dialog_result = LOCALSEND_DIALOG_RESULT_NONE;
static LocalSendDevice localsend_devices[DEVICE_LIST_MAX];
static int localsend_device_count = 0;
static int localsend_dialog_sel = 0;

static float localsend_dialog_x = 0.0f;
static float localsend_dialog_y = 0.0f;
static float localsend_dialog_width = 0.0f;
static float localsend_dialog_height = 0.0f;
static float localsend_dialog_scale = 0.0f;

int getLocalSendDialogStatus() {
    return localsend_dialog_status;
}

void setLocalSendDialogStatus(int status) {
    localsend_dialog_status = status;
}

int getLocalSendDialogResult() {
    return localsend_dialog_result;
}

void setLocalSendDialogResult(int result) {
    localsend_dialog_result = result;
}

void initLocalSendDialog(LocalSendDevice* devices, int device_count) {
    if (device_count > DEVICE_LIST_MAX) {
        device_count = DEVICE_LIST_MAX;
    }

    memcpy(localsend_devices, devices, sizeof(LocalSendDevice) * device_count);
    localsend_device_count = device_count;
    localsend_dialog_sel = 0;

    // Calculate dialog dimensions
    localsend_dialog_width = MAX_WIDTH;
    localsend_dialog_height = (device_count + 2) * FONT_Y_SPACE; // +2 for title and padding
    localsend_dialog_x = SCREEN_HALF_WIDTH - localsend_dialog_width / 2.0f;
    localsend_dialog_y = SCREEN_HALF_HEIGHT - localsend_dialog_height / 2.0f;

    localsend_dialog_scale = 0.0f;
    setLocalSendDialogStatus(LOCALSEND_DIALOG_OPENING);
    setLocalSendDialogResult(LOCALSEND_DIALOG_RESULT_NONE);
}

void finishLocalSendDialog() {
    setLocalSendDialogStatus(LOCALSEND_DIALOG_CLOSED);
    setLocalSendDialogResult(LOCALSEND_DIALOG_RESULT_NONE);
}

void localSendDialogCtrl() {
    if (localsend_dialog_status == LOCALSEND_DIALOG_OPENING) {
        localsend_dialog_scale += 0.1f;
        if (localsend_dialog_scale >= 1.0f) {
            localsend_dialog_scale = 1.0f;
            localsend_dialog_status = LOCALSEND_DIALOG_OPENED;
        }
    } else if (localsend_dialog_status == LOCALSEND_DIALOG_CLOSING) {
        localsend_dialog_scale -= 0.1f;
        if (localsend_dialog_scale <= 0.0f) {
            localsend_dialog_scale = 0.0f;
            localsend_dialog_status = LOCALSEND_DIALOG_CLOSED;
        }
    }

    if (localsend_dialog_status == LOCALSEND_DIALOG_OPENED) {
        // Handle input
        if (pressed_pad[PAD_UP]) {
            if (localsend_dialog_sel > 0) {
                localsend_dialog_sel--;
            }
        } else if (pressed_pad[PAD_DOWN]) {
            if (localsend_dialog_sel < localsend_device_count - 1) {
                localsend_dialog_sel++;
            }
        } else if (pressed_pad[SCE_CTRL_ENTER]) {
            // Device selected
            setLocalSendDialogResult(LOCALSEND_DIALOG_RESULT_SELECTED);
            localsend_dialog_status = LOCALSEND_DIALOG_CLOSING;
        } else if (pressed_pad[SCE_CTRL_CANCEL]) {
            // Cancelled
            setLocalSendDialogResult(LOCALSEND_DIALOG_RESULT_CANCELLED);
            localsend_dialog_status = LOCALSEND_DIALOG_CLOSING;
        }
    }
}

void drawLocalSendDialog() {
    if (localsend_dialog_status == LOCALSEND_DIALOG_CLOSED)
        return;

    float scale = localsend_dialog_scale;
    if (localsend_dialog_status == LOCALSEND_DIALOG_CLOSING)
        scale = 1.0f - localsend_dialog_scale;

    float x = localsend_dialog_x + (1.0f - scale) * localsend_dialog_width / 2.0f;
    float y = localsend_dialog_y + (1.0f - scale) * localsend_dialog_height / 2.0f;
    float width = localsend_dialog_width * scale;
    float height = localsend_dialog_height * scale;

    // Draw background
    vita2d_draw_rectangle(x, y, width, height, DIALOG_BG_COLOR);

    // Draw title
    pgf_draw_textf(x + 10.0f, y + 10.0f, TITLE_COLOR, "Select LocalSend Device");

    // Draw device list
    for (int i = 0; i < localsend_device_count; i++) {
        float device_y = y + 30.0f + i * FONT_Y_SPACE;
        uint32_t color = (i == localsend_dialog_sel) ? SELECTED_COLOR : TEXT_COLOR;

        char device_info[128];
        snprintf(device_info, sizeof(device_info), "%s (%s)",
                 localsend_devices[i].alias, localsend_devices[i].ip);

        pgf_draw_text(x + 20.0f, device_y, color, device_info);
    }
}