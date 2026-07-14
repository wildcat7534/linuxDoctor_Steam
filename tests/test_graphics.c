#define _POSIX_C_SOURCE 200809L

#include "graphics.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static FILE *fixture_stream(const char *contents)
{
    FILE *stream = tmpfile();

    assert(stream != NULL);
    assert(fputs(contents, stream) >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    return stream;
}

int main(void)
{
    GraphicsDevice device = {0};
    GraphicsInfo graphics;
    char manifest_path[160];
    char manifest_directory[160];
    char directory_manifest[200];
    FILE *stream = fixture_stream(
        "DRIVER=nvidia\n"
        "PCI_CLASS=30000\n"
        "PCI_ID=10DE:2204\n");

    assert(graphics_parse_uevent(stream, &device) == 0);
    assert(strcmp(device.driver, "nvidia") == 0);
    assert(strcmp(device.vendor, "NVIDIA") == 0);
    assert(strcmp(device.vendor_id, "10DE") == 0);
    assert(strcmp(device.device_id, "2204") == 0);
    assert(fclose(stream) == 0);

    device = (GraphicsDevice){0};
    stream = fixture_stream("PCI_ID=8086:9A49\n");
    assert(graphics_parse_uevent(stream, &device) == 0);
    assert(strcmp(device.vendor, "Intel") == 0);
    assert(device.driver[0] == '\0');
    assert(fclose(stream) == 0);

    device = (GraphicsDevice){0};
    stream = fixture_stream("PCI_ID=malformed\nDRIVER=amdgpu\n");
    assert(graphics_parse_uevent(stream, &device) == 0);
    assert(strcmp(device.vendor, "Inconnu") == 0);
    assert(strcmp(device.driver, "amdgpu") == 0);
    assert(fclose(stream) == 0);

    assert(graphics_parse_uevent(NULL, &device) == -1);
    stream = fixture_stream("DRIVER=nvidia\n");
    assert(graphics_parse_uevent(stream, NULL) == -1);
    assert(fclose(stream) == 0);
    assert(graphics_collect(NULL, NULL, 0U) == -1);

    (void)snprintf(manifest_path, sizeof(manifest_path), "/tmp/linux-doctor-vulkan-%ld.json", (long)getpid());
    stream = fopen(manifest_path, "w");
    assert(stream != NULL);
    assert(fputs("{\"file_format_version\":\"1.0.0\",\"ICD\":{\"library_path\":\"libtest.so\"}}", stream) >= 0);
    assert(fclose(stream) == 0);
    assert(setenv("VK_DRIVER_FILES", manifest_path, 1) == 0);
    assert(setenv("XDG_SESSION_TYPE", "tty", 1) == 0);
    assert(unsetenv("WAYLAND_DISPLAY") == 0);
    assert(unsetenv("DISPLAY") == 0);
    assert(graphics_collect(&graphics, NULL, 0U) == 0);
    assert(graphics.vulkan_icd_count == 1U);
    assert(!graphics.session_available);
    assert(strcmp(graphics.session_type, "tty") == 0);
    assert(graphics.device_count <= GRAPHICS_DEVICE_LIMIT);
    if (graphics.wayland_session || graphics.x11_session) assert(graphics.session_available);

    (void)snprintf(manifest_directory, sizeof(manifest_directory), "/tmp/linux-doctor-vulkan-%ld", (long)getpid());
    assert(mkdir(manifest_directory, 0700) == 0);
    (void)snprintf(directory_manifest, sizeof(directory_manifest), "%s/test.json", manifest_directory);
    stream = fopen(directory_manifest, "w");
    assert(stream != NULL);
    assert(fputs("{\"ICD\":{\"library_path\":\"libtest.so\"}}", stream) >= 0);
    assert(fclose(stream) == 0);
    assert(setenv("VK_DRIVER_FILES", manifest_directory, 1) == 0);
    assert(graphics_collect(&graphics, NULL, 0U) == 0);
    assert(graphics.vulkan_icd_count == 1U);
    assert(unlink(directory_manifest) == 0);
    assert(rmdir(manifest_directory) == 0);

    stream = fopen(manifest_path, "w");
    assert(stream != NULL);
    assert(fputs("{}", stream) >= 0);
    assert(fclose(stream) == 0);
    assert(setenv("VK_DRIVER_FILES", manifest_path, 1) == 0);
    assert(graphics_collect(&graphics, NULL, 0U) == 0);
    assert(graphics.vulkan_icd_count == 0U);
    assert(unlink(manifest_path) == 0);
    assert(unsetenv("VK_DRIVER_FILES") == 0);
    return 0;
}
