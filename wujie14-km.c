#include <linux/acpi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>
#include <linux/device.h>


#include "wujie14-km.h"
#include "wujie14-perfmode.h"
#include "wujie14-wmi-event.h"
#include "wujie14-kb.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Xu");
MODULE_DESCRIPTION("mechrevo wujie 14 driver km");

static struct wujie14_private priv;


int wujie_plat_probe(struct platform_device *pdev)
{
	const struct dmi_system_id *dmi_match;	
	struct acpi_device* adev;
	unsigned long long ec_sta_result;
	int err;
	priv.pdev = pdev;
	// do dmi match first
	dmi_match = dmi_first_match(wujie14_allowlist);
	if (dmi_match == NULL) {
		dev_err(&pdev->dev, "Unsupported laptop model.\n");
		return -EINVAL;
	}
	// EC acpi _STA test
	adev = ACPI_COMPANION(&pdev->dev);
	err = acpi_evaluate_integer(adev->handle, "_STA", NULL, &ec_sta_result);
	if (err) {
		dev_err(&pdev->dev, "ACPI _STA evaluation failed.\n");
		return err;
	}
	dev_dbg(&pdev->dev, "got _STA %lld\n", ec_sta_result);
	err = wujie14_platform_profile_init(&priv);
	if (err) {
		dev_err(&pdev->dev, "Platform profile initialization failed.\n");
		return err;
	}
	err = wujie14_wmi_event_init(&priv);
	if (err) {
		return err;
	}
	err = wujie14_sysfs_init(&priv);
	if (err) {
		return err;
	}

	return 0;
}

void wujie_plat_remove(struct platform_device *pdev)
{
	wujie14_platform_profile_exit(&priv);
	wujie14_wmi_event_exit(&priv);
	wujie14_sysfs_exit(&priv);
}


static const struct acpi_device_id wujie_device_ids[] = {
	{ "PNP0C09", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, wujie_device_ids);

static struct platform_driver wujie14_platdriver = {
	.probe = wujie_plat_probe,
	.remove_new = wujie_plat_remove,
	.driver = {
		.name   = "wujie14",
		.acpi_match_table = ACPI_PTR(wujie_device_ids),
	},
};

static int __init wujie14_init(void)
{
	int err;
	err = platform_driver_register(&wujie14_platdriver);
	return err;
	
}

static void __exit wujie14_exit(void)
{
	platform_driver_unregister(&wujie14_platdriver);
}

module_init(wujie14_init);
module_exit(wujie14_exit);