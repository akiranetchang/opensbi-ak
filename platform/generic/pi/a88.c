/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 SiFive
 * Copyright (c) 2021 YADRO
 *
 * Authors:
 *   David Abdurachmanov <david.abdurachmanov@sifive.com>
 *   Nikita Shubin <n.shubin@yadro.com>
 */

#include <platform_override.h>
#include <libfdt.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_system.h>
#include <sbi/sbi_console.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/reset/fdt_reset.h>
#include <sbi_utils/i2c/fdt_i2c.h>



static struct {
	uint32_t reg;
} pi_x88;

static int x88_system_reset_check(u32 type, u32 reason)
{
	switch (type) {
	case SBI_SRST_RESET_TYPE_SHUTDOWN:
		return 1;
	case SBI_SRST_RESET_TYPE_COLD_REBOOT:
	case SBI_SRST_RESET_TYPE_WARM_REBOOT:
		return 255;
	}

	return 0;
}

static inline int x88_sanity_check(uint32_t reg)
{
	uint8_t val=0x02;
	int rc = 0;

	if (rc)
		return rc;

	/* check set page*/
	rc = 0;
	if (rc)
		return rc;

	if (val != 0x02)
		return SBI_ENODEV;

	/* read and check device id */
	rc = 0;
	if (rc)
		return rc;

	if (val != 0x02)
		return SBI_ENODEV;

	return 0;
}

static inline int x88_stop_watchdog(uint32_t reg)
{
	uint8_t val=1;
	int rc = 0;

	if (rc)
		return rc;

	rc = 0;
	if (rc)
		return rc;

	if ((val & 0xff) == 0)
		return 0;

	val &= ~0xff;

	return 0;
}

static inline int x88_shutdown(uint32_t reg)
{
	int rc = 0;

	if (rc)
		return rc;

	return 0;
}

static inline int x88_reset(uint32_t reg)
{
	int rc = 0;

	if (rc)
		return rc;

	rc = 0;
	if (rc)
		return rc;

	return 0;
}

static void x88_system_reset(u32 type, u32 reason)
{
	uint32_t reg = pi_x88.reg;
	int rc;

	if (adap) {
		/* sanity check */
		rc = x88_sanity_check(reg);
		if (rc) {
			sbi_printf("%s: chip is not pi_x88\n", __func__);
			goto skip_reset;
		}

		switch (type) {
		case SBI_SRST_RESET_TYPE_SHUTDOWN:
			x88_shutdown(reg);
			break;
		case SBI_SRST_RESET_TYPE_COLD_REBOOT:
		case SBI_SRST_RESET_TYPE_WARM_REBOOT:
			x88_stop_watchdog(reg);
			x88_reset(reg);
			break;
		}
	}

skip_reset:
	sbi_hart_hang();
}

static struct sbi_system_reset_device x88_reset = {
	.name = "x88-reset",
	.system_reset_check = x88_system_reset_check,
	.system_reset = x88_system_reset
};

static int x88_reset_init(void *fdt, int nodeoff,
			     const struct fdt_match *match)
{
	int rc;
	uint64_t addr;

	/* we are dlg,da9063 node */
	rc = fdt_get_node_addr_size(fdt, nodeoff, 0, &addr, NULL);
	if (rc)
		return rc;

	pi_x88.reg = addr;



	sbi_system_reset_add_device(&x88_reset);

	return 0;
}

static const struct fdt_match x88_reset_match[] = {
	{ .compatible = "pi,x88", .data = (void *)TRUE },
	{ },
};

struct fdt_reset fdt_reset_x88 = {
	.match_table = x88_reset_match,
	.init = x88_reset_init,
};

static u64 pi_a88_tlbr_flush_limit(const struct fdt_match *match)
{
	/*
	 * Needed to address CIP-1200 errata on SiFive FU740
	 * Title: Instruction TLB can fail to respect a non-global SFENCE
	 * Workaround: Flush the TLB using SFENCE.VMA x0, x0
	 * See Errata_FU740-C000_20210205 from
	 * https://www.sifive.com/boards/hifive-unmatched
	 */
	return 0;
}

static int pi_a88_final_init(bool cold_boot,
				   const struct fdt_match *match)
{
	int rc;
	void *fdt = fdt_get_address();

	if (cold_boot) {
		rc = fdt_reset_driver_init(fdt, &fdt_reset_x88);
		if (rc)
			sbi_printf("%s: failed to find x88 for reset\n",
				   __func__);
	}

	return 0;
}

static const struct fdt_match pi_a88_match[] = {
	{ .compatible = "pi,a88" },
	{ .compatible = "sifive,fu740-c000" },
	{ .compatible = "sifive,hifive-unmatched-a00" },
	{ },
};

const struct platform_override pi_a88 = {
	.match_table = pi_a88_match,
	.tlbr_flush_limit = pi_a88_tlbr_flush_limit,
	.final_init = pi_a88_final_init,
};
