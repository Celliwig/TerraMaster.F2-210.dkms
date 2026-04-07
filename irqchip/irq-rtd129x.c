/*
 * irq-rtd129x.c - RTK irq mux driver
 *
 * Copyright (C) 2017 Realtek Semiconductor Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Updated 2026 - Celliwig <celliwig@nym.hush.com>
 */

#include <linux/err.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <asm/hardirq.h>
#include <asm/exception.h>
#include <asm/irq.h>

#include "irq-rtd129x.h"

#define DEV_NAME "RTK_IRQ_MUX"

#ifdef CONFIG_RTK_XEN_SUPPORT
#define IRDA_HW_IRQ     37
#endif

static DEFINE_SPINLOCK(irq_mux_lock);

struct rtk_irqmux_data {
	void __iomem		*base;
	unsigned char		index;
	unsigned int		irq;
	unsigned int		irq_idx_offset;
	u32			reg_offset_status;
	u32			reg_offset_enabled;
};

static struct irq_domain *rtk_domain;

#define DEFAULT_UART0_IRQ	false
static bool uart0_irq_disable = DEFAULT_UART0_IRQ;
module_param(uart0_irq_disable, bool, 0);
MODULE_PARM_DESC(uart0_irq_disable, "Forciably disable UART0 [console] IRQ (default=" __MODULE_STRING(DEFAULT_UART0_IRQ) ")");


static void rtkmux_ack_irq(struct irq_data *data)
{
	struct rtk_irqmux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 status_offset;

	mux_data += (data->hwirq / IRQ_INMUX);						// Select mux data structure from array
	base = mux_data->base;
	status_offset = mux_data->reg_offset_status;

	/* Celliwig: didn't originally have spinlock, is this needed to protect for SMP??? */
	spin_lock(&irq_mux_lock);
	__raw_writel(BIT(data->hwirq % IRQ_INMUX), base + status_offset);		// Clear flag
	spin_unlock(&irq_mux_lock);
}

static void rtkmux_unmask_irq(struct irq_data *data)
{
	struct rtk_irqmux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 enable_offset;
	u8 enable_irq_bit;

	mux_data += (data->hwirq / IRQ_INMUX);						// Select mux data structure from array
	base = mux_data->base;
	enable_offset = mux_data->reg_offset_enabled;

	/* Check requested IRQ bit against RTK MUX table */
	enable_irq_bit = rtk_irq_map_tbl[mux_data->index][data->hwirq % IRQ_INMUX];

	if ((enable_irq_bit != MISC_INT_RVD) && (enable_irq_bit != MISC_INT_FAIL)) {

		/* Celliwig: didn't originally have spinlock, is this needed to protect for SMP??? */
		spin_lock(&irq_mux_lock);
		__raw_writel((__raw_readl(base + enable_offset) | BIT(enable_irq_bit)), base + enable_offset);
		spin_unlock(&irq_mux_lock);

	} else if (enable_irq_bit == MISC_INT_FAIL) {
		pr_err("[%s] Enable irq(%lu) fail\n", DEV_NAME, data->hwirq);
	}
}

static void rtkmux_mask_irq(struct irq_data *data)
{
	struct rtk_irqmux_data *mux_data = irq_data_get_irq_chip_data(data);
	void __iomem *base;
	u32 enable_offset;
	u8 enable_irq_bit;

	// Celliwig: Rtk disabled IRDA interrupts for Xen guests
	//if (data->hwirq == IRDA_HW_IRQ && !xen_initial_domain()) {
	//	pr_info("%s:Skip IRDA on Guest Domain\n", __func__);
	//	return;
	//}

	mux_data += (data->hwirq / IRQ_INMUX);						// Select mux data structure from array
	base = mux_data->base;
	enable_offset = mux_data->reg_offset_enabled;

	/* Check requested IRQ bit against RTK MUX table */
	enable_irq_bit = rtk_irq_map_tbl[mux_data->index][data->hwirq % IRQ_INMUX];

	if ((enable_irq_bit != MISC_INT_RVD) && (enable_irq_bit != MISC_INT_FAIL)) {

		spin_lock(&irq_mux_lock);
		__raw_writel((__raw_readl(base + enable_offset) & ~BIT(enable_irq_bit)), base + enable_offset);
		spin_unlock(&irq_mux_lock);

	} else if (enable_irq_bit == MISC_INT_FAIL) {
		pr_err("[%s] Disable irq(%lu) fail\n", DEV_NAME, data->hwirq);
	}
}

