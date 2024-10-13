#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "wujie14-km.h"

enum ec_command{
    RD_EC = 0x80,
    WR_EC = 0x81
};

enum ec_status_bit{
    OBF = 0,
    IBF = 1
};

enum kb_backlight_level{
    KBLT_OFF = 0,
    KBLT_MED = 0x5C,
    KBLT_HI = 0xB8
};

#define EC_COMMAND_PORT 0x66
#define EC_DATA_PORT 0x62


int wujie14_kbd_sysfs_init(struct wujie14_private* priv);
void wujie14_kbd_sysfs_exit(struct wujie14_private* priv);