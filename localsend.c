/*
 * LocalSend Lite for VitaShell
 * Lightweight LocalSend client implementation for PS Vita
 */

#include "localsend.h"
#include "main.h"
#include "utils.h"
#include "file.h"
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/http.h>
#include <psp2/sysmodule.h>
#include <string.h>
#include <stdio.h>

// Global variables
static int http_initialized = 0;
static int tmpl_id = -1;
char device_token[64] = {0};

// Helper function to generate a simple token
static void generate_token(char* token, int size) {
    sceKernelGetProcessTimeLow();
    snprintf(token, size, "vita_%08x", sceKernelGetProcessTimeLow());
}

// Initialize LocalSend module
int localsend_init() {
    if (http_initialized) {
        return 0;
    }

    // Load HTTP module
    int ret = sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP);
    if (ret < 0) {
        return ret;
    }

    // Initialize HTTP
    ret = sceHttpInit(0x20000);
    if (ret < 0) {
        sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
        return ret;
    }

    // Create HTTP template
    tmpl_id = sceHttpCreateTemplate("VitaShell-LocalSend/1.0", SCE_HTTP_VERSION_1_1, SCE_FALSE);
    if (tmpl_id < 0) {
        sceHttpTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
        return tmpl_id;
    }

    http_initialized = 1;
    generate_token(device_token, sizeof(device_token));

    return 0;
}

// Discover devices using HTTP scanning (simplified approach)
int localsend_discover_devices(LocalSendDevice* devices, int max_devices) {
    // Use the efficient discovery function
    return localsend_discover_devices_efficient(devices, max_devices);
}

// Register this device with LocalSend network
int localsend_register_device(const char* alias, const char* device_model, int device_type) {
    if (!http_initialized) {
        return -1;
    }

    // For PS Vita, we'll just store the info locally
    // The actual registration happens when other devices discover us
    return 0;
}

// Cleanup LocalSend module
void localsend_cleanup() {
    if (http_initialized) {
        if (tmpl_id >= 0) {
            sceHttpDeleteTemplate(tmpl_id);
            tmpl_id = -1;
        }
        sceHttpTerm();
        sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTP);
        http_initialized = 0;
    }
}