#ifdef CONFIG_SMP
static int __maybe_unused rtkmux_set_affinity(struct irq_data *d,
	const struct cpumask *mask_val,
	bool force)
{
	struct rtk_irqmux_data *mux_data = irq_data_get_irq_chip_data(d);
	struct irq_chip *chip = irq_get_chip(mux_data->irq);
	struct irq_data *data = irq_get_irq_data(mux_data->irq);

	if (chip && chip->irq_set_affinity)
		return chip->irq_set_affinity(data, mask_val, force);
	else
		return -EINVAL;

}
#endif

static struct irq_chip rtkmux_chip = {
	.name = DEV_NAME,
	.irq_ack = rtkmux_ack_irq,
	.irq_mask = rtkmux_mask_irq,
	.irq_unmask = rtkmux_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity = rtkmux_set_affinity,
#endif
};

static void rtkmux_irq_handle(struct irq_desc *desc)
{
	struct rtk_irqmux_data *mux_data = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	//struct irq_data *data = irq_desc_get_irq_data(desc);
	//unsigned int hwirq = data->hwirq;
	unsigned int irq = irq_desc_get_irq(desc);
	unsigned int irq_ref, irq_idx;
	unsigned int enable_offset, status_offset;
	unsigned int status_current, status_new;
	unsigned int enable_current;
	int ret;
	u8 enable_irq_bit;

	/* Used to watch for stuck bits */
	static u32 count;

	status_offset = mux_data->reg_offset_status;
	enable_offset = mux_data->reg_offset_enabled;

	chained_irq_enter(chip, desc);

	/* Get current IRQ MUX state */
	spin_lock(&irq_mux_lock);
	enable_current = __raw_readl(mux_data->base + enable_offset);
	status_current = __raw_readl(mux_data->base + status_offset);
	spin_unlock(&irq_mux_lock);

	/* Iterate through IRQ MUX bitx */
	for (unsigned int i = 0 ; i < IRQ_INMUX ; i++) {
		/* Check whether IRQ status bit set */
		if (status_current & BIT(i)) {
			/* Check requested IRQ bit against RTK MUX table */
			enable_irq_bit = rtk_irq_map_tbl[mux_data->index][i];
			irq_idx = mux_data->irq_idx_offset + i;

			if ((enable_irq_bit < IRQ_INMUX) &&
				(enable_current & BIT(enable_irq_bit))) {		// IRQ bit is valid, and enabled

				irq_ref = irq_find_mapping(rtk_domain, irq_idx);
				ret = generic_handle_irq(irq_ref);			// Fire interrupt handler

				if (ret != 0) {
					pr_err("[%s] irq(%u) desc is not found"
						"(st:0x%08x en:0x%08x)\n",
						DEV_NAME,
						irq_idx,
						status_current,
						enable_current);
				}
			} else if (enable_irq_bit == MISC_INT_RVD) {			// IRQ bit is reserved

				irq_ref = irq_find_mapping(rtk_domain, irq_idx);
				ret = generic_handle_irq(irq_ref);			// Fire interrupt handler

				if (ret != 0) {
					pr_err("[%s] irq(%u) desc is not found"
						"(st:0x%08x en:0x%08x)\n",
						DEV_NAME,
						irq_idx,
						status_current,
						enable_current);
				}
			} else {
				pr_err("[%s] irq(%u) should not happen"
					"(st:0x%08x en:0x%08x)\n",
					DEV_NAME,
					irq_idx,
					status_current,
					enable_current);
			}
		}
	}

	/* As a transmission interface, SPI wont do too much here */
	if ((irq == 1) && (status_current | 0x08000000))
		goto out;

	/* Get new IRQ MUX state */
	spin_lock(&irq_mux_lock);
	status_new = __raw_readl(mux_data->base + status_offset);
	spin_unlock(&irq_mux_lock);

	/* Check for unacknowledged IRQs */
	if (status_new == status_current) {
		if (count > 1) {
			pr_err("[%s] (%u) %s irq status has not changed, clear it! (st:0x%08x en:0x%08x)\n",
				DEV_NAME,
				irq,
				mux_data->index ? "ISO" : "MISC",
				status_current,
				enable_current);
		} else {
			count++;
		}

		/* Forcibly clear first IRQ bit in status */
		spin_lock(&irq_mux_lock);
		__raw_writel(BIT(__ffs(status_current)), mux_data->base + status_offset);
		spin_unlock(&irq_mux_lock);

	} else {
		count = 0;
	}
out:
	chained_irq_exit(chip, desc);
}

