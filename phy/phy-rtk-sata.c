/*
 * Realtek SATA PHY driver
 *
 * Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Updated 2026 - Celliwig <celliwig@nym.hush.com>
 */


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/reset.h>
#include <linux/phy/phy.h>
#include <linux/delay.h>

#include "rtk-chip.h"


#define RTK_SATA_PHY_NUMBER		2
#define DEFAULT_TXDRV_LEVEL		3
#define PHY_MAX_CLK			5
#define PHY_MAX_RST			5


struct rtk_phy_desc {
	unsigned int		index;
	unsigned int		param_size;
	unsigned int		*param_table;
	unsigned int		txdrv_size;
	unsigned int		*txdrv_table;
	unsigned int		ssc_en;
	unsigned int		speed;
	struct phy		*phy;
	struct reset_control	*rsts[PHY_MAX_RST];
};

struct rtk_phy_priv {
	void __iomem		*base;
	void __iomem		*sb2base;
	struct clk		*clks[PHY_MAX_CLK];
	struct reset_control	*rsts[PHY_MAX_RST];
	struct rtk_phy_desc	**phys;
	struct device		*dev;
	unsigned int		nphys;
	enum rtd_chip_id	chip_id;
	enum rtd_chip_revision	chip_revision;
};


static const unsigned int PHY_PARA_TABLE[] = {
	0x00001111, 0x00005111, 0x00009111,
	0x336a0511, 0x336a4511, 0x336a8511,
	0xE0700111, 0xE05C4111, 0xE04A8111,
	0x00150611, 0x00154611, 0x00158611,
	0xC6000A11, 0xC6004A11, 0xC6008A11,
	0x70000211, 0x70004211, 0x70008211,
	0xC6600A11, 0xC6604A11, 0xC6608A11,
	0x20041911, 0x20045911, 0x20049911,
	0x94aa2011, 0x94aa6011, 0x94aaa011,
	0x17171511, 0x17175511, 0x17179511,
	0x07701611, 0x07705611, 0x07709611,
	0x40002a11, 0x40006a11, 0x4000aa11,
	0x29001011, 0x29005011, 0x29009011,
	0x40000C11, 0x40004C11, 0x40008C11,
	0x00271711, 0x00275711, 0x00279711,
};

static const unsigned int TX_DRV_TABLE[][6] = {
	{ 0x94a52011, 0x94a56011, 0x94a5a011, 0x385a2111, 0x385a6111, 0x385aa111 },		// Max 500mV
	{ 0x94a62011, 0x94a66011, 0x94a6a011, 0x486a2111, 0x486a6111, 0x486aa111 },		// Max 573mV
	{ 0x94a72011, 0x94a76011, 0x94a7a011, 0x587a2111, 0x587a6111, 0x587aa111 },		// Max 683mV
	{ 0x94aa2011, 0x94aa6011, 0x94aaa011, 0x88aa2111, 0x88aa6111, 0x88aaa111 },		// Max 786mV
	{ 0x94aa2011, 0x94aa6011, 0x94aaa011, 0x88aa2111, 0x88aa6111, 0x88aaa111 },		// Max 786mV
	{ 0x94aa2011, 0x94aa6011, 0x94aaa011, 0x88aa2111, 0x88aa6111, 0x88aaa111 },		// Max 786mV
	{ 0x94aa2011, 0x94aa6011, 0x94aaa011, 0x88aa2111, 0x88aa6111, 0x88aaa111 },		// Max 786mV
	{ 0x94aa2011, 0x94aa6011, 0x94aaa011, 0x88aa2111, 0x88aa6111, 0x88aaa111 },		// Max 786mV
};

static const unsigned int PHY_SENSITIVITY[][6] = {
	{ 0x72100911, 0x72104911, 0x72108911, 0x27730311, 0x27684311, 0x27688311 },
	{ 0x42100911, 0x42104911, 0x42108911, 0x276f0311, 0x276d4311, 0x276d8311 },
};

static const unsigned int SSCEN_SET_TABLE[] = {
	0x738E0411, 0x738E0411, 0x738E8411,
	0x35910811, 0x35914811, 0x35918811,
	0x02342711, 0x02346711, 0x0234a711,
};

