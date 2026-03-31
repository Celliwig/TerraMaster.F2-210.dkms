// SPDX-License-Identifier: GPL-2.0+
/*
 * Realtek RTD129x watchdog
 *
 * Copyright (c) 2017 Andreas Färber
 *
 * Updated 2026 - Celliwig <celliwig@nym.hush.com>
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <linux/watchdog.h>


#define DEV_NAME				"rtd1295-wtd"
#define WATCHDOG_TIMEOUT			30

#define RTD119X_TCW_CTRL			0x0
#define RTD119X_TCW_CLR				0x4
#define RTD119X_TCW_TIMEOUT			0xc
#define RTD119X_TCW_COUNT			0x40

#define RTD119X_TCW_CTRL_WDEN_DISABLED		0xa5
#define RTD119X_TCW_CTRL_WDEN_ENABLED		0xff
#define RTD119X_TCW_CTRL_WDEN_MASK		0xff

#define RTD119X_TCW_CLR_WDCLR			BIT(0)


struct rtd119x_watchdog_device {
	struct watchdog_device wdt_dev;
	void __iomem *base;
	struct clk *clk;
};


// Module parameters
////////////////////////////////////////////////////////////////////////////////
static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
        __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static unsigned timeout;
module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout, "Watchdog timeout in seconds (default="
        __MODULE_STRING(WATCHDOG_TIMEOUT) ")");

// Watchdog
////////////////////////////////////////////////////////////////////////////////
static int rtd119x_wdt_start(struct watchdog_device *wdev)
{
	struct rtd119x_watchdog_device *data = watchdog_get_drvdata(wdev);
	u32 val;

	val = readl_relaxed(data->base + RTD119X_TCW_CTRL);
	val &= ~RTD119X_TCW_CTRL_WDEN_MASK;
	val |= RTD119X_TCW_CTRL_WDEN_ENABLED;
	writel(val, data->base + RTD119X_TCW_CTRL);

	return 0;
}

static int rtd119x_wdt_stop(struct watchdog_device *wdev)
{
	struct rtd119x_watchdog_device *data = watchdog_get_drvdata(wdev);
	u32 val;

	val = readl_relaxed(data->base + RTD119X_TCW_CTRL);
	val &= ~RTD119X_TCW_CTRL_WDEN_MASK;
	val |= RTD119X_TCW_CTRL_WDEN_DISABLED;
	writel(val, data->base + RTD119X_TCW_CTRL);

	return 0;
}

static int rtd119x_wdt_ping(struct watchdog_device *wdev)
{
	struct rtd119x_watchdog_device *data = watchdog_get_drvdata(wdev);

	writel_relaxed(RTD119X_TCW_CLR_WDCLR, data->base + RTD119X_TCW_CLR);

	return rtd119x_wdt_start(wdev);
}

static int rtd119x_wdt_set_count(struct watchdog_device *wdev, unsigned int count)
{
	struct rtd119x_watchdog_device *data = watchdog_get_drvdata(wdev);

	writel(count, data->base + RTD119X_TCW_COUNT);

	return rtd119x_wdt_start(wdev);
}

static int rtd119x_wdt_set_timeout(struct watchdog_device *wdev, unsigned int val)
{
	struct rtd119x_watchdog_device *data = watchdog_get_drvdata(wdev);

	writel(val * clk_get_rate(data->clk), data->base + RTD119X_TCW_TIMEOUT);

	data->wdt_dev.timeout = val;

	return 0;
}

static const struct watchdog_ops rtd119x_wdt_ops = {
	.owner = THIS_MODULE,
	.start		= rtd119x_wdt_start,
	.stop		= rtd119x_wdt_stop,
	.ping		= rtd119x_wdt_ping,
	.set_timeout	= rtd119x_wdt_set_timeout,
};

static const struct watchdog_info rtd119x_wdt_info = {
	.identity = "rtd119x-wdt",
	.options = WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT,
};

// Restart handler
////////////////////////////////////////////////////////////////////////////////
/* Watchdog pointer saved for restart handler */
static void __iomem *wdt_base;