static int rtkmux_irq_domain_xlate(struct irq_domain *d,
	struct device_node *controller,
	const u32 *intspec,
	unsigned int intsize,
	unsigned long *out_hwirq,
	unsigned int *out_type)
{
	if (controller != irq_domain_get_of_node(d))
		return -EINVAL;

	if (intsize < 2)
		return -EINVAL;

	*out_hwirq = intspec[0] * IRQ_INMUX + intspec[1];
	*out_type = 0;

	pr_debug("%s: xlate IRQ: isp0: %u, isp1: %u, isize: %u, hwirq: %lu\n", __func__, intspec[0], intspec[1], intsize, *out_hwirq);

	return 0;
}

static int rtkmux_irq_domain_map(struct irq_domain *d,
						unsigned int irq,
						irq_hw_number_t hw)
{
	struct rtk_irqmux_data *data = d->host_data;

	irq_set_chip_and_handler(irq, &rtkmux_chip, handle_level_irq);
	irq_set_chip_data(irq, data);
	irq_set_probe(irq);

	pr_debug("%s: mapped IRQ: id: %u, irq: %u, hwirq: %lu\n", __func__, data->index, irq, hw);

	return 0;
}

static const struct irq_domain_ops mux_irq_domain_ops = {
	.xlate = rtkmux_irq_domain_xlate,
	.map = rtkmux_irq_domain_map,
};

static void __init mux_init_each(struct rtk_irqmux_data *mux_data,
	void __iomem *base, u32 irq,
	u32 status_offset, u32 enable_offset, int mux_index)
{
	mux_data->base = base;
	mux_data->index = mux_index;
	mux_data->irq = irq;
	mux_data->irq_idx_offset = mux_index * IRQ_INMUX;
	mux_data->reg_offset_status = status_offset;
	mux_data->reg_offset_enabled = enable_offset;


	/* Forciably disable UART0 (console) IRQ, required if using polling */
	if ((mux_index == 1) && uart0_irq_disable) {
		/* Disable UART0 IRQ */
		__raw_writel((__raw_readl(base + enable_offset) & ~BIT(2)), base + enable_offset);
		/* Acknowledge IRQ in status register */
		__raw_writel(BIT(2), base + status_offset);
	}

	irq_set_chained_handler_and_data(irq, rtkmux_irq_handle, mux_data);

	pr_info("%s: registered interrupt MUX: index: %u, irq: %u, irq_idx_offset: %u, reg_st: %u, reg_en: %u\n",
		DEV_NAME,
		mux_data->index,
		mux_data->irq,
		mux_data->irq_idx_offset,
		mux_data->reg_offset_status,
		mux_data->reg_offset_enabled);
}

static int __init mux_of_init(struct device_node *np, struct device_node *parent)
{
	struct rtk_irqmux_data *mux_data;
	void __iomem *base;
	u32 irq;
	u32 mux_count = 1;
	u32 status_offset, enable_offset;

	pr_info("%s: initialising interrupt controller\n", DEV_NAME);

	if (WARN_ON(!np))
		return -ENODEV;

	if (of_property_read_u32(np, "Realtek,mux-nr", &mux_count))
		pr_err("[%s] can not specified mux number\n", DEV_NAME);

	mux_data = kcalloc(mux_count, sizeof(*mux_data), GFP_KERNEL);

	/*
	 * TODO : temporarily define the first irq number
	 * in this mux is 160, end in 160+64
	 */
	rtk_domain = irq_domain_add_simple(np,
		(mux_count * IRQ_INMUX),
		160,
		&mux_irq_domain_ops,
		mux_data);

	if (!rtk_domain)
		pr_warn("[%s] IRQ domain init failed\n", DEV_NAME);

	for (unsigned int i = 0; i < mux_count; i++) {
		base = of_iomap(np, i);

		if (!base)
			pr_warn("[%s] unable to map IRQ base registers\n", DEV_NAME);

		irq = irq_of_parse_and_map(np, i);

		if (!irq)
			pr_warn("[%s] can not map IRQ\n", DEV_NAME);

		of_property_read_u32_index(np, "intr-status", i, &status_offset);
		of_property_read_u32_index(np, "intr-en", i, &enable_offset);

		mux_init_each(mux_data, base, irq, status_offset, enable_offset, i);

		mux_data++;
	}

	return 0;
}

IRQCHIP_PLATFORM_DRIVER_BEGIN(rtk_irq_mux)
IRQCHIP_MATCH("Realtek,rtk-irq-mux", mux_of_init)
IRQCHIP_PLATFORM_DRIVER_END(rtk_irq_mux)

MODULE_AUTHOR("Realtek Semiconductor Corporation");
MODULE_DESCRIPTION("Realtek RTD129x interrupt controller");
MODULE_LICENSE("GPL v2");
