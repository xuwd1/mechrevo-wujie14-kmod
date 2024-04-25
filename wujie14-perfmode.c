#include <linux/kernel.h>
#include <linux/platform_profile.h>
#include <linux/acpi.h>
#include <linux/wmi.h>
#include <linux/platform_device.h>

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

void wujie14_powermode_wmi_event_handler(struct wujie14_private* priv)
{
    dev_info(&priv->pdev->dev, "powermode change\n");
    platform_profile_notify();
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
    set_bit(PLATFORM_PROFILE_QUIET, priv->pphandler.choices);
    set_bit(PLATFORM_PROFILE_BALANCED, priv->pphandler.choices);
    set_bit(PLATFORM_PROFILE_PERFORMANCE, priv->pphandler.choices);
    priv->pphandler.profile_get = wujie14_platform_profile_get;
    priv->pphandler.profile_set = wujie14_platform_profile_set;
    err = platform_profile_register(&priv->pphandler);
    return err;
}

void wujie14_platform_profile_exit(struct wujie14_private* priv)
{
    platform_profile_remove();
}


