/*
 * LocalSend Lite for VitaShell
 * Lightweight LocalSend client implementation for PS Vita
 */

#ifndef LOCALSEND_H
#define LOCALSEND_H

#include <psp2/types.h>
#include <psp2/net/http.h>

// LocalSend constants
#define LOCALSEND_PORT 53317
#define LOCALSEND_PROTOCOL_VERSION "2.1"
#define LOCALSEND_PEER_PROTOCOL_VERSION "1.0"
#define LOCALSEND_MULTICAST_GROUP "224.0.0.167"

// Device structure
typedef struct {
    char ip[16];
    int port;
    char alias[64];
    char version[16];
    char device_model[32];
    int device_type; // 0: mobile, 1: desktop, 2: web, 3: headless, 4: server
    int https_enabled;
} LocalSendDevice;

extern char device_token[64];

// File transfer structure
typedef struct {
    char filename[256];
    char filepath[512];
    uint64_t size;
    char mime_type[64];
} LocalSendFile;

// Function prototypes
int localsend_init();
int localsend_discover_devices(LocalSendDevice* devices, int max_devices);
int localsend_discover_devices_efficient(LocalSendDevice* devices, int max_devices);
int localsend_register_device(const char* alias, const char* device_model, int device_type);
int localsend_send_file(const char* target_ip, int target_port, const char* filepath);
int localsend_receive_file(const char* session_id, const char* filename, const char* save_path);
int localsend_get_available_files(const char* target_ip, int target_port, char files[][256], int max_files);
int localsend_receive_file_from_device(const char* target_ip, int target_port, const char* filename, const char* save_directory);
void localsend_cleanup();

#endif // LOCALSEND_H