static const unsigned int SSCDIS_SET_TABLE[] = {
	0x538E0411, 0x538E4411, 0x538E8411,
};

static const unsigned int PHY_CLOCK_DELAY[] = {
	0x7c002a11, 0x7c006a11, 0x7c00aa11,
};


static void writel_delay(unsigned int value, void __iomem *address)
{
	writel(value, address);
	mdelay(1);
}

static struct phy *rtk_sata_phy_xlate(struct device *dev,
					const struct of_phandle_args *args)
{
	struct rtk_phy_priv *priv = dev_get_drvdata(dev);
	int i;

	for (i = 0; i < priv->nphys; i++) {
		if (priv->phys[i]->index == args->args[0])
			break;
	}

	if (i == priv->nphys) {
		pr_warn("%s: failed to find PHY[%u]\n", __func__, args->args[0]);
		return ERR_PTR(-ENODEV);
	}

	return priv->phys[i]->phy;
}

static int rtk_sata_phy_init(struct phy *phy)
{
	struct rtk_phy_desc *desc = phy_get_drvdata(phy);
	struct rtk_phy_priv *priv = dev_get_drvdata(phy->dev.parent);
	void __iomem *base = priv->base;
	void __iomem *sb2base = priv->sb2base;
	unsigned int *table;
	unsigned int reg;
	unsigned int i;
	int size;

	reg = readl(sb2base);
	if (desc->index == 0) {
		reg = reg | BIT(0) | BIT(2) | BIT(4) | BIT(8);
	} else {
		reg = reg | BIT(1) | BIT(3) | BIT(5);
	}
	writel(reg, sb2base);

	writel_delay(desc->index, base + 4);

	for (i = 0; i < desc->param_size; i++) {
		writel_delay(desc->param_table[i], base);
	}

	for (i = 0; i < desc->txdrv_size; i++) {
		writel_delay(desc->txdrv_table[i], base);
	}

	if (desc->ssc_en) {
		table = (unsigned int *) &SSCEN_SET_TABLE;
		size = sizeof(SSCEN_SET_TABLE) / sizeof(unsigned int);
	} else {
		table = (unsigned int *) &SSCDIS_SET_TABLE;
		size = sizeof(SSCDIS_SET_TABLE) / sizeof(unsigned int);
	}

	for (i = 0; i < size; i++) {
		writel_delay(table[i], base);
	}

	if ((priv->chip_id & 0xFFF0) == CHIP_ID_RTD129X) {
		if ((priv->chip_revision == RTD_CHIP_A00) || (priv->chip_revision == RTD_CHIP_A01)) {
			table = (unsigned int *) PHY_SENSITIVITY[0];
		} else if (priv->chip_revision == RTD_CHIP_B00) {
			table = (unsigned int *) PHY_SENSITIVITY[1];
		} else {
			table = (unsigned int *) PHY_SENSITIVITY[1];
		}
		size = sizeof(*PHY_SENSITIVITY) / sizeof(unsigned int);
		for (i = 0; i < size; i++) {
			writel_delay(table[i], base);
		}
	}

	if (((priv->chip_id & 0xFFF0) == CHIP_ID_RTD129X) && (priv->chip_revision == RTD_CHIP_B00)) {
		table = (unsigned int *) &PHY_CLOCK_DELAY;
		size = sizeof(PHY_CLOCK_DELAY) / sizeof(unsigned int);
		for (i = 0; i < size; i++) {
			writel_delay(table[i], base);
		}
	}

	if (desc->speed == 0) {
		writel_delay(0xA, base + 8);
	} else if (desc->speed == 2) {
		writel_delay(0x5, base + 8);
	} else if (desc->speed == 1) {
		writel_delay(0x0, base + 8);
	}

	dev_info(priv->dev, "initialised PHY[%d]\n", desc->index);

	return 0;
}

