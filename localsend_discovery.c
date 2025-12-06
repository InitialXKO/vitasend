/*
 * LocalSend Lite - Device Discovery
 * Efficient device discovery implementation for PS Vita
 */

#include "localsend.h"
#include "main.h"
#include "utils.h"
#include <psp2/net/http.h>
#include <string.h>
#include <stdio.h>

// Helper function to send HTTP GET request and get response
int http_get_request(const char* url, char* response, int max_response_size) {
    int res = -1;
    int statusCode;
    int tmplId = -1, connId = -1, reqId = -1;

    // Create HTTP template
    tmplId = sceHttpCreateTemplate("VitaShell-LocalSend/1.0", SCE_HTTP_VERSION_1_1, SCE_FALSE);
    if (tmplId < 0)
        goto ERROR_EXIT;

    // Create connection
    connId = sceHttpCreateConnectionWithURL(tmplId, url, SCE_FALSE);
    if (connId < 0)
        goto ERROR_EXIT;

    // Create request
    reqId = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, url, 0);
    if (reqId < 0)
        goto ERROR_EXIT;

    // Send request
    res = sceHttpSendRequest(reqId, NULL, 0);
    if (res < 0)
        goto ERROR_EXIT;

    // Get status code
    res = sceHttpGetStatusCode(reqId, &statusCode);
    if (res < 0)
        goto ERROR_EXIT;

    if (statusCode == 200) {
        // Read response data
        int total_read = 0;
        uint8_t buf[1024];

        while (total_read < max_response_size - 1) {
            int read = sceHttpReadData(reqId, buf, sizeof(buf));
            if (read <= 0)
                break;

            int to_copy = MIN(read, max_response_size - 1 - total_read);
            memcpy(response + total_read, buf, to_copy);
            total_read += to_copy;
        }
        response[total_read] = '\0';
        res = total_read;
    } else {
        res = -1;
    }

ERROR_EXIT:
    if (reqId >= 0)
        sceHttpDeleteRequest(reqId);
    if (connId >= 0)
        sceHttpDeleteConnection(connId);
    if (tmplId >= 0)
        sceHttpDeleteTemplate(tmplId);

    return res;
}

// Parse JSON response to extract device info
static void parse_device_info(const char* json, LocalSendDevice* device) {
    // Simple JSON parsing for LocalSend info endpoint
    // Expected format: {"alias":"DeviceName","version":"1.15.0","deviceModel":"iPhone12,1","deviceType":"MOBILE","token":"abc123"}

    const char* alias_start = strstr(json, "\"alias\":");
    if (alias_start) {
        alias_start += 9; // Skip "\"alias\":"
        const char* alias_end = strchr(alias_start, '"');
        if (alias_end && (alias_end - alias_start) < sizeof(device->alias)) {
            strncpy(device->alias, alias_start, alias_end - alias_start);
            device->alias[alias_end - alias_start] = '\0';
        }
    }

    const char* version_start = strstr(json, "\"version\":");
    if (version_start) {
        version_start += 11; // Skip "\"version\":"
        const char* version_end = strchr(version_start, '"');
        if (version_end && (version_end - version_start) < sizeof(device->version)) {
            strncpy(device->version, version_start, version_end - version_start);
            device->version[version_end - version_start] = '\0';
        }
    }

    const char* model_start = strstr(json, "\"deviceModel\":");
    if (model_start) {
        model_start += 15; // Skip "\"deviceModel\":"
        const char* model_end = strchr(model_start, '"');
        if (model_end && (model_end - model_start) < sizeof(device->device_model)) {
            strncpy(device->device_model, model_start, model_end - model_start);
            device->device_model[model_end - model_start] = '\0';
        }
    }

    const char* type_start = strstr(json, "\"deviceType\":");
    if (type_start) {
        type_start += 14; // Skip "\"deviceType\":"
        if (strncmp(type_start, "\"MOBILE\"", 8) == 0) {
            device->device_type = 0;
        } else if (strncmp(type_start, "\"DESKTOP\"", 9) == 0) {
            device->device_type = 1;
        } else if (strncmp(type_start, "\"WEB\"", 5) == 0) {
            device->device_type = 2;
        } else if (strncmp(type_start, "\"HEADLESS\"", 10) == 0) {
            device->device_type = 3;
        } else if (strncmp(type_start, "\"SERVER\"", 8) == 0) {
            device->device_type = 4;
        } else {
            device->device_type = 1; // default to desktop
        }
    }

    // Check if HTTPS is enabled by trying HTTPS first
    device->https_enabled = 0; // For now, assume HTTP only
}

// Discover devices on local network
int localsend_discover_devices_efficient(LocalSendDevice* devices, int max_devices) {
    if (!devices || max_devices <= 0) {
        return -1;
    }

    // First, try to discover devices using common IP ranges
    // We'll focus on the most common ranges: 192.168.1.x and 192.168.0.x
    char ip_ranges[][16] = {"192.168.1.", "192.168.0."};
    int range_count = 2;
    int found = 0;

    // Add a small delay between requests to avoid overwhelming the network
    const int delay_ms = 50;

    for (int r = 0; r < range_count && found < max_devices; r++) {
        // Scan a limited range to keep it fast (1-50 instead of 1-254)
        for (int i = 1; i <= 50 && found < max_devices; i++) {
            char target_ip[16];
            snprintf(target_ip, sizeof(target_ip), "%s%d", ip_ranges[r], i);

            // Skip our own IP if we have one
            if (strlen(vita_ip) > 0 && strcmp(target_ip, vita_ip) == 0) {
                continue;
            }

            // Try to connect to LocalSend info endpoint
            char url[64];
            snprintf(url, sizeof(url), "http://%s:%d/info", target_ip, LOCALSEND_PORT);

            char response[1024];
            int res = http_get_request(url, response, sizeof(response));

            if (res > 0) {
                // Successfully got response, parse device info
                strcpy(devices[found].ip, target_ip);
                devices[found].port = LOCALSEND_PORT;
                parse_device_info(response, &devices[found]);
                found++;

                // Small delay to be network-friendly
                sceKernelDelayThread(delay_ms * 1000);
            }
        }
    }

    return found;
}