static int rtd119x_restart_handler(struct notifier_block *this, unsigned long mode, void *cmd)
{
	/*
	 * Perform a hardware reset with the use of the Watchdog timer.
	 */
	writel(0x00800000, wdt_base + RTD119X_TCW_COUNT);
	writel(BIT(0), wdt_base + RTD119X_TCW_CLR);
	writel(0x00800000, wdt_base + RTD119X_TCW_TIMEOUT);
	writel(0x000000FF, wdt_base + RTD119X_TCW_CTRL);

	mdelay(2000);

	pr_emerg("%s: Unable to restart system\n", DEV_NAME);
	return NOTIFY_DONE;
}

static struct notifier_block rtd119x_restart_nb = {
	.notifier_call = rtd119x_restart_handler,
	.priority = 192,
};


// Module
////////////////////////////////////////////////////////////////////////////////
static int rtd119x_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rtd119x_watchdog_device *data;
	int ret;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		pr_err("%s: failed to allocate memory\n", DEV_NAME);
		return -ENOMEM;
	}

	data->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(data->base)) {
		pr_err("%s: failed to remap IO\n", DEV_NAME);
		return PTR_ERR(data->base);
	}

	data->clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(data->clk)) {
		pr_err("%s: failed to enable clock\n", DEV_NAME);
		return PTR_ERR(data->clk);
	}

	data->wdt_dev.info = &rtd119x_wdt_info;
	data->wdt_dev.ops = &rtd119x_wdt_ops;
	data->wdt_dev.timeout = WATCHDOG_TIMEOUT;
	data->wdt_dev.max_timeout = 0xffffffff / clk_get_rate(data->clk);
	data->wdt_dev.min_timeout = 1;
	data->wdt_dev.parent = dev;

	watchdog_set_nowayout(&data->wdt_dev, nowayout);
	watchdog_init_timeout(&data->wdt_dev, timeout, dev);

	//watchdog_stop_on_reboot(&data->wdt_dev);				// Need WDT for restart
	watchdog_set_drvdata(&data->wdt_dev, data);
	platform_set_drvdata(pdev, data);

	writel_relaxed(RTD119X_TCW_CLR_WDCLR, data->base + RTD119X_TCW_CLR);	// Ping
	//rtd119x_wdt_set_timeout(&data->wdt_dev, data->wdt_dev.timeout);
	//rtd119x_wdt_stop(&data->wdt_dev);

	ret = devm_watchdog_register_device(dev, &data->wdt_dev);
	if (ret) {
                pr_err("%s: failed to register watchdog\n", DEV_NAME);
		goto rtd119x_wdt_probe_finish;
	}

	/* Save watchdog data pointer for restart handler */
	wdt_base = data->base;

        ret = register_restart_handler(&rtd119x_restart_nb);
        if (ret)
                pr_warn("%s: failed to register restart handler\n", DEV_NAME);

	pr_info("%s: initialised watchdog", DEV_NAME);

rtd119x_wdt_probe_finish:
	return ret;
}

static void rtd119x_wdt_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdt_dev = platform_get_drvdata(pdev);
	struct rtd119x_watchdog_device *rtd119x_dev = watchdog_get_drvdata(wdt_dev);
	int ret;

	ret = unregister_restart_handler(&rtd119x_restart_nb);
	if (ret)
		pr_warn("%s: failed to unregister restart handler\n", DEV_NAME);

	watchdog_unregister_device(wdt_dev);

	pr_info("%s: removed watchdog", DEV_NAME);
}


static const struct of_device_id rtd119x_wdt_dt_ids[] = {
	 { .compatible = "realtek,rtd1295-watchdog" },
	 { }
};

static struct platform_driver rtd119x_wdt_driver = {
	.probe				= rtd119x_wdt_probe,
	.remove_new			= rtd119x_wdt_remove,
	.driver = {
		.name			= DEV_NAME,
		.of_match_table		= rtd119x_wdt_dt_ids,
	},
};


MODULE_DEVICE_TABLE(of, rtd119x_wdt_dt_ids);
module_platform_driver(rtd119x_wdt_driver);

MODULE_AUTHOR("Andreas Färber");
MODULE_DESCRIPTION("Realtek RTD129x watchdog");
MODULE_LICENSE("GPL v2");