static int rtk_sata_phy_power_on(struct phy *phy)
{
	struct rtk_phy_desc *desc = phy_get_drvdata(phy);
	struct rtk_phy_priv *priv = dev_get_drvdata(phy->dev.parent);
	unsigned int i;

	dev_info(priv->dev, "power on PHY[%d]\n", desc->index);

	for (i = 0; i < PHY_MAX_RST; i++) {
		if (desc->rsts[i] != NULL) {
			reset_control_deassert(desc->rsts[i]);
		} else {
			break;
		}
	}

	return 0;
}

static int rtk_sata_phy_power_off(struct phy *phy)
{
	struct rtk_phy_desc *desc = phy_get_drvdata(phy);
	struct rtk_phy_priv *priv = dev_get_drvdata(phy->dev.parent);
	unsigned int i;

	dev_info(priv->dev, "power off PHY[%d]\n", desc->index);

	for (i = 0;  i < PHY_MAX_RST; i++) {
		if (desc->rsts[i] != NULL) {
			reset_control_assert(desc->rsts[i]);
		} else {
			break;
		}
	}

	return 0;
}

static struct phy_ops rtk_sata_phy_ops = {
	.init		= rtk_sata_phy_init,
	.power_on	= rtk_sata_phy_power_on,
	.power_off	= rtk_sata_phy_power_off,
	.owner		= THIS_MODULE,
};


static int rtk_sata_phy_get_parameter(struct device *dev, struct device_node *node,
				struct rtk_phy_desc *desc)
{
	const void *prop;
	unsigned int drv_level;
	unsigned int size;
	unsigned int *table;

	/* Get PHY config parameters from devicetree */
	prop = of_get_property(node, "phy-param", NULL);
	/* Get size of PHY config parameters or use defaults */
	if (!prop) {
		size = sizeof(PHY_PARA_TABLE) / sizeof(unsigned int);
	} else {
		size = of_property_count_u32_elems(node, "phy-param");
	}

	/* Allocate memory for PHY config parameters store */
	table = devm_kzalloc(dev, sizeof(unsigned int) * size, GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	/* Copy PHY config parameters to store from devicetree, or defaults */
	if (!prop) {
		memcpy(table, &PHY_PARA_TABLE, sizeof(PHY_PARA_TABLE));
	} else {
		of_property_read_u32_array(node, "phy-param", table, size);
	}

	desc->param_size = size;
	desc->param_table = table;

	/* Get Tx drive paramters from devicetree */
	prop = of_get_property(node, "tx-driving-tbl", NULL);
	/* Check whether Tx drive parameters ae in devicetree */
	if (!prop) {
		/* Tx drive parameters not in devicetree */
		drv_level = DEFAULT_TXDRV_LEVEL;
		of_property_read_u32(node, "tx-driving", &drv_level);

		if (drv_level >= (sizeof(TX_DRV_TABLE) / sizeof(*TX_DRV_TABLE))) {
			drv_level = DEFAULT_TXDRV_LEVEL;
		}

		size = sizeof(*TX_DRV_TABLE) / sizeof(unsigned int);
	} else {
		size = of_property_count_u32_elems(node, "tx-driving-tbl");
	}

	/* Allocate memory for PHY Tx drive store */
	table = devm_kzalloc(dev, sizeof(unsigned int) * size, GFP_KERNEL);
	if (!table) {
		return -ENOMEM;
	}

	/* Copy PHY Tx drive parameters to store from devicetree, or defaults */
	if (!prop) {
		memcpy(table, TX_DRV_TABLE[drv_level], sizeof(*TX_DRV_TABLE));
	} else {
		of_property_read_u32_array(node, "tx-driving-tbl", table, size);
	}

	desc->txdrv_size = size;
	desc->txdrv_table = table;

	of_property_read_u32(node, "spread-spectrum", &desc->ssc_en);
	of_property_read_u32(node, "speed-limit", &desc->speed);

	return 0;
}

static int rtk_sata_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *child;
	struct phy *phy;
	struct rtk_phy_priv *priv;
	struct phy_provider *phy_provider;
	struct clk *clk;
	struct reset_control *rst;

	unsigned int phy_id;
	unsigned int i = 0, cnt = 0;
	int ret;

	/* Allocate storage for private data */
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		goto rtk_sata_phy_probe_memalloc_fail;

