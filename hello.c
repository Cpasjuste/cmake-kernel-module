#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#include <drm/drm_drv.h>
#include <drm/drm_modes.h>
#include <video/videomode.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cpasjuste");
MODULE_DESCRIPTION("CRT DRM CUSTOM MODES");
MODULE_VERSION("0.1");

// ~ 20 timings line + comments
#define READ_SIZE_MAX 2048
#define LINE_SIZE_MAX 256

static char read_buf[READ_SIZE_MAX];
static char *timings_path = "/boot/timings.txt";

static struct drm_display_mode *drm_display_mode_from_timings(struct drm_connector *connector, const char *line) {

    int ret, hsync, vsync, interlace;
    struct drm_display_mode *mode = NULL;
    struct videomode vm;

    if (line != NULL) {
        memset(&vm, 0, sizeof(vm));
        ret = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %*s %*s %*s %*s %d %ld %*s",
                     &vm.hactive, &hsync, &vm.hfront_porch, &vm.hsync_len, &vm.hback_porch,
                     &vm.vactive, &vsync, &vm.vfront_porch, &vm.vsync_len, &vm.vback_porch,
                     &interlace, &vm.pixelclock);
        if (ret != 12) {
            printk(KERN_WARNING
                   "[CRT_DRM]: malformed mode requested, skipping (%s)\n", line);
            return NULL;
        }

        // setup flags
        vm.flags = interlace ? DRM_MODE_FLAG_INTERLACE : 0;
        vm.flags |= hsync ? DRM_MODE_FLAG_NHSYNC : DRM_MODE_FLAG_PHSYNC;
        vm.flags |= vsync ? DRM_MODE_FLAG_NVSYNC : DRM_MODE_FLAG_PVSYNC;

        // create/init display mode, convert from video mode
        mode = drm_mode_create(connector->dev);
        if (mode == NULL) {
            printk(KERN_WARNING
                   "[CRT_DRM]: drm_mode_create failed, skipping (%s)\n", line);
            return NULL;
        }
        mode->type = DRM_MODE_TYPE_DRIVER;
        drm_display_mode_from_videomode(&vm, mode);

        return mode;
    }

    return NULL;
}

int drm_display_mode_load_timings(struct drm_connector *connector) {

    struct file *fp = NULL;
    ssize_t read_size = 0;
    size_t cursor = 0;
    char line[LINE_SIZE_MAX];
    size_t line_start = 0;
    size_t line_len = 0;
    struct drm_display_mode *mode = NULL;
    int mode_count = 0;

    fp = filp_open(timings_path, O_RDONLY, 0);
    if (IS_ERR(fp) || !fp) {
        printk(KERN_WARNING
               "[CRT_DRM]: timings file not found, skipping custom modes loading\n");
        return 0;
    }

    read_size = kernel_read(fp, &read_buf, READ_SIZE_MAX, &fp->f_pos);
    if (read_size <= 0) {
        filp_close(fp, NULL);
        printk(KERN_WARNING
               "[CRT_DRM]: empty timings file found, skipping custom modes loading\n");
        return 0;
    }
    filp_close(fp, NULL);

    for (cursor = 0; cursor < read_size; cursor++) {
        line[cursor - line_start] = read_buf[cursor];
        line_len++;
        if (line_len >= LINE_SIZE_MAX || read_buf[cursor] == '\n' || read_buf[cursor] == '\0') {
            if (line_len > 32 && line[0] != '#') {
                line[line_len - 1] = '\0';
                if ((mode = drm_display_mode_from_timings(connector, line)) != NULL) {
                    printk(KERN_INFO
                           "[CRT_DRM]: mode: " DRM_MODE_FMT, DRM_MODE_ARG(mode));
                    drm_mode_probed_add(connector, mode);
                    mode_count++;
                }
            }
            line_start += line_len;
            line_len = 0;
            memset(line, 0, 128);
        }
    }

    return mode_count;
}

static int __init hello_start(void) {

    printk(KERN_INFO
           "[CRT_DRM]: CTR_DRM module loaded\n");

    drm_display_mode_load_timings(NULL);

    return 0;
}

static void __exit hello_end(void) {

    printk(KERN_INFO
           "[CRT_DRM]: CTR_DRM module unloaded\n");
}

module_init(hello_start);

module_exit(hello_end);
