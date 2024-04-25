#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/wmi.h>

#include "wujie14-km.h"
#include "wujie14-wmi-event.h"
#include "wujie14-perfmode.h"

static const struct wujie14_wmi_notify_handler_entry handler_tbl[] = {
    {
        .febc_offset = 1,
        .febc_value = 0x11,
        .handler = wujie14_powermode_wmi_event_handler
    },
    {
        .febc_offset = 1,
        .febc_value = 0x12,
        .handler = wujie14_powermode_wmi_event_handler
    },
    {
        .febc_offset = 1,
        .febc_value = 0x13,
        .handler = wujie14_powermode_wmi_event_handler
    },
    {}
};

static inline wujie14_wmi_notify_handler locate_handler(
    u8* response_buffer
){
    const struct wujie14_wmi_notify_handler_entry* cur;
    wujie14_wmi_notify_handler ret_handler = NULL;
    cur = handler_tbl;
    while(cur->febc_offset){
        if (response_buffer[cur->febc_offset] == cur->febc_value){
            ret_handler = cur->handler;
            break;
        }
        cur++;
    }
    return ret_handler;
}

static void wujie14_wmi_event_handler(
    u32 value,
    void* context
)
{
    struct acpi_buffer response = {ACPI_ALLOCATE_BUFFER, NULL};
    union acpi_object* obj;
    acpi_status status;
    wujie14_wmi_notify_handler handler;
    struct wujie14_private* priv = context;
    status = wmi_get_event_data(value, &response);
    if (ACPI_FAILURE(status)) {
        dev_err(&priv->pdev->dev, "get event data failed\n");
        return;
    }
    obj = response.pointer;
    handler = locate_handler(obj->buffer.pointer);
    if (handler == NULL){
        return;
    }
    handler(priv); 
    ACPI_FREE(response.pointer);
}

int wujie14_wmi_event_init(struct wujie14_private* priv)
{
    acpi_status status;
    status = wmi_install_notify_handler(
        WUJIE14_IP3WMIEVENT_GUID,
        wujie14_wmi_event_handler,
        priv);
    if (ACPI_FAILURE(status)){
        dev_err(&priv->pdev->dev, "get event data failed\n");
        return -ENODEV;
    }
    return 0;
}

void wujie14_wmi_event_exit(struct wujie14_private* priv)
{
    wmi_remove_notify_handler(WUJIE14_IP3WMIEVENT_GUID);
}