	/* Get SOC info */
	priv->chip_id = get_rtd_chip_id();
	priv->chip_revision = get_rtd_chip_revision();

	/* Get I/O addresses */
	priv->base = of_iomap(dev->of_node, 0);
	if (!priv->base)
		goto rtk_sata_phy_probe_dtb_fail;
	priv->sb2base = of_iomap(dev->of_node, 1);
	if (!priv->base)
		goto rtk_sata_phy_probe_dtb_fail;

	/* Get clocks */
	for (i = 0; i < PHY_MAX_CLK; i++) {
		clk = of_clk_get(dev->of_node, i);
		if (IS_ERR(clk))
			break;
		priv->clks[i] = clk;
		clk_prepare_enable(clk);
	}

	/* Get resets */
	for (i=0; i<PHY_MAX_RST; i++) {
		rst = of_reset_control_get_by_index(dev->of_node, i);
		if (IS_ERR(rst))
			break;
		priv->rsts[i] = rst;
		if (priv->chip_id == CHIP_ID_RTD1619)
			reset_control_deassert(rst);
	}

	/* Get PHY count */
	priv->nphys = of_get_child_count(dev->of_node);
	if (priv->nphys == 0 || priv->nphys > RTK_SATA_PHY_NUMBER)
		return -ENODEV;

	priv->dev = dev;

	/* Allocate storage for PHYs private data*/
	priv->phys = devm_kcalloc(dev, priv->nphys, sizeof(*priv->phys), GFP_KERNEL);
	if (!priv->phys)
		goto rtk_sata_phy_probe_memalloc_fail;

	dev_set_drvdata(dev, priv);

	/* Setup each PHY */
	for_each_available_child_of_node(dev->of_node, child) {
		struct rtk_phy_desc *phy_desc;

		phy_desc = devm_kzalloc(dev, sizeof(*phy_desc), GFP_KERNEL);
		if (!phy_desc)
			goto rtk_sata_phy_probe_memalloc_fail;

		if (of_property_read_u32(child, "reg", &phy_id))
			goto rtk_sata_phy_probe_dtb_fail;

		if (phy_id >= RTK_SATA_PHY_NUMBER)
			goto rtk_sata_phy_probe_dtb_fail;

		for (i=0; i<PHY_MAX_RST; i++) {
			rst = of_reset_control_get_by_index(child, i);
			if (IS_ERR(rst))
				break;
			phy_desc->rsts[i] = rst;
			if (priv->chip_id == CHIP_ID_RTD1619)
				reset_control_deassert(rst);
		}

		ret = rtk_sata_phy_get_parameter(dev, child, phy_desc);
		if (ret < 0)
			goto rtk_sata_phy_probe_memalloc_fail;

		phy = devm_phy_create(dev, NULL, &rtk_sata_phy_ops);
		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy %d\n", phy_id);
			return PTR_ERR(phy);
		}

		phy_desc->phy = phy;
		phy_desc->index = phy_id;
		phy_set_drvdata(phy, phy_desc);
		priv->phys[cnt++] = phy_desc;
	}

	phy_provider = devm_of_phy_provider_register(dev, rtk_sata_phy_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);

rtk_sata_phy_probe_dtb_fail:
	dev_err(dev, "[SATA_PHY] get resource from dtb fail\n");
	return -EINVAL;

rtk_sata_phy_probe_memalloc_fail:
	dev_err(dev, "[SATA_PHY] can't allocate memory\n");
	return -ENOMEM;
}


static const struct of_device_id rtk_sata_phy_of_match[] = {
	{ .compatible = "Realtek,rtk-sata-phy" },
	{ },
};

static struct platform_driver rtk_sata_phy_driver = {
	.driver		= {
		.name			= "phy-rtk-sata",
		.of_match_table		= rtk_sata_phy_of_match,
	},
	.probe		= rtk_sata_phy_probe,
};
module_platform_driver(rtk_sata_phy_driver);


MODULE_DESCRIPTION("Realtek SATA PHY driver");
MODULE_AUTHOR("Simon HSU <simon_hsu@realtek.com>");
MODULE_LICENSE("GPL v2");
