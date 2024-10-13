#include <linux/kernel.h>
#include <linux/platform_profile.h>
#include <linux/acpi.h>
#include <linux/wmi.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#include "wujie14-perfmode.h"
#include "wujie14-km.h"


static int wujie14_set_powermode_wmi(struct wujie14_private* priv, enum wujie14_powermode powermode)
{
    acpi_status status;
    int retval;
    u8 powermode_in = powermode;
    struct acpi_buffer params = {sizeof(powermode_in), &powermode_in};
    if (!(powermode == WUJIE14_QUIET_MODE || 
        powermode == WUJIE14_BALANCED_MODE ||
        powermode == WUJIE14_PERFORMANCE_MODE)) {
        return -EINVAL;
    }
    status = wmi_evaluate_method(
        WUJIE14_IP3POWERSWITCH_WMI_GUID,
        0,
        WUJIE14_IP3POWERSWITCH_SET_METHODID,
        &params,
        NULL);
    if (ACPI_FAILURE(status)) {
        dev_err(&priv->pdev->dev,"wmi call failed\n");
        retval = -EIO;
    } else {
        retval = 0;
    }
    
    return retval;
}

static int wujie14_get_powermode_wmi(struct wujie14_private* priv, enum wujie14_powermode* powermode)
{
    acpi_status status;
    int retval;
    struct acpi_buffer params = {0, NULL};
    struct acpi_buffer outbuffer = {ACPI_ALLOCATE_BUFFER, NULL};
    union acpi_object* out;

    status = wmi_evaluate_method(
        WUJIE14_IP3POWERSWITCH_WMI_GUID,
        0,
        WUJIE14_IP3POWERSWITCH_GET_METHODID,
        &params,
        &outbuffer
    );
    if (ACPI_FAILURE(status)) {
        dev_err(&priv->pdev->dev,"wmi call failed\n");
        retval = -EIO;
    } else {
        retval = 0;
    }
    out = outbuffer.pointer;
    *powermode = out->integer.value;
    ACPI_FREE(outbuffer.pointer);
    
    return retval;
}

extern struct wujie14_private priv;

// utility functions

enum wujie14_powermode wujie14_powermode_from_string(const char* str){
    // first convert the string to lowercase
    // char* lower_str = kstrdup(str,GFP_KERNEL);
    // char* p = lower_str;
    // while(*p){
    //     *p = tolower(*p);
    //     p++;
    // }
    enum wujie14_powermode powermode;
    if (strncmp(str,"quiet",5) == 0){
        powermode = WUJIE14_QUIET_MODE;
    } else if (strncmp(str,"balanced",8) == 0){
        powermode = WUJIE14_BALANCED_MODE;
    } else if (strncmp(str,"performance",11) == 0){
        powermode = WUJIE14_PERFORMANCE_MODE;
    } else {
        powermode = WUJIE14_INVALID_MODE;
    }
    return powermode;
}

const char* wujie14_powermode_to_string(enum wujie14_powermode mode){
    switch (mode){
        case WUJIE14_QUIET_MODE:
            return "quiet";
        case WUJIE14_BALANCED_MODE:
            return "balanced";
        case WUJIE14_PERFORMANCE_MODE:
            return "performance";
        default:
            return "unknown";
    }
}

static bool is_number(const char* str){
    const char* current_char_ptr = str;
    while(1){
        char current_char = *current_char_ptr;
        if (current_char == '\0' || current_char == '\n' || current_char == ' '){
            break;
        }
        if (!isdigit(current_char)){
            return false;
        }
        current_char_ptr++;
    }
    return true;
}



// sysfs attr to show all eligible powermodes
// The number in the square brackets is the corresponding powermode enum value


static ssize_t wujie14_eligible_powermodes_show(
    struct device* dev, struct device_attribute* attr, char* buf
){
    return sysfs_emit(buf, "[2] quiet\n[0] balanced\n[1] performance\n");
}

static DEVICE_ATTR_RO(wujie14_eligible_powermodes);

// sysfs attr to show/set the current powermode
// The set interface expects a number in the square brackets, or a string representation of the powermode

static ssize_t wujie14_powermode_show(
    struct device* dev, struct device_attribute* attr, char* buf
){
    int err;
    enum wujie14_powermode powermode;
    const char* powermode_str;
    err = wujie14_get_powermode_wmi(&priv, &powermode);
    if (err) {
        return err;
    }
    powermode_str = wujie14_powermode_to_string(powermode);
    return sysfs_emit(buf, "%s\n", powermode_str);
}

