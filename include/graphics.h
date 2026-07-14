#ifndef LINUX_DOCTOR_GRAPHICS_H
#define LINUX_DOCTOR_GRAPHICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define GRAPHICS_DEVICE_LIMIT 8U
#define GRAPHICS_TEXT_CAPACITY 128U

typedef struct GraphicsDevice {
    char card[32];
    char vendor[GRAPHICS_TEXT_CAPACITY];
    char vendor_id[16];
    char device_id[16];
    char driver[GRAPHICS_TEXT_CAPACITY];
    bool boot_vga;
} GraphicsDevice;

typedef struct GraphicsInfo {
    bool device_inventory_available;
    bool inventory_truncated;
    GraphicsDevice devices[GRAPHICS_DEVICE_LIMIT];
    size_t device_count;
    bool session_available;
    char session_type[32];
    bool wayland_session;
    bool x11_session;
    bool vulkan_loader_available;
    size_t vulkan_icd_count;
    bool opengl_loader_available;
} GraphicsInfo;

int graphics_parse_uevent(FILE *stream, GraphicsDevice *device);
int graphics_collect(GraphicsInfo *graphics, char *error, size_t error_size);

#endif
