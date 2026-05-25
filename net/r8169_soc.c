// SPDX-License-Identifier: GPL-2.0-only
/*
 * r8169.c: RealTek 8169/8168/8101 ethernet driver - SOC specific code.
 *
 * Copyright (c) 2002 ShuChen <shuchen@realtek.com.tw>
 * Copyright (c) 2003 - 2007 Francois Romieu <romieu@fr.zoreil.com>
 * Copyright (c) a lot of people too. Please respect their work.
 *
 * See MAINTAINERS file for support contact information.
 *
 * Updated 2026 - Celliwig <celliwig@nym.hush.com>
 */


/* Taken from the function 'rtl8168g_2_hw_mac_mcu_patch' in r8169soc.c */
void r8169_hw_phy_config_soc(struct rtl8169_private *tp, struct phy_device *phydev,
			 enum mac_version ver)
{
//	#if defined(CONFIG_ARCH_RTD129x)
	unsigned int tmp;
//	unsigned int cpu_revision;
//	void __iomem *rgmii0_addr, *tmp_addr;
//	#endif /* CONFIG_ARCH_RTD129x */

//	MDIO_LOCK;
//	/* power down PHY */
//	rtl_writephy(tp, 0x1f, 0x0000);
//	rtl_patchphy(tp, MII_BMCR, BMCR_PDOWN);
//	MDIO_UNLOCK;

//	#if defined(CONFIG_ARCH_RTD129x)
//	// mac mcu patch from Tenno to support EEE/EEE+
//	cpu_revision = get_rtd129x_cpu_revision();
//	if ((cpu_revision == RTD129x_CHIP_REVISION_A00) ||
//		(cpu_revision == RTD129x_CHIP_REVISION_A01) ||
//		(cpu_revision >= RTD129x_CHIP_REVISION_B00)) {
//		// disable break point
//		r8169soc_ocp_reg_write(tp, 0xfc28, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc2a, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc2c, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc2e, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc30, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc32, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc34, 0);
//		r8169soc_ocp_reg_write(tp, 0xfc36, 0);
//		mdelay(3);
//
//		// disable base address
//		r8169soc_ocp_reg_write(tp, 0xfc26, 0);
//
//		// patch code
//		r8169soc_ocp_reg_write(tp, 0xf800, 0xE008);
//		r8169soc_ocp_reg_write(tp, 0xf802, 0xE012);
//		r8169soc_ocp_reg_write(tp, 0xf804, 0xE044);
//		r8169soc_ocp_reg_write(tp, 0xf806, 0xE046);
//		r8169soc_ocp_reg_write(tp, 0xf808, 0xE048);
//		r8169soc_ocp_reg_write(tp, 0xf80a, 0xE04A);
//		r8169soc_ocp_reg_write(tp, 0xf80c, 0xE04C);
//		r8169soc_ocp_reg_write(tp, 0xf80e, 0xE04E);
//		r8169soc_ocp_reg_write(tp, 0xf810, 0x44E3);
//		r8169soc_ocp_reg_write(tp, 0xf812, 0xC708);
//		r8169soc_ocp_reg_write(tp, 0xf814, 0x75E0);
//		r8169soc_ocp_reg_write(tp, 0xf816, 0x485D);
//		r8169soc_ocp_reg_write(tp, 0xf818, 0x9DE0);
//		r8169soc_ocp_reg_write(tp, 0xf81a, 0xC705);
//		r8169soc_ocp_reg_write(tp, 0xf81c, 0xC502);
//		r8169soc_ocp_reg_write(tp, 0xf81e, 0xBD00);
//		r8169soc_ocp_reg_write(tp, 0xf820, 0x01EE);
//		r8169soc_ocp_reg_write(tp, 0xf822, 0xE85A);
//		r8169soc_ocp_reg_write(tp, 0xf824, 0xE000);
//		r8169soc_ocp_reg_write(tp, 0xf826, 0xC72D);
//		r8169soc_ocp_reg_write(tp, 0xf828, 0x76E0);
//		r8169soc_ocp_reg_write(tp, 0xf82a, 0x49ED);
//		r8169soc_ocp_reg_write(tp, 0xf82c, 0xF026);
//		r8169soc_ocp_reg_write(tp, 0xf82e, 0xC02A);
//		r8169soc_ocp_reg_write(tp, 0xf830, 0x7400);
//		r8169soc_ocp_reg_write(tp, 0xf832, 0xC526);
//		r8169soc_ocp_reg_write(tp, 0xf834, 0xC228);
//		r8169soc_ocp_reg_write(tp, 0xf836, 0x9AA0);
//		r8169soc_ocp_reg_write(tp, 0xf838, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf83a, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf83c, 0xF11E);
//		r8169soc_ocp_reg_write(tp, 0xf83e, 0xC324);
//		r8169soc_ocp_reg_write(tp, 0xf840, 0x9BA2);
//		r8169soc_ocp_reg_write(tp, 0xf842, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf844, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf846, 0xF0FE);
//		r8169soc_ocp_reg_write(tp, 0xf848, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf84a, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf84c, 0xF1FE);
//		r8169soc_ocp_reg_write(tp, 0xf84e, 0x1A02);
//		r8169soc_ocp_reg_write(tp, 0xf850, 0x49C9);
//		r8169soc_ocp_reg_write(tp, 0xf852, 0xF003);
//		r8169soc_ocp_reg_write(tp, 0xf854, 0x4821);
//		r8169soc_ocp_reg_write(tp, 0xf856, 0xE002);
//		r8169soc_ocp_reg_write(tp, 0xf858, 0x48A1);
//		r8169soc_ocp_reg_write(tp, 0xf85a, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf85c, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf85e, 0xF10D);
//		r8169soc_ocp_reg_write(tp, 0xf860, 0xC313);
//		r8169soc_ocp_reg_write(tp, 0xf862, 0x9AA0);
//		r8169soc_ocp_reg_write(tp, 0xf864, 0xC312);
//		r8169soc_ocp_reg_write(tp, 0xf866, 0x9BA2);
//		r8169soc_ocp_reg_write(tp, 0xf868, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf86a, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf86c, 0xF0FE);
//		r8169soc_ocp_reg_write(tp, 0xf86e, 0x73A2);
//		r8169soc_ocp_reg_write(tp, 0xf870, 0x49BE);
//		r8169soc_ocp_reg_write(tp, 0xf872, 0xF1FE);
//		r8169soc_ocp_reg_write(tp, 0xf874, 0x48ED);
//		r8169soc_ocp_reg_write(tp, 0xf876, 0x9EE0);
//		r8169soc_ocp_reg_write(tp, 0xf878, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf87a, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf87c, 0x0532);
//		r8169soc_ocp_reg_write(tp, 0xf87e, 0xDE00);
//		r8169soc_ocp_reg_write(tp, 0xf880, 0xE85A);
//		r8169soc_ocp_reg_write(tp, 0xf882, 0xE086);
//		r8169soc_ocp_reg_write(tp, 0xf884, 0x0A44);
//		r8169soc_ocp_reg_write(tp, 0xf886, 0x801F);
//		r8169soc_ocp_reg_write(tp, 0xf888, 0x8015);
//		r8169soc_ocp_reg_write(tp, 0xf88a, 0x0015);
//		r8169soc_ocp_reg_write(tp, 0xf88c, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf88e, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf890, 0x0000);
//		r8169soc_ocp_reg_write(tp, 0xf892, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf894, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf896, 0x0000);
//		r8169soc_ocp_reg_write(tp, 0xf898, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf89a, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf89c, 0x0000);
//		r8169soc_ocp_reg_write(tp, 0xf89e, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf8a0, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf8a2, 0x0000);
//		r8169soc_ocp_reg_write(tp, 0xf8a4, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf8a6, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf8a8, 0x0000);
//		r8169soc_ocp_reg_write(tp, 0xf8aa, 0xC602);
//		r8169soc_ocp_reg_write(tp, 0xf8ac, 0xBE00);
//		r8169soc_ocp_reg_write(tp, 0xf8ae, 0x0000);
//
//		// enable base address
//		r8169soc_ocp_reg_write(tp, 0xfc26, 0x8000);
//
//		// enable breakpoint
//		r8169soc_ocp_reg_write(tp, 0xfc28, 0x01ED);
//		r8169soc_ocp_reg_write(tp, 0xfc2a, 0x0531);
//
//		if (tp->eee_enable) {
//			/* EEE MAC mode */
//			r8169soc_ocp_reg_write(tp, 0xe040, (r8169soc_ocp_reg_read(tp, 0xe040) | (BIT(1) | BIT(0))));
//			/* EEE+ MAC mode */
//			r8169soc_ocp_reg_write(tp, 0xe080, (r8169soc_ocp_reg_read(tp, 0xe080) | BIT(1)));
//		}
//	}

//	if ((cpu_revision >= RTD129x_CHIP_REVISION_B00)) {
//		if (tp->output_mode == OUTPUT_EMBEDDED_PHY) {

