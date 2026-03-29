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
	.options = 0,
};

// Restart handler
////////////////////////////////////////////////////////////////////////////////
/* Watchdog pointer saved for restart handler */
static struct rtd119x_watchdog_device *rtd119x_wdt_dev;

static int rtd119x_restart_handler(struct notifier_block *this, unsigned long mode, void *cmd)
{
	/*
	 * Perform a hardware reset with the use of the Watchdog timer.
	 */
	rtd119x_wdt_set_count(&rtd119x_wdt_dev->wdt_dev, 0x00800000);
	rtd119x_wdt_ping(&rtd119x_wdt_dev->wdt_dev);
	rtd119x_wdt_set_timeout(&rtd119x_wdt_dev->wdt_dev, 0x00800000);
	rtd119x_wdt_start(&rtd119x_wdt_dev->wdt_dev);

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
	if (!data)
		return -ENOMEM;

	data->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(data->base))
		return PTR_ERR(data->base);

	data->clk = devm_clk_get_enabled(dev, NULL);
	if (IS_ERR(data->clk))
		return PTR_ERR(data->clk);

	data->wdt_dev.info = &rtd119x_wdt_info;
	data->wdt_dev.ops = &rtd119x_wdt_ops;
	data->wdt_dev.timeout = 120;
	data->wdt_dev.max_timeout = 0xffffffff / clk_get_rate(data->clk);
	data->wdt_dev.min_timeout = 1;
	data->wdt_dev.parent = dev;

	watchdog_stop_on_reboot(&data->wdt_dev);				// Need WDT for restart
	watchdog_set_drvdata(&data->wdt_dev, data);
	platform_set_drvdata(pdev, data);

	writel_relaxed(RTD119X_TCW_CLR_WDCLR, data->base + RTD119X_TCW_CLR);
	rtd119x_wdt_set_timeout(&data->wdt_dev, data->wdt_dev.timeout);
	rtd119x_wdt_stop(&data->wdt_dev);

	ret = devm_watchdog_register_device(dev, &data->wdt_dev);
	if (ret) {
                pr_err("%s: failed to register watchdog\n", DEV_NAME);
		goto rtd119x_wdt_probe_finish;
	}

	/* Save watchdog data pointer for restart handler */
	rtd119x_wdt_dev = data;

        ret = register_restart_handler(&rtd119x_restart_nb);
        if (ret)
                pr_warn("%s: failed to register restart handler\n", DEV_NAME);

	pr_info("%s: initialised watchdog", DEV_NAME);

rtd119x_wdt_probe_finish:
	return ret;
}


static const struct of_device_id rtd119x_wdt_dt_ids[] = {
	 { .compatible = "realtek,rtd1295-watchdog" },
	 { }
};

static struct platform_driver rtd119x_wdt_driver = {
	.probe = rtd119x_wdt_probe,
	.driver = {
		.name = DEV_NAME,
		.of_match_table	= rtd119x_wdt_dt_ids,
	},
};


MODULE_DEVICE_TABLE(of, rtd119x_wdt_dt_ids);
module_platform_driver(rtd119x_wdt_driver);

MODULE_AUTHOR("Andreas Färber");
MODULE_DESCRIPTION("Realtek RTD129x watchdog");
MODULE_LICENSE("GPL v2");
