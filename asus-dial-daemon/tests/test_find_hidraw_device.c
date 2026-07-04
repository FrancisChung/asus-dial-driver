// Proves find_hidraw_device() identifies the ASUS dial via device/uevent's
// HID_NAME field. I2C-attached HID devices (like the dial, unlike USB ones)
// have no device/product file, so matching on "product" never finds it and
// the daemon falls back to whatever hidraw number happened to be hardcoded.
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../helpers.h"
#include "../openwheel.h"

static void write_file(const char *path, const char *contents) {
    FILE *f = fopen(path, "w");
    assert(f != NULL);
    fputs(contents, f);
    fclose(f);
}

static void run(const char *cmd) {
    int rc = system(cmd);
    assert(rc == 0);
}

// Mimics a real /sys/class/hidraw entry: a symlink named e.g. "hidraw1"
// pointing at a device directory that contains a uevent file with HID_NAME.
static void make_fake_hidraw_entry(const char *base, const char *name, const char *hid_name) {
    char target_device_dir[512], link_path[512], uevent_path[512], mkdir_cmd[600], ln_cmd[1200];

    snprintf(target_device_dir, sizeof(target_device_dir), "%s/targets/%s/device", base, name);
    snprintf(link_path, sizeof(link_path), "%s/hidraw/%s", base, name);
    snprintf(uevent_path, sizeof(uevent_path), "%s/uevent", target_device_dir);

    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p '%s'", target_device_dir);
    run(mkdir_cmd);

    char uevent_contents[256];
    snprintf(uevent_contents, sizeof(uevent_contents),
             "DRIVER=hid-generic\nHID_ID=0018:00000B05:00000220\nHID_NAME=%s\nHID_PHYS=i2c-test\n",
             hid_name);
    write_file(uevent_path, uevent_contents);

    snprintf(ln_cmd, sizeof(ln_cmd), "ln -s '%s/targets/%s' '%s'", base, name, link_path);
    run(ln_cmd);
}

int main(void) {
    char base[] = "/tmp/hidraw_test_XXXXXX";
    assert(mkdtemp(base) != NULL);

    char hidraw_dir[600], mkdir_cmd[700];
    snprintf(hidraw_dir, sizeof(hidraw_dir), "%s/hidraw", base);
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p '%s'", hidraw_dir);
    run(mkdir_cmd);

    // hidraw0 is an unrelated touch device; hidraw1 is the ASUS dial.
    // This mirrors what was observed on real hardware behind a dock.
    make_fake_hidraw_entry(base, "hidraw0", "ELAN9008:00 04F3:4101");
    make_fake_hidraw_entry(base, "hidraw1", "ASUS2020:00 0B05:0220");

    char hidraw_dir_with_slash[620];
    snprintf(hidraw_dir_with_slash, sizeof(hidraw_dir_with_slash), "%s/", hidraw_dir);

    char device_path[BUFFER_SIZE];
    int rc = find_hidraw_device(hidraw_dir_with_slash, device_path, sizeof(device_path));

    assert(rc == 0);
    assert(strcmp(device_path, "/dev/hidraw1") == 0);

    printf("PASS: find_hidraw_device -> %s\n", device_path);
    return 0;
}