			tmp = (r8168_mac_ocp_read(tp, 0xea34) & ~(BIT(0) | BIT(1))) & 0xffff;
			tmp |= BIT(1);						// set MII output
			r8168_mac_ocp_write(tp, 0xea34, tmp);

//		} else { /* RGMII */
//			if (tp->output_mode == OUTPUT_RGMII_TO_PHY) {
//				//# ETN spec, MDC freq=2.5MHz
//				r8169soc_ocp_reg_write(tp, 0xde30, (r8169soc_ocp_reg_read(tp, 0xde30) & ~(BIT(6) | BIT(7))));
//				//# ETN spec, set external PHY addr
//				r8169soc_ocp_reg_write(tp, 0xde24,
//					(r8169soc_ocp_reg_read(tp, 0xde24) & ~(0x1F)) | (tp->ext_phy_id & 0x1F));
//			}
//
//			tmp = r8169soc_ocp_reg_read(tp, 0xea34) & ~(BIT(0) | BIT(1));
//			tmp |= BIT(0); // RGMII
//
//			if (tp->output_mode == OUTPUT_RGMII_TO_MAC) {
//				tmp &= ~(BIT(3) | BIT(4));
//				tmp |= BIT(4); // speed: 1G
//				tmp |= BIT(2); // full duplex
//			}
//			if (tp->rgmii_rx_delay == RGMII_DELAY_0NS)
//				tmp &= ~BIT(6);
//			else
//				tmp |= BIT(6);
//
//			if (tp->rgmii_tx_delay == RGMII_DELAY_0NS)
//				tmp &= ~BIT(7);
//			else
//				tmp |= BIT(7);
//
//			r8169soc_ocp_reg_write(tp, 0xea34, tmp);
//
//			/* adjust RGMII voltage */
//			rgmii0_addr = ioremap(RGMII0_PAD_CTRL_ADDR, 12);
//			switch(tp->rgmii_voltage) {
//			case VOLTAGE_1_DOT_8V:
//				writel(0, rgmii0_addr);
//				writel(0x44444444, rgmii0_addr + 4);
//				writel(0x24444444, rgmii0_addr + 8);
//				break;
//			case VOLTAGE_2_DOT_5V:
//				writel(0, rgmii0_addr);
//				writel(0x44444444, rgmii0_addr + 4);
//				writel(0x64444444, rgmii0_addr + 8);
//				break;
//			case VOLTAGE_3_DOT_3V:
//				writel(0x3F, rgmii0_addr);
//				writel(0, rgmii0_addr + 4);
//				writel(0xA4000000, rgmii0_addr + 8);
//			}
//			iounmap(rgmii0_addr);
//
//			/* switch RGMII/MDIO to GMAC */
//			tmp_addr = ioremap(ISO_RGMII_MDIO_TO_GMAC, 4);
//			writel((readl(tmp_addr) | BIT(1)), tmp_addr);
//			iounmap(tmp_addr);
//
//			if (tp->output_mode == OUTPUT_RGMII_TO_MAC) {
//				/* force GMAC link up */
//				r8169soc_ocp_reg_write(tp, 0xde40, 0x30EC);
//				/* ignore mac_intr from PHY */
//				r8169soc_ocp_reg_write(tp, 0xfc1e, (r8169soc_ocp_reg_read(tp, 0xfc1e) & ~BIT(1)));
//			}
//		}
//	}
//	#endif /* CONFIG_ARCH_RTD129x */
}
