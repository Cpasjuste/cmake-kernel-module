#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
//#include <linux/slab.h>
//#include <linux/unistd.h>
//#include <linux/sched.h>
#include <linux/fs.h>
//#include <linux/file.h>
//#include <linux/mm.h>
//#include <linux/uaccess.h>
//#include <asm/segment.h>
//#include <asm/uaccess.h>
//#include <linux/buffer_head.h>

#include <drm/drm_drv.h>
#include <drm/drm_modes.h>
#include <video/videomode.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cpasjuste");
MODULE_DESCRIPTION("CRT DRM CUSTOM MODES");
MODULE_VERSION("0.1");

static char *timings_path = "/boot/timings.txt";

static int drm_display_mode_from_timings(const char *line, struct drm_display_mode *mode) {

    struct videomode vm;
    int ret, hsync, vsync, interlace;

    if (line != NULL) {
        ret = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %*s %*s %*s %*s %d %ld %*s",
                     &vm.hactive, &hsync, &vm.hfront_porch, &vm.hsync_len, &vm.hback_porch,
                     &vm.vactive, &vsync, &vm.vfront_porch, &vm.vsync_len, &vm.vback_porch,
                     &interlace, &vm.pixelclock);
        if (ret != 12) {
            //printf("[DRM]: malformed mode requested: %s (%i)\n", line, ret);
            return 1;
        }

        // setup flags
        vm.flags = interlace ? DRM_MODE_FLAG_INTERLACE : 0;
        vm.flags |= hsync ? DRM_MODE_FLAG_NHSYNC : DRM_MODE_FLAG_PHSYNC;
        vm.flags |= vsync ? DRM_MODE_FLAG_NVSYNC : DRM_MODE_FLAG_PVSYNC;
        drm_display_mode_from_videomode(&vm, mode);

        return 0;
    }

    return 1;
}

int drm_display_mode_load_timings(void) {

    struct file *fp = NULL;

    char buf[512];

    fp = filp_open(timings_path, O_RDONLY, 0);
    if (IS_ERR(fp) || !fp) {
        printk(KERN_WARNING
               "[CRT_DRM]: timings file not found, skipping custom modes\n");
        return -1;
    }

    ssize_t len = kernel_read(fp, &buf,512, &fp->f_pos);
    printk(KERN_WARNING
           "[CRT_DRM]: read: %i\n", len);
    printk(KERN_WARNING
           "[CRT_DRM]: read: %s\n", buf);

    filp_close(fp, NULL);

    return 0;
}

static int __init hello_start(void) {
    printk(KERN_INFO
           "[CRT_DRM]: module loaded\n");

    drm_display_mode_load_timings();

    return 0;
}

static void __exit hello_end(void) {

    printk(KERN_INFO
           "[CRT_DRM]: module unloaded\n");
}

module_init(hello_start);

module_exit(hello_end);
