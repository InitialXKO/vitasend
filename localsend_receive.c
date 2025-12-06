/*
 * LocalSend Lite - File Receive (Client-side implementation)
 * Simplified file receive implementation for PS Vita
 */

#include "localsend.h"
#include "main.h"
#include "utils.h"
#include "file.h"
#include <psp2/net/http.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper function to download file from LocalSend device
static int download_file_from_localsend(const char* target_ip, int target_port, const char* filename, const char* save_path) {
    char url[256];
    snprintf(url, sizeof(url), "http://%s:%d/files/%s", target_ip, target_port, filename);

    // Use existing downloadFile function from network_download.c
    return downloadFile(url, save_path, NULL);
}

// Get list of available files from a LocalSend device
int localsend_get_available_files(const char* target_ip, int target_port, char files[][256], int max_files) {
    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/files", target_ip, target_port);

    char response[4096];
    int res = http_get_request(url, response, sizeof(response));
    if (res <= 0) {
        return -1;
    }

    // Parse JSON response to extract file list
    // Expected format: {"files":["file1.txt","file2.jpg",...]}
    const char* files_start = strstr(response, "\"files\":[");
    if (!files_start) {
        return -1;
    }

    files_start += 9; // Skip "\"files\":["
    const char* files_end = strchr(files_start, ']');
    if (!files_end) {
        return -1;
    }

    int count = 0;
    const char* current = files_start;
    while (current < files_end && count < max_files) {
        const char* quote_start = strchr(current, '"');
        if (!quote_start || quote_start >= files_end) {
            break;
        }
        quote_start++;
        const char* quote_end = strchr(quote_start, '"');
        if (!quote_end || quote_end > files_end) {
            break;
        }

        int len = quote_end - quote_start;
        if (len < 256) {
            strncpy(files[count], quote_start, len);
            files[count][len] = '\0';
            count++;
        }

        current = quote_end + 1;
    }

    return count;
}

// Receive file from LocalSend device
int localsend_receive_file_from_device(const char* target_ip, int target_port, const char* filename, const char* save_directory) {
    if (!target_ip || !filename || !save_directory) {
        return -1;
    }

    // Create full save path
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "%s%s", save_directory, filename);

    // Download the file
    return download_file_from_localsend(target_ip, target_port, filename, save_path);
}