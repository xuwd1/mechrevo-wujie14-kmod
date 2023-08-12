# Mechrevo WUJIE14 Kernel Module

## What is this

This is a Linux kernel module for Mechrevo WUJIE14 laptop that implements a Linux platform profile driver and exports keyboard backlight level/shut-time sysfs attributes.

## More detailed explainations on the purpose of this kernel module

### 1. Laptop performance mode and Linux platform profile interface

The Mechrevo WUJIE14 laptop has three performace modes namely quiet, balanced and rage which could be switched with key combination Fn+X. This switching is handled all by the emdedded controller(EC) that also serves as the keyboard controller thus is OS/software independent, i.e., without any need of some software, the EC itself is programmed to detect the key combination and do the switching, which is very ideal.

However the WUJIE14 does NOT have any hardware such as some LED light that gives the users an indication of what the current performance mode is. On Windows this problem is solved with a proprietary OSD software that shows the current performance mode whenever Fn+X is pressed(and consequently a wmi event notified). However with stock Linux the only way to tell this is by performing a stress test and checking the power consumption.

Luckily WUJIE14 has these performance mode related [wmi](https://www.kernel.org/doc/html/next/wmi/acpi-interface.html) methods and events implemented inside its ACPI table. Thus this kernel module first implements a linux platform profile driver that enables the user to:

- Read the current performance mode via `/sys/firmware/acpi/platform_profile` :
    ```shell
    $ cat /sys/firmware/acpi/platform_profile
    balanced
    ```

- Get a list of available perf modes via `/sys/firmware/acpi/platform_profile_choices`:
    ```shell
    $ cat /sys/firmware/acpi/platform_profile_choices
    quiet balanced performance
    ```

- Set the perf mode via `/sys/firmware/acpi/platform_profile`:
    ```shell
    $ echo 'quiet' | sudo tee /sys/firmware/acpi/platform_profile
    $ cat /sys/firmware/acpi/platform_profile
    quiet
    ```

The better part is that some Linux desktop environments(DE) implement GUI power profile tweaking tools on top of this standard interface. Taking KDE as example, with this kernel module loaded you could conveniently check and set the performance mode with the "Battery and Brightness" system tray entry:

<img src="assets/systemtray.png" alt="systray" width="200" />


### 2. Keyboard backlight brigtness level and auto-shutoff time configuring

The WUJIE14 has keyboard backlight with three brightness level off, dim and bright that could be switched by pressing Fn+F9 and this is also controlled by the EC. However a major problem with this backlight is that the backlight would automatically be turned off after 30 seconds and this auto-shutoff time cannot be easily altered without writing the EC ram. This kernel module then provides:

- A readonly sysfs attribute that reports the current backlight brightness level via `/sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/keyboard_backlight_level`:

    ```shell
    $ cat /sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/keyboard_backlight_level
    5C
    ```
    
    The possible values here are: 0 for off, 5C for dim and B8 for bright.

- A readwrite sysfs attribute that allows the users to set the auto-shutoff time via `/sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/keyboard_backlight_time`:

    ```shell
    # read the backlight time
    $ cat /sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/keyboard_backlight_time
    0
    ```
    The default value read is always zero after a cold system boot. Note that the last modification to the auto-shutoff time would persist across reboots since the EC won't be reset.

    ```shell
    # write the backlight time
    $ echo -1 | sudo tee /sys/devices/pci0000:00/0000:00:14.3/PNP0C09:00/keyboard_backlight_time
    ```
    Here the number written should be in range of signed char(s8), i.e., [-128,127] and it is only known that here 0 represents 30s, 1 represents 1 minute and -1 represents forever.