static ssize_t wujie14_powermode_store(
    struct device* dev, struct device_attribute* attr, const char* buf, size_t count
){
    int err;
    enum wujie14_powermode powermode;
    if (is_number(buf)){
        dev_dbg(dev,"got number\n");
        err = kstrtoint(buf, 0, (int*)&powermode);
    } else {
        dev_dbg(dev,"got string\n");
        powermode = wujie14_powermode_from_string(buf);
        if (powermode == WUJIE14_INVALID_MODE){
            err = -EINVAL;
        } else {
            err = 0;
        }
    }
    if (err) {
        return err;
    }
    err = wujie14_set_powermode_wmi(&priv, powermode);
    if (err) {
        return err;
    }
    return count;

}

static DEVICE_ATTR_RW(wujie14_powermode);

int wujie14_powermode_sysfs_init(struct wujie14_private* priv)
{
    int err;
    err = device_create_file(&priv->pdev->dev, &dev_attr_wujie14_powermode);
    if (err) goto error;
    err = device_create_file(&priv->pdev->dev, &dev_attr_wujie14_eligible_powermodes);
    if (err) goto error;
error:
    return err;
}

void wujie14_powermode_sysfs_exit(struct wujie14_private* priv)
{
    device_remove_file(&priv->pdev->dev, &dev_attr_wujie14_powermode);
    device_remove_file(&priv->pdev->dev, &dev_attr_wujie14_eligible_powermodes);
}

void wujie14_powermode_wmi_event_handler(struct wujie14_private* priv)
{
    enum wujie14_powermode current_powermode;
    int err = wujie14_get_powermode_wmi(priv, &current_powermode);
    if (err) {
        dev_err(&priv->pdev->dev,"failed to get powermode\n");
        return;
    }
    dev_info(&priv->pdev->dev, "powermode change. Current powermode: %s\n", wujie14_powermode_to_string(current_powermode));
#if !defined (NO_PLATFORM_PROFILE)
    platform_profile_notify();
#endif
}

static int wujie14_platform_profile_set(
    struct platform_profile_handler* handler,
    enum platform_profile_option profile_option
){
    enum wujie14_powermode powermode;
    struct wujie14_private* priv = container_of(handler, struct wujie14_private, pphandler);
    struct platform_device* pdev = priv->pdev;
    switch (profile_option) {
        case PLATFORM_PROFILE_BALANCED:
            powermode = WUJIE14_BALANCED_MODE;
            break;
        case PLATFORM_PROFILE_PERFORMANCE:
            powermode = WUJIE14_PERFORMANCE_MODE;
            break;
        case PLATFORM_PROFILE_QUIET:
            powermode = WUJIE14_QUIET_MODE;
            break;
        default:
            return -EINVAL;
    }
    dev_dbg(&pdev->dev,"platform_profile_set\n");
    return wujie14_set_powermode_wmi(priv, powermode);
}

static int wujie14_platform_profile_get(
    struct platform_profile_handler* handler,
    enum platform_profile_option* profile_option
){
    int err;
    enum wujie14_powermode powermode;
    struct wujie14_private* priv = container_of(handler, struct wujie14_private, pphandler);
    struct platform_device* pdev = priv->pdev;
    err = wujie14_get_powermode_wmi(priv, &powermode);
    if (err) {
        return err;
    }
    switch (powermode){
        case WUJIE14_BALANCED_MODE:
            *profile_option = PLATFORM_PROFILE_BALANCED;
            break;
        case WUJIE14_PERFORMANCE_MODE:
            *profile_option = PLATFORM_PROFILE_PERFORMANCE;
            break;
        case WUJIE14_QUIET_MODE:
            *profile_option = PLATFORM_PROFILE_QUIET;
            break;
    }
    dev_dbg(&pdev->dev,"platform_profile_get\n");
    return 0;
}

int wujie14_platform_profile_init(struct wujie14_private* priv)
{
    int err = 0;
#if !defined (NO_PLATFORM_PROFILE)
    set_bit(PLATFORM_PROFILE_QUIET, priv->pphandler.choices);
    set_bit(PLATFORM_PROFILE_BALANCED, priv->pphandler.choices);
    set_bit(PLATFORM_PROFILE_PERFORMANCE, priv->pphandler.choices);
    priv->pphandler.profile_get = wujie14_platform_profile_get;
    priv->pphandler.profile_set = wujie14_platform_profile_set;
    err = platform_profile_register(&priv->pphandler);
#endif
    return err;
}

void wujie14_platform_profile_exit(struct wujie14_private* priv)
{
#if !defined (NO_PLATFORM_PROFILE)
    platform_profile_remove();
#endif
}


