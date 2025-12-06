/*
 * LocalSend Lite - File Transfer
 * File transfer implementation for PS Vita
 */

#include "localsend.h"
#include "main.h"
#include "utils.h"
#include "file.h"
#include <psp2/net/http.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper function to create JSON for prepare upload request
static int create_prepare_upload_json(const char* filepath, const char* target_alias, char* json_buffer, int buffer_size) {
    // Get file info
    SceIoStat stat;
    if (sceIoGetstat(filepath, &stat) < 0) {
        return -1;
    }

    // Extract filename from path
    const char* filename = strrchr(filepath, '/');
    if (!filename) {
        filename = filepath;
    } else {
        filename++;
    }

    // Determine MIME type (simplified)
    const char* mime_type = "application/octet-stream";
    const char* ext = strrchr(filename, '.');
    if (ext) {
        if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
            mime_type = "image/jpeg";
        } else if (strcmp(ext, ".png") == 0) {
            mime_type = "image/png";
        } else if (strcmp(ext, ".txt") == 0) {
            mime_type = "text/plain";
        } else if (strcmp(ext, ".pdf") == 0) {
            mime_type = "application/pdf";
        }
    }

    // Create JSON payload
    int len = snprintf(json_buffer, buffer_size,
        "{\"info\":{\"alias\":\"VitaShell\",\"version\":\"%s\",\"deviceModel\":\"PS Vita\",\"deviceType\":\"DESKTOP\",\"token\":\"%s\",\"port\":%d,\"protocol\":\"HTTP\"},"
        "\"files\":{\"%s\":{\"name\":\"%s\",\"size\":%llu,\"mime\":\"%s\"}}}",
        LOCALSEND_PROTOCOL_VERSION, device_token, LOCALSEND_PORT, filename, filename,
        (unsigned long long)stat.st_size, mime_type);

    return (len < buffer_size) ? len : -1;
}

// Helper function to send HTTP POST request with JSON body
static int http_post_request(const char* url, const char* json_data, char* response, int max_response_size) {
    int res;
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

    // Create request with content length
    int content_length = strlen(json_data);
    reqId = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_POST, url, content_length);
    if (reqId < 0)
        goto ERROR_EXIT;

    // Set headers
    sceHttpAddRequestHeader(reqId, "Content-Type", "application/json", SCE_HTTP_HEADER_OVERWRITE);

    // Send request with body
    res = sceHttpSendRequest(reqId, (void*)json_data, content_length);
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

// Helper function to send HTTP PUT request for file upload
static int http_put_file(const char* url, const char* filepath) {
    int res;
    int statusCode;
    int tmplId = -1, connId = -1, reqId = -1;
    SceUID fd = -1;

    // Open source file
    fd = sceIoOpen(filepath, SCE_O_RDONLY, 0);
    if (fd < 0)
        goto ERROR_EXIT;

    // Get file size
    SceOff file_size = sceIoLseek(fd, 0, SCE_SEEK_END);
    sceIoLseek(fd, 0, SCE_SEEK_SET);

    // Create HTTP template
    tmplId = sceHttpCreateTemplate("VitaShell-LocalSend/1.0", SCE_HTTP_VERSION_1_1, SCE_FALSE);
    if (tmplId < 0)
        goto ERROR_EXIT;

    // Create connection
    connId = sceHttpCreateConnectionWithURL(tmplId, url, SCE_FALSE);
    if (connId < 0)
        goto ERROR_EXIT;

    // Create request with file size
    reqId = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_PUT, url, file_size);
    if (reqId < 0)
        goto ERROR_EXIT;

    // Upload file in chunks
    uint8_t buf[4096];
    SceOff bytes_sent = 0;

    while (bytes_sent < file_size) {
        int to_read = MIN(sizeof(buf), file_size - bytes_sent);
        int read = sceIoRead(fd, buf, to_read);
        if (read <= 0)
            break;

        int written = sceHttpSendRequest(reqId, buf, read);
        if (written < 0) {
            res = written;
            goto ERROR_EXIT;
        }

        bytes_sent += read;
    }

    // Get status code
    res = sceHttpGetStatusCode(reqId, &statusCode);
    if (res < 0)
        goto ERROR_EXIT;

    res = (statusCode == 200) ? 0 : -1;

ERROR_EXIT:
    if (fd >= 0)
        sceIoClose(fd);
    if (reqId >= 0)
        sceHttpDeleteRequest(reqId);
    if (connId >= 0)
        sceHttpDeleteConnection(connId);
    if (tmplId >= 0)
        sceHttpDeleteTemplate(tmplId);

    return res;
}

// Parse prepare upload response to get session ID and upload URLs
static int parse_prepare_upload_response(const char* json, char* session_id, int session_id_size,
                                        char upload_urls[][256], int max_files) {
    // Expected format: {"sessionId":"abc123","files":{"filename.txt":"/upload/abc123/filename.txt"}}

    const char* session_start = strstr(json, "\"sessionId\":");
    if (!session_start)
        return -1;

    session_start += 13; // Skip "\"sessionId\":"
    const char* session_end = strchr(session_start, '"');
    if (!session_end || (session_end - session_start) >= session_id_size)
        return -1;

    strncpy(session_id, session_start, session_end - session_start);
    session_id[session_end - session_start] = '\0';

    // For simplicity, we'll assume single file upload
    // In a full implementation, we would parse the files object properly
    return 1; // Number of files
}

// Send file to target device
int localsend_send_file(const char* target_ip, int target_port, const char* filepath) {
    if (!target_ip || !filepath) {
        return -1;
    }

    // Step 1: Prepare upload
    char prepare_url[128];
    snprintf(prepare_url, sizeof(prepare_url), "http://%s:%d/prepare-upload", target_ip, target_port);

    char json_buffer[1024];
    if (create_prepare_upload_json(filepath, target_ip, json_buffer, sizeof(json_buffer)) < 0) {
        return -1;
    }

    char response[1024];
    int res = http_post_request(prepare_url, json_buffer, response, sizeof(response));
    if (res <= 0) {
        return -1;
    }

    // Step 2: Parse response to get upload URL
    char session_id[64];
    char upload_urls[1][256]; // Support single file for now
    int file_count = parse_prepare_upload_response(response, session_id, sizeof(session_id), upload_urls, 1);
    if (file_count <= 0) {
        return -1;
    }

    // Step 3: Upload file
    char upload_url[256];
    const char* filename = strrchr(filepath, '/');
    if (!filename) {
        filename = filepath;
    } else {
        filename++;
    }
    snprintf(upload_url, sizeof(upload_url), "http://%s:%d/upload/%s/%s", target_ip, target_port, session_id, filename);

    res = http_put_file(upload_url, filepath);
    return res;
}

// Receive file (placeholder - would be handled by incoming HTTP server)
int localsend_receive_file(const char* session_id, const char* filename, const char* save_path) {
    // This function would be called when receiving files via HTTP server
    // For PS Vita, implementing a full HTTP server is complex
    // This is a placeholder for future implementation
    return 0;
}