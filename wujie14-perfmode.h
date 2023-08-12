#ifndef WUJIE14_PLATPROFILE_H
#define WUJIE14_PLATPROFILE_H

#include "wujie14-km.h"

/*
    wujie 14:
    1: performance 
    2: quiet
    0: balanced
*/

enum wujie14_powermode {
    WUJIE14_BALANCED_MODE = 0,
    WUJIE14_PERFORMANCE_MODE = 1,
    WUJIE14_QUIET_MODE = 2
};

#define WUJIE14_IP3POWERSWITCH_WMI_GUID "99D89064-8D50-42BB-BEA9-155B2E5D0FCD"
#define WUJIE14_IP3POWERSWITCH_SET_METHODID 1
#define WUJIE14_IP3POWERSWITCH_GET_METHODID 2


int wujie14_platform_profile_init(struct wujie14_private* priv);
void wujie14_platform_profile_exit(struct wujie14_private* priv);
void wujie14_powermode_wmi_event_handler(struct wujie14_private* priv);

#endif