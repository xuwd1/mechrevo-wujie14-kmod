#include <asm/io.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>

#include "wujie14-kb.h"
#include "wujie14-km.h"

#define NTRIAL 5
#define MWAIT 50

//todo: ec mutex

static inline int test_bit_set(int value, int bitnr)
{
    int bitmask = 1L<<bitnr;
    return (bitmask & value)!=0;
}

static inline int test_bit_clear(int value, int bitnr)
{
    return test_bit_set(~value, bitnr);
}

static inline u8 read_ec_status(void){
    return inb(EC_COMMAND_PORT);
}

static int ec_waitwrite(void){
    u8 status;
    int i=0;
    for (;i<NTRIAL;i++){
        status = read_ec_status();
        if (test_bit_clear(status,IBF)){
            return 0;
        }
        msleep(MWAIT);
    }
    return -ETIME;
}

static int ec_waitread(void){
    u8 status;
    int i=0;
    for (;i<NTRIAL;i++){
        status = read_ec_status();
        if (test_bit_set(status,OBF)){
            return 0;
        }
        msleep(MWAIT);
    }
    return -ETIME;
}

static inline void write_ec_command(enum ec_command cmd){
    outb((u8)cmd,EC_COMMAND_PORT);
}

static inline u8 read_ec_data(void){
    return inb(EC_DATA_PORT);
}

static inline void write_ec_data(u8 data){
    outb(data,EC_DATA_PORT);
}

static int read_ecram(u8 addr, u8* data){
    int err = 0;
    err = ec_waitwrite();
    if (err) goto error;
    write_ec_command(RD_EC);
    err = ec_waitwrite();
    if (err) goto error;
    write_ec_data(addr);
    err = ec_waitread();
    if (err) goto error;
    *data = read_ec_data();
error:
    return err;
}

static int write_ecram(u8 addr, u8 data){
    int err = 0;
    err = ec_waitwrite();
    if (err) goto error;
    write_ec_command(WR_EC);
    err = ec_waitwrite();
    if (err) goto error;
    write_ec_data(addr);
    err = ec_waitwrite();
    if (err) goto error;
    write_ec_data(data);
error:
    return err;
}

static ssize_t keyboard_backlight_level_show(struct device* dev,
                    struct device_attribute* attr, char* buf)
{
    int err;
    u8 data;
    err = read_ecram(0x09,&data);
    if (err){
        return err;
    }
    return sysfs_emit(buf, "%hhX\n", data);
}

//static inline u8 round_next_bl_level(u8 backlight_level){
//    u8 rounded=0;
//    if (backlight_level <= KBLT_OFF) {
//        rounded = KBLT_OFF;
//    } else if (backlight_level <= KBLT_MED){
//        rounded = KBLT_MED;
//    } else if (backlight_level <= KBLT_HI){
//        rounded = KBLT_HI;
//    }
//    return rounded;
//};
//
//static ssize_t keyboard_backlight_store(struct device *dev,
//			        struct device_attribute *attr, const char *buf,
//			        size_t count)
//{
//    int err;
//    u8 new_backlight;
//    err = kstrtou8(buf,0,&new_backlight);
//    if (err){
//        return err;
//    }
//    new_backlight = round_next_bl_level(new_backlight);
//    err = write_ecram(0x09, new_backlight);
//    if (err){
//        return err;
//    }
//    return count;
//}

static DEVICE_ATTR_RO(keyboard_backlight_level);


static ssize_t keyboard_backlight_time_show(struct device* dev,
                    struct device_attribute* attr, char* buf)
{
    int err;
    u8 data;
    err = read_ecram(0x34,&data);
    if (err){
        return err;
    }
    return sysfs_emit(buf, "%hhX\n", data);
}

static ssize_t keyboard_backlight_time_store(struct device *dev,
			        struct device_attribute *attr, const char *buf,
			        size_t count)
{
    int err;
    s8 new_time;
    err = kstrtos8(buf,0,&new_time);
    if (err) {
        return err;
    }
    err = write_ecram(0x34, (u8)new_time);
    if (err) {
        return err;
    }
    return count;
}

static DEVICE_ATTR_RW(keyboard_backlight_time);

// sysfs attrs should be under /sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00
int wujie14_sysfs_init(struct wujie14_private* priv){
    int err = 0;
    err = device_create_file(&priv->pdev->dev,&dev_attr_keyboard_backlight_level);
    if (err) goto error;
    err = device_create_file(&priv->pdev->dev,&dev_attr_keyboard_backlight_time);
    if (err) goto error;
error:
    return err;
}

void wujie14_sysfs_exit(struct wujie14_private* priv){
    device_remove_file(&priv->pdev->dev,&dev_attr_keyboard_backlight_level);
    device_remove_file(&priv->pdev->dev,&dev_attr_keyboard_backlight_time);
}
