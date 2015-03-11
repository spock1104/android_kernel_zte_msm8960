/* Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifdef CONFIG_SPI_QUP
#include <linux/spi/spi.h>
#endif
#include <linux/leds.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_novatek.h"
#include "mdp4.h"
#include <mach/gpio.h>

extern u32 LcdPanleID;//zzl,2012.11.27
static struct mipi_dsi_panel_platform_data *mipi_novatek_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;
static int mipi_novatek_lcd_init(void);
static void lcd_panle_reset(void);

static int wled_trigger_initialized;

#define MIPI_DSI_NOVATEK_SPI_DEVICE_NAME	"dsi_novatek_3d_panel_spi"
#define HPCI_FPGA_READ_CMD	0x84
#define HPCI_FPGA_WRITE_CMD	0x04

#ifdef CONFIG_SPI_QUP
static struct spi_device *panel_3d_spi_client;

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	u8 data[4] = {0x0, 0x0, 0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}
	data[0] = HPCI_FPGA_WRITE_CMD;
	data[1] = addr;
	data[2] = ((value >> 8) & 0xFF);
	data[3] = (value & 0xFF);

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	t.tx_buf = data;
	t.len = 4;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);

	return;
}

static void novatek_fpga_read(uint8 addr)
{
	char tx_buf[32];
	int  rc;
	struct spi_message  m;
	struct spi_transfer t;
	struct spi_transfer rx;
	char rx_value[2];
	u8 data[4] = {0x0, 0x0};

	if (!panel_3d_spi_client) {
		pr_err("%s panel_3d_spi_client is NULL\n", __func__);
		return;
	}

	data[0] = HPCI_FPGA_READ_CMD;
	data[1] = addr;

	memset(&t, 0, sizeof t);
	memset(tx_buf, 0, sizeof tx_buf);
	memset(&rx, 0, sizeof rx);
	memset(rx_value, 0, sizeof rx_value);
	t.tx_buf = data;
	t.len = 2;
	rx.rx_buf = rx_value;
	rx.len = 2;
	spi_setup(panel_3d_spi_client);
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	spi_message_add_tail(&rx, &m);

	rc = spi_sync(panel_3d_spi_client, &m);
	if (rc)
		pr_err("%s: SPI transfer failed\n", __func__);
	else
		pr_info("%s: rx_value = 0x%x, 0x%x\n", __func__,
						rx_value[0], rx_value[1]);

	return;
}

static int __devinit panel_3d_spi_probe(struct spi_device *spi)
{
	panel_3d_spi_client = spi;
	return 0;
}
static int __devexit panel_3d_spi_remove(struct spi_device *spi)
{
	panel_3d_spi_client = NULL;
	return 0;
}
static struct spi_driver panel_3d_spi_driver = {
	.probe         = panel_3d_spi_probe,
	.remove        = __devexit_p(panel_3d_spi_remove),
	.driver		   = {
		.name = "dsi_novatek_3d_panel_spi",
		.owner  = THIS_MODULE,
	}
};

#else

static void novatek_fpga_write(uint8 addr, uint16 value)
{
	return;
}

static void novatek_fpga_read(uint8 addr)
{
	return;
}

#endif


/* novatek blue panel */

#ifdef NOVETAK_COMMANDS_UNUSED
static char display_config_cmd_mode1[] = {
	/* TYPE_DCS_LWRITE */
	0x2A, 0x00, 0x00, 0x01,
	0x3F, 0xFF, 0xFF, 0xFF
};

static char display_config_cmd_mode2[] = {
	/* DTYPE_DCS_LWRITE */
	0x2B, 0x00, 0x00, 0x01,
	0xDF, 0xFF, 0xFF, 0xFF
};

static char display_config_cmd_mode3_666[] = {
	/* DTYPE_DCS_WRITE1 */
	0x3A, 0x66, 0x15, 0x80 /* 666 Packed (18-bits) */
};

static char display_config_cmd_mode3_565[] = {
	/* DTYPE_DCS_WRITE1 */
	0x3A, 0x55, 0x15, 0x80 /* 565 mode */
};

static char display_config_321[] = {
	/* DTYPE_DCS_WRITE1 */
	0x66, 0x2e, 0x15, 0x00 /* Reg 0x66 : 2E */
};

static char display_config_323[] = {
	/* DTYPE_DCS_WRITE */
	0x13, 0x00, 0x05, 0x00 /* Reg 0x13 < Set for Normal Mode> */
};

static char display_config_2lan[] = {
	/* DTYPE_DCS_WRITE */
	0x61, 0x01, 0x02, 0xff /* Reg 0x61 : 01,02 < Set for 2 Data Lane > */
};

static char display_config_exit_sleep[] = {
	/* DTYPE_DCS_WRITE */
	0x11, 0x00, 0x05, 0x80 /* Reg 0x11 < exit sleep mode> */
};

static char display_config_TE_ON[] = {
	/* DTYPE_DCS_WRITE1 */
	0x35, 0x00, 0x15, 0x80
};

static char display_config_39H[] = {
	/* DTYPE_DCS_WRITE */
	0x39, 0x00, 0x05, 0x80
};

static char display_config_set_tear_scanline[] = {
	/* DTYPE_DCS_LWRITE */
	0x44, 0x00, 0x00, 0xff
};

static char display_config_set_twolane[] = {
	/* DTYPE_DCS_WRITE1 */
	0xae, 0x03, 0x15, 0x80
};

static char display_config_set_threelane[] = {
	/* DTYPE_DCS_WRITE1 */
	0xae, 0x05, 0x15, 0x80
};

#else

static char sw_reset[2] = {0x01, 0x00}; /* DTYPE_DCS_WRITE */
static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_off[2] = {0x28, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */



static char rgb_888[2] = {0x3A, 0x77}; /* DTYPE_DCS_WRITE1 */

#if defined(NOVATEK_TWO_LANE)
static char set_num_of_lanes[2] = {0xae, 0x03}; /* DTYPE_DCS_WRITE1 */
#else  /* 1 lane */
static char set_num_of_lanes[2] = {0xae, 0x01}; /* DTYPE_DCS_WRITE1 */
#endif
/* commands by Novatke */
static char novatek_f4[2] = {0xf4, 0x55}; /* DTYPE_DCS_WRITE1 */
static char novatek_8c[16] = { /* DTYPE_DCS_LWRITE */
	0x8C, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x08, 0x00, 0x30, 0xC0, 0xB7, 0x37};
static char novatek_ff[2] = {0xff, 0x55 }; /* DTYPE_DCS_WRITE1 */

static char set_width[5] = { /* DTYPE_DCS_LWRITE */
	0x2A, 0x00, 0x00, 0x02, 0x1B}; /* 540 - 1 */
static char set_height[5] = { /* DTYPE_DCS_LWRITE */
	0x2B, 0x00, 0x00, 0x03, 0xBF}; /* 960 - 1 */
#endif

static char led_pwm2[2] = {0x53, 0x24}; /* DTYPE_DCS_WRITE1 */
static char led_pwm3[2] = {0x55, 0x00}; /* DTYPE_DCS_WRITE1 */
#if 0//back
static struct dsi_cmd_desc novatek_video_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(set_num_of_lanes), set_num_of_lanes},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(led_pwm3), led_pwm3},
};
#endif

static char otm_shift_00[] = {0x00, 0x00};

//truly OTM1281A
static char truly_cmd_FF1[] = {0xFF, 0x12,0x80,0x01};
static char truly_shift_FF2[] = {0x00, 0x80};
static char truly_cmd_FF2[] = {0xFF, 0x12,0x80};
static char truly_shift_b0[] = {0x00, 0xb0};
static char truly_cmd_b0[] = {0xb0, 0x20};
static char truly_shift_C51[] = {0x00, 0x90};
static char truly_cmd_C51[] = {0xC5, 0x20,0x6F,0x02,0x88,0x1d,0x15,0x00,0x08,0x44,0x44,0x44};
static char truly_shift_C52[] = {0x00, 0xa0};
static char truly_cmd_C52[] = {0xC5, 0x20,0x6F,0x02,0x88,0x1d,0x15,0x00,0x08};
static char truly_shift_C53[] = {0x00, 0x80};
static char truly_cmd_C53[] = {0xC5, 0x20,0x01,0x00,0xB0,0xB0,0x00,0x08,0x00};
static char truly_shift_D8[] = {0x00, 0x00};
static char truly_cmd_D8[] = {0xD8, 0x58,0x00,0x58,0x00};
static char truly_shift_F5[] = {0x00, 0xb8};
static char truly_cmd_F5[] = {0xF5, 0x0C,0x12};

static char truly_shift_C1_80[] = {0x00, 0x80};
static char truly_cmd_C1_80[] = {0xc1, 0x85};
static char truly_shift_C5_D0[] = {0x00, 0xD0};
static char truly_cmd_C5_D0[] = {0xc5,0x38};
#ifdef COLOR_ENHANCE
static char truly_shift_83[] = {0x00, 0x83};
static char truly_cmd_F4[] = {0xf4, 0x00};
static char truly_shift_00[] = {0x00, 0x00};
static char truly_cmd_D4[] = {0xD4,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,
						    0x40};
static char truly_cmd_D5[] = {0xD5,0x00,0x60,0x00,0x60,0x00,0x5f,0x00,0x5f,0x00,
						  0x5e,0x00,0x5e,0x00,0x5d,0x00,0x5d,0x00,0x5d,0x00,
						  0x5c,0x00,0x5c,0x00,0x5b,0x00,0x5b,0x00,0x5a,0x00,
						  0x5a,0x00,0x5a,0x00,0x5b,0x00,0x5c,0x00,0x5d,0x00,
						  0x5d,0x00,0x5e,0x00,0x5f,0x00,0x60,0x00,0x61,0x00,
						  0x62,0x00,0x63,0x00,0x63,0x00,0x64,0x00,0x65,0x00,
						  0x66,0x00,0x67,0x00,0x68,0x00,0x69,0x00,0x69,0x00,
						  0x6a,0x00,0x6b,0x00,0x6c,0x00,0x6d,0x00,0x6e,0x00,
						  0x6f,0x00,0x6f,0x00,0x70,0x00,0x71,0x00,0x72,0x00,
						  0x73,0x00,0x74,0x00,0x74,0x00,0x75,0x00,0x76,0x00,
						  0x77,0x00,0x78,0x00,0x78,0x00,0x79,0x00,0x7a,0x00,
						  0x7b,0x00,0x7c,0x00,0x7d,0x00,0x7d,0x00,0x7e,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,
						  0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7e,0x00,0x7d,0x00,
						  0x7c,0x00,0x7b,0x00,0x7a,0x00,0x7a,0x00,0x79,0x00,
						  0x78,0x00,0x77,0x00,0x76,0x00,0x76,0x00,0x75,0x00,
						  0x74,0x00,0x73,0x00,0x72,0x00,0x71,0x00,0x71,0x00,
						  0x70,0x00,0x6f,0x00,0x6e,0x00,0x6d,0x00,0x6c,0x00,
						  0x6c,0x00,0x6b,0x00,0x6a,0x00,0x69,0x00,0x68,0x00,
						  0x67,0x00,0x66,0x00,0x66,0x00,0x66,0x00,0x65,0x00,
						  0x65,0x00,0x64,0x00,0x64,0x00,0x63,0x00,0x63,0x00,
						  0x63,0x00,0x62,0x00,0x62,0x00,0x61,0x00,0x61,0x00,
						  0x60};
#endif
static char truly_cmd_36[] = {0x36, 0xd0};
//static char truly_cmd_35[] = {0x35, 0x00};

static char truly_cmd_2c[] = {0x2c, 0x00};
static char truly_cmd_3c[] = {0x3c, 0x00};
static char truly_shift_B0_80[] = {0x00,0x80};
static char truly_cmd_B0_80[] = {0xb0,0x60};
static char truly_shift_B0_B1[] = {0x00,0xB1};
static char truly_cmd_B0_B1[] = {0xb0,004};


static struct dsi_cmd_desc truly_display_on_cmds[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_FF1),truly_cmd_FF1},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_FF2),truly_shift_FF2},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_FF2),truly_cmd_FF2},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_b0),truly_shift_b0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_b0),truly_cmd_b0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_C51),truly_shift_C51},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_C51),truly_cmd_C51},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_C52),truly_shift_C52},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_C52),truly_cmd_C52},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_C53),truly_shift_C53},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_C53),truly_cmd_C53},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_D8),truly_shift_D8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_D8),truly_cmd_D8},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_F5),truly_shift_F5},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_F5),truly_cmd_F5},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_C1_80),truly_shift_C1_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_C1_80),truly_cmd_C1_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_C5_D0),truly_shift_C5_D0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_C5_D0),truly_cmd_C5_D0},


#ifdef COLOR_ENHANCE
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_83),truly_shift_83},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_F4),truly_cmd_F4},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_00),truly_shift_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_D4),truly_cmd_D4},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_00),truly_shift_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_D5),truly_cmd_D5},

#endif
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(truly_cmd_36),truly_cmd_36},
//		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_cmd_35),truly_cmd_35},
		{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep),exit_sleep},
	
		{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},

		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_cmd_2c),truly_cmd_2c},
	
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_cmd_3c), truly_cmd_3c},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_shift_B0_80),truly_shift_B0_80},
	
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_cmd_B0_80), truly_cmd_B0_80},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_shift_B0_B1),truly_shift_B0_B1},
	
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(truly_cmd_B0_B1), truly_cmd_B0_B1},

};


static char tianma_NT35590_para001[] = {0xFF,0x01};
static char tianma_NT35590_para002[] = {0xFB,0x01};
static char tianma_NT35590_para003[] = {0x00,0x2A};
static char tianma_NT35590_para004[] = {0x01,0x33};
static char tianma_NT35590_para005[] = {0x02,0x53};
static char tianma_NT35590_para006[] = {0x03,0x53};
static char tianma_NT35590_para007[] = {0x04,0x55};
static char tianma_NT35590_para008[] = {0x05,0x33};
static char tianma_NT35590_para009[] = {0x06,0x22};
static char tianma_NT35590_para010[] = {0x08,0x66};
static char tianma_NT35590_para011[] = {0x09,0x8A};
static char tianma_NT35590_para012[] = {0x0B,0xBF};
static char tianma_NT35590_para013[] = {0x0C,0xBF};
static char tianma_NT35590_para014[] = {0x0D,0x2F};
static char tianma_NT35590_para015[] = {0x0E,0x33};
static char tianma_NT35590_para016[] = {0x11,0x95};
static char tianma_NT35590_para017[] = {0x12,0x03};
static char tianma_NT35590_para018[] = {0x71,0x2C};
static char tianma_NT35590_para019[] = {0x0F,0x0A};
static char tianma_NT35590_para020[] = {0x36,0x73};
static char tianma_NT35590_para021[] = {0xFF,0x00};
static char tianma_NT35590_para022[] = {0xFF,0x05};

static char tianma_NT35590_para21[] = {0x21,0x06};//add

static char tianma_NT35590_para023[] = {0xFB,0x01};
static char tianma_NT35590_para024[] = {0x01,0x00};
static char tianma_NT35590_para025[] = {0x02,0x8D};
static char tianma_NT35590_para026[] = {0x03,0x8E};
static char tianma_NT35590_para027[] = {0x04,0x8E};
static char tianma_NT35590_para028[] = {0x05,0x30};
static char tianma_NT35590_para029[] = {0x06,0x33};
static char tianma_NT35590_para030[] = {0x07,0x00};
static char tianma_NT35590_para031[] = {0x08,0x00};
static char tianma_NT35590_para032[] = {0x09,0x00};
static char tianma_NT35590_para033[] = {0x0A,0x7E};
static char tianma_NT35590_para034[] = {0x0B,0x83};
static char tianma_NT35590_para035[] = {0x0D,0x08};
static char tianma_NT35590_para036[] = {0x0E,0x1A};
static char tianma_NT35590_para037[] = {0x0F,0x0A};
static char tianma_NT35590_para038[] = {0x10,0x71};
static char tianma_NT35590_para039[] = {0x14,0x05};
static char tianma_NT35590_para040[] = {0x16,0x07};
static char tianma_NT35590_para041[] = {0x17,0x00};
static char tianma_NT35590_para042[] = {0x19,0x03};
static char tianma_NT35590_para043[] = {0x1A,0x00};
static char tianma_NT35590_para044[] = {0x1B,0xFC};
static char tianma_NT35590_para045[] = {0x1C,0x00};
static char tianma_NT35590_para046[] = {0x1D,0x00};
static char tianma_NT35590_para047[] = {0x1E,0x00};
static char tianma_NT35590_para048[] = {0x1F,0x00};
static char tianma_NT35590_para049[] = {0x22,0x55};
static char tianma_NT35590_para050[] = {0x23,0x0D};
static char tianma_NT35590_para051[] = {0x6C,0x03};
static char tianma_NT35590_para052[] = {0x6D,0x03};
static char tianma_NT35590_para053[] = {0x24,0x45};
static char tianma_NT35590_para054[] = {0x26,0x55};
static char tianma_NT35590_para055[] = {0x27,0x55};
static char tianma_NT35590_para056[] = {0x28,0x01};
static char tianma_NT35590_para057[] = {0x2F,0x02};
static char tianma_NT35590_para058[] = {0x29,0x52};
static char tianma_NT35590_para059[] = {0x2A,0xA9};
static char tianma_NT35590_para060[] = {0x2B,0x12};
static char tianma_NT35590_para061[] = {0x2D,0x02};
static char tianma_NT35590_para062[] = {0x32,0x00};
static char tianma_NT35590_para063[] = {0x33,0xB8};
static char tianma_NT35590_para064[] = {0x35,0x16};
static char tianma_NT35590_para065[] = {0x36,0x00};
static char tianma_NT35590_para066[] = {0x37,0x09};
static char tianma_NT35590_para067[] = {0x38,0x00};
static char tianma_NT35590_para068[] = {0x39,0x00};
static char tianma_NT35590_para069[] = {0x49,0x00};
static char tianma_NT35590_para070[] = {0x4A,0x01};
static char tianma_NT35590_para071[] = {0x82,0x18};
static char tianma_NT35590_para072[] = {0x89,0x00};
static char tianma_NT35590_para073[] = {0x8D,0x01};
static char tianma_NT35590_para074[] = {0x8E,0x64};
static char tianma_NT35590_para075[] = {0x8F,0x20};
static char tianma_NT35590_para076[] = {0x92,0x00};
static char tianma_NT35590_para077[] = {0x9E,0x60};
static char tianma_NT35590_para078[] = {0x9F,0x6B};
static char tianma_NT35590_para079[] = {0xA2,0x10};
static char tianma_NT35590_para080[] = {0xA4,0x05};
static char tianma_NT35590_para081[] = {0xBB,0x0A};
static char tianma_NT35590_para082[] = {0xBC,0x0A};
static char tianma_NT35590_para083[] = {0xFF,0x00};
static char tianma_NT35590_para084[] = {0xFF,0x01};
static char tianma_NT35590_para085[] = {0x75,0x01};
static char tianma_NT35590_para086[] = {0x76,0x23};
static char tianma_NT35590_para087[] = {0x77,0x01};
static char tianma_NT35590_para088[] = {0x78,0x31};
static char tianma_NT35590_para089[] = {0x79,0x01};
static char tianma_NT35590_para090[] = {0x7A,0x65};
static char tianma_NT35590_para091[] = {0x7B,0x01};
static char tianma_NT35590_para092[] = {0x7C,0x7F};
static char tianma_NT35590_para093[] = {0x7D,0x01};
static char tianma_NT35590_para094[] = {0x7E,0x95};
static char tianma_NT35590_para095[] = {0x7F,0x01};
static char tianma_NT35590_para096[] = {0x80,0xA4};
static char tianma_NT35590_para097[] = {0x81,0x01};
static char tianma_NT35590_para098[] = {0x82,0xB2};
static char tianma_NT35590_para099[] = {0x83,0x01};
static char tianma_NT35590_para100[] = {0x84,0xC0};
static char tianma_NT35590_para101[] = {0x85,0x01};
static char tianma_NT35590_para102[] = {0x86,0xCB};
static char tianma_NT35590_para103[] = {0x87,0x01};
static char tianma_NT35590_para104[] = {0x88,0xE4};
static char tianma_NT35590_para105[] = {0x89,0x02};
static char tianma_NT35590_para106[] = {0x8A,0x00};
static char tianma_NT35590_para107[] = {0x8B,0x02};
static char tianma_NT35590_para108[] = {0x8C,0x2B};
static char tianma_NT35590_para109[] = {0x8D,0x02};
static char tianma_NT35590_para110[] = {0x8E,0x4F};
static char tianma_NT35590_para111[] = {0x8F,0x02};
static char tianma_NT35590_para112[] = {0x90,0x85};
static char tianma_NT35590_para113[] = {0x91,0x02};
static char tianma_NT35590_para114[] = {0x92,0xB2};
static char tianma_NT35590_para115[] = {0x93,0x02};
static char tianma_NT35590_para116[] = {0x94,0xB3};
static char tianma_NT35590_para117[] = {0x95,0x02};
static char tianma_NT35590_para118[] = {0x96,0xDA};
static char tianma_NT35590_para119[] = {0x97,0x03};
static char tianma_NT35590_para120[] = {0x98,0x05};
static char tianma_NT35590_para121[] = {0x99,0x03};
static char tianma_NT35590_para122[] = {0x9A,0x1E};
static char tianma_NT35590_para123[] = {0x9B,0x03};
static char tianma_NT35590_para124[] = {0x9C,0x3C};
static char tianma_NT35590_para125[] = {0x9D,0x03};
static char tianma_NT35590_para126[] = {0x9E,0x51};
static char tianma_NT35590_para127[] = {0x9F,0x03};
static char tianma_NT35590_para128[] = {0xA0,0x6A};
static char tianma_NT35590_para129[] = {0xA2,0x03};
static char tianma_NT35590_para130[] = {0xA3,0x72};
static char tianma_NT35590_para131[] = {0xA4,0x03};
static char tianma_NT35590_para132[] = {0xA5,0x7D};
static char tianma_NT35590_para133[] = {0xA6,0x03};
static char tianma_NT35590_para134[] = {0xA7,0x8D};
static char tianma_NT35590_para135[] = {0xA9,0x03};
static char tianma_NT35590_para136[] = {0xAA,0x9C};
static char tianma_NT35590_para137[] = {0xAB,0x03};
static char tianma_NT35590_para138[] = {0xAC,0xA1};
static char tianma_NT35590_para139[] = {0xAD,0x03};
static char tianma_NT35590_para140[] = {0xAE,0xAD};
static char tianma_NT35590_para141[] = {0xAF,0x03};
static char tianma_NT35590_para142[] = {0xB0,0xBD};
static char tianma_NT35590_para143[] = {0xB1,0x03};
static char tianma_NT35590_para144[] = {0xB2,0xDE};
static char tianma_NT35590_para145[] = {0xB3,0x01};
static char tianma_NT35590_para146[] = {0xB4,0x23};
static char tianma_NT35590_para147[] = {0xB5,0x01};
static char tianma_NT35590_para148[] = {0xB6,0x31};
static char tianma_NT35590_para149[] = {0xB7,0x01};
static char tianma_NT35590_para150[] = {0xB8,0x65};
static char tianma_NT35590_para151[] = {0xB9,0x01};
static char tianma_NT35590_para152[] = {0xBA,0x7F};
static char tianma_NT35590_para153[] = {0xBB,0x01};
static char tianma_NT35590_para154[] = {0xBC,0x95};
static char tianma_NT35590_para155[] = {0xBD,0x01};
static char tianma_NT35590_para156[] = {0xBE,0xA4};
static char tianma_NT35590_para157[] = {0xBF,0x01};
static char tianma_NT35590_para158[] = {0xC0,0xB2};
static char tianma_NT35590_para159[] = {0xC1,0x01};
static char tianma_NT35590_para160[] = {0xC2,0xC0};
static char tianma_NT35590_para161[] = {0xC3,0x01};
static char tianma_NT35590_para162[] = {0xC4,0xCB};
static char tianma_NT35590_para163[] = {0xC5,0x01};
static char tianma_NT35590_para164[] = {0xC6,0xE4};
static char tianma_NT35590_para165[] = {0xC7,0x02};
static char tianma_NT35590_para166[] = {0xC8,0x00};
static char tianma_NT35590_para167[] = {0xC9,0x02};
static char tianma_NT35590_para168[] = {0xCA,0x2B};
static char tianma_NT35590_para169[] = {0xCB,0x02};
static char tianma_NT35590_para170[] = {0xCC,0x4F};
static char tianma_NT35590_para171[] = {0xCD,0x02};
static char tianma_NT35590_para172[] = {0xCE,0x85};
static char tianma_NT35590_para173[] = {0xCF,0x02};
static char tianma_NT35590_para174[] = {0xD0,0xB2};
static char tianma_NT35590_para175[] = {0xD1,0x02};
static char tianma_NT35590_para176[] = {0xD2,0xB3};
static char tianma_NT35590_para177[] = {0xD3,0x02};
static char tianma_NT35590_para178[] = {0xD4,0xDA};
static char tianma_NT35590_para179[] = {0xD5,0x03};
static char tianma_NT35590_para180[] = {0xD6,0x05};
static char tianma_NT35590_para181[] = {0xD7,0x03};
static char tianma_NT35590_para182[] = {0xD8,0x1E};
static char tianma_NT35590_para183[] = {0xD9,0x03};
static char tianma_NT35590_para184[] = {0xDA,0x3C};
static char tianma_NT35590_para185[] = {0xDB,0x03};
static char tianma_NT35590_para186[] = {0xDC,0x51};
static char tianma_NT35590_para187[] = {0xDD,0x03};
static char tianma_NT35590_para188[] = {0xDE,0x6A};
static char tianma_NT35590_para189[] = {0xDF,0x03};
static char tianma_NT35590_para190[] = {0xE0,0x72};
static char tianma_NT35590_para191[] = {0xE1,0x03};
static char tianma_NT35590_para192[] = {0xE2,0x7D};
static char tianma_NT35590_para193[] = {0xE3,0x03};
static char tianma_NT35590_para194[] = {0xE4,0x8D};
static char tianma_NT35590_para195[] = {0xE5,0x03};
static char tianma_NT35590_para196[] = {0xE6,0x9C};
static char tianma_NT35590_para197[] = {0xE7,0x03};
static char tianma_NT35590_para198[] = {0xE8,0xA1};
static char tianma_NT35590_para199[] = {0xE9,0x03};
static char tianma_NT35590_para200[] = {0xEA,0xAD};
static char tianma_NT35590_para201[] = {0xEB,0x03};
static char tianma_NT35590_para202[] = {0xEC,0xBD};
static char tianma_NT35590_para203[] = {0xED,0x03};
static char tianma_NT35590_para204[] = {0xEE,0xDE};
static char tianma_NT35590_para205[] = {0xEF,0x01};
static char tianma_NT35590_para206[] = {0xF0,0x23};
static char tianma_NT35590_para207[] = {0xF1,0x01};
static char tianma_NT35590_para208[] = {0xF2,0x31};
static char tianma_NT35590_para209[] = {0xF3,0x01};
static char tianma_NT35590_para210[] = {0xF4,0x65};
static char tianma_NT35590_para211[] = {0xF5,0x01};
static char tianma_NT35590_para212[] = {0xF6,0x7F};
static char tianma_NT35590_para213[] = {0xF7,0x01};
static char tianma_NT35590_para214[] = {0xF8,0x95};
static char tianma_NT35590_para215[] = {0xF9,0x01};
static char tianma_NT35590_para216[] = {0xFA,0xA4};
static char tianma_NT35590_para217[] = {0xFF,0x00};
static char tianma_NT35590_para218[] = {0xFF,0x02};
static char tianma_NT35590_para219[] = {0x00,0x01};
static char tianma_NT35590_para220[] = {0x01,0xB2};
static char tianma_NT35590_para221[] = {0x02,0x01};
static char tianma_NT35590_para222[] = {0x03,0xC0};
static char tianma_NT35590_para223[] = {0x04,0x01};
static char tianma_NT35590_para224[] = {0x05,0xCB};
static char tianma_NT35590_para225[] = {0x06,0x01};
static char tianma_NT35590_para226[] = {0x07,0xE4};
static char tianma_NT35590_para227[] = {0x08,0x02};
static char tianma_NT35590_para228[] = {0x09,0x00};
static char tianma_NT35590_para229[] = {0x0A,0x02};
static char tianma_NT35590_para230[] = {0x0B,0x2B};
static char tianma_NT35590_para231[] = {0x0C,0x02};
static char tianma_NT35590_para232[] = {0x0D,0x4F};
static char tianma_NT35590_para233[] = {0x0E,0x02};
static char tianma_NT35590_para234[] = {0x0F,0x85};
static char tianma_NT35590_para235[] = {0x10,0x02};
static char tianma_NT35590_para236[] = {0x11,0xB2};
static char tianma_NT35590_para237[] = {0x12,0x02};
static char tianma_NT35590_para238[] = {0x13,0xB3};
static char tianma_NT35590_para239[] = {0x14,0x02};
static char tianma_NT35590_para240[] = {0x15,0xDA};
static char tianma_NT35590_para241[] = {0x16,0x03};
static char tianma_NT35590_para242[] = {0x17,0x05};
static char tianma_NT35590_para243[] = {0x18,0x03};
static char tianma_NT35590_para244[] = {0x19,0x1E};
static char tianma_NT35590_para245[] = {0x1A,0x03};
static char tianma_NT35590_para246[] = {0x1B,0x3C};
static char tianma_NT35590_para247[] = {0x1C,0x03};
static char tianma_NT35590_para248[] = {0x1D,0x51};
static char tianma_NT35590_para249[] = {0xAE,0x03};
static char tianma_NT35590_para250[] = {0x1F,0x6A};
static char tianma_NT35590_para251[] = {0x20,0x03};
static char tianma_NT35590_para252[] = {0x21,0x73};
static char tianma_NT35590_para253[] = {0x22,0x03};
static char tianma_NT35590_para254[] = {0x23,0x7D};
static char tianma_NT35590_para255[] = {0x24,0x03};
static char tianma_NT35590_para256[] = {0x25,0x8D};
static char tianma_NT35590_para257[] = {0x26,0x03};
static char tianma_NT35590_para258[] = {0x27,0x9C};
static char tianma_NT35590_para259[] = {0x28,0x03};
static char tianma_NT35590_para260[] = {0x29,0xA1};
static char tianma_NT35590_para261[] = {0x2A,0x03};
static char tianma_NT35590_para262[] = {0x2B,0xAD};
static char tianma_NT35590_para263[] = {0x2D,0x03};
static char tianma_NT35590_para264[] = {0x2F,0xBD};
static char tianma_NT35590_para265[] = {0x30,0x03};
static char tianma_NT35590_para266[] = {0x31,0xDE};
static char tianma_NT35590_para267[] = {0x32,0x01};
static char tianma_NT35590_para268[] = {0x33,0x23};
static char tianma_NT35590_para269[] = {0x34,0x01};
static char tianma_NT35590_para270[] = {0x35,0x31};
static char tianma_NT35590_para271[] = {0x36,0x01};
static char tianma_NT35590_para272[] = {0x37,0x65};
static char tianma_NT35590_para273[] = {0x38,0x01};
static char tianma_NT35590_para274[] = {0x39,0x7F};
static char tianma_NT35590_para275[] = {0x3A,0x01};
static char tianma_NT35590_para276[] = {0x3B,0x95};
static char tianma_NT35590_para277[] = {0x3D,0x01};
static char tianma_NT35590_para278[] = {0x3F,0xA4};
static char tianma_NT35590_para279[] = {0x40,0x01};
static char tianma_NT35590_para280[] = {0x41,0xB2};
static char tianma_NT35590_para281[] = {0x42,0x01};
static char tianma_NT35590_para282[] = {0x43,0xC0};
static char tianma_NT35590_para283[] = {0x44,0x01};
static char tianma_NT35590_para284[] = {0x45,0xCB};
static char tianma_NT35590_para285[] = {0x46,0x01};
static char tianma_NT35590_para286[] = {0x47,0xE4};
static char tianma_NT35590_para287[] = {0x48,0x02};
static char tianma_NT35590_para288[] = {0x49,0x00};
static char tianma_NT35590_para289[] = {0x4A,0x02};
static char tianma_NT35590_para290[] = {0x4B,0x2B};
static char tianma_NT35590_para291[] = {0x4C,0x02};
static char tianma_NT35590_para292[] = {0x4D,0x4F};
static char tianma_NT35590_para293[] = {0x4E,0x02};
static char tianma_NT35590_para294[] = {0x4F,0x85};
static char tianma_NT35590_para295[] = {0x50,0x02};
static char tianma_NT35590_para296[] = {0x51,0xB2};
static char tianma_NT35590_para297[] = {0x52,0x02};
static char tianma_NT35590_para298[] = {0x53,0xB3};
static char tianma_NT35590_para299[] = {0x54,0x02};
static char tianma_NT35590_para300[] = {0x55,0xDA};
static char tianma_NT35590_para301[] = {0x56,0x03};
static char tianma_NT35590_para302[] = {0x58,0x05};
static char tianma_NT35590_para303[] = {0x59,0x03};
static char tianma_NT35590_para304[] = {0x5A,0x1E};
static char tianma_NT35590_para305[] = {0x5B,0x03};
static char tianma_NT35590_para306[] = {0x5C,0x3C};
static char tianma_NT35590_para307[] = {0x5D,0x03};
static char tianma_NT35590_para308[] = {0x5E,0x51};
static char tianma_NT35590_para309[] = {0x5F,0x03};
static char tianma_NT35590_para310[] = {0x60,0x6A};
static char tianma_NT35590_para311[] = {0x61,0x03};
static char tianma_NT35590_para312[] = {0x62,0x72};
static char tianma_NT35590_para313[] = {0x63,0x03};
static char tianma_NT35590_para314[] = {0x64,0x7D};
static char tianma_NT35590_para315[] = {0x65,0x03};
static char tianma_NT35590_para316[] = {0x66,0x8D};
static char tianma_NT35590_para317[] = {0x67,0x03};
static char tianma_NT35590_para318[] = {0x68,0x9C};
static char tianma_NT35590_para319[] = {0x69,0x03};
static char tianma_NT35590_para320[] = {0x6A,0xA1};
static char tianma_NT35590_para321[] = {0x6B,0x03};
static char tianma_NT35590_para322[] = {0x6C,0xAD};
static char tianma_NT35590_para323[] = {0x6D,0x03};
static char tianma_NT35590_para324[] = {0x6E,0xBD};
static char tianma_NT35590_para325[] = {0x6F,0x03};
static char tianma_NT35590_para326[] = {0x70,0xDE};
static char tianma_NT35590_para327[] = {0x71,0x01};
static char tianma_NT35590_para328[] = {0x72,0x23};
static char tianma_NT35590_para329[] = {0x73,0x01};
static char tianma_NT35590_para330[] = {0x74,0x31};
static char tianma_NT35590_para331[] = {0x75,0x01};
static char tianma_NT35590_para332[] = {0x76,0x65};
static char tianma_NT35590_para333[] = {0x77,0x01};
static char tianma_NT35590_para334[] = {0x78,0x7F};
static char tianma_NT35590_para335[] = {0x79,0x01};
static char tianma_NT35590_para336[] = {0x7A,0x95};
static char tianma_NT35590_para337[] = {0x7B,0x01};
static char tianma_NT35590_para338[] = {0x7C,0xA4};
static char tianma_NT35590_para339[] = {0x7D,0x01};
static char tianma_NT35590_para340[] = {0x7E,0xB2};
static char tianma_NT35590_para341[] = {0x7F,0x01};
static char tianma_NT35590_para342[] = {0x80,0xC0};
static char tianma_NT35590_para343[] = {0x81,0x01};
static char tianma_NT35590_para344[] = {0x82,0xCB};
static char tianma_NT35590_para345[] = {0x83,0x01};
static char tianma_NT35590_para346[] = {0x84,0xE4};
static char tianma_NT35590_para347[] = {0x85,0x02};
static char tianma_NT35590_para348[] = {0x86,0x00};
static char tianma_NT35590_para349[] = {0x87,0x02};
static char tianma_NT35590_para350[] = {0x88,0x2B};
static char tianma_NT35590_para351[] = {0x89,0x02};
static char tianma_NT35590_para352[] = {0x8A,0x4F};
static char tianma_NT35590_para353[] = {0x8B,0x02};
static char tianma_NT35590_para354[] = {0x8C,0x85};
static char tianma_NT35590_para355[] = {0x8D,0x02};
static char tianma_NT35590_para356[] = {0x8E,0xB2};
static char tianma_NT35590_para357[] = {0x8F,0x02};
static char tianma_NT35590_para358[] = {0x90,0xB3};
static char tianma_NT35590_para359[] = {0x91,0x02};
static char tianma_NT35590_para360[] = {0x92,0xDA};
static char tianma_NT35590_para361[] = {0x93,0x03};
static char tianma_NT35590_para362[] = {0x94,0x05};
static char tianma_NT35590_para363[] = {0x95,0x03};
static char tianma_NT35590_para364[] = {0x96,0x1E};
static char tianma_NT35590_para365[] = {0x97,0x03};
static char tianma_NT35590_para366[] = {0x98,0x3C};
static char tianma_NT35590_para367[] = {0x99,0x03};
static char tianma_NT35590_para368[] = {0x9A,0x51};
static char tianma_NT35590_para369[] = {0x9B,0x03};
static char tianma_NT35590_para370[] = {0x9C,0x6A};
static char tianma_NT35590_para371[] = {0x9D,0x03};
static char tianma_NT35590_para372[] = {0x9E,0x72};
static char tianma_NT35590_para373[] = {0x9F,0x03};
static char tianma_NT35590_para374[] = {0xA0,0x7D};
static char tianma_NT35590_para375[] = {0xA2,0x03};
static char tianma_NT35590_para376[] = {0xA3,0x8D};
static char tianma_NT35590_para377[] = {0xA4,0x03};
static char tianma_NT35590_para378[] = {0xA5,0x9C};
static char tianma_NT35590_para379[] = {0xA6,0x03};
static char tianma_NT35590_para380[] = {0xA7,0xA1};
static char tianma_NT35590_para381[] = {0xA9,0x03};
static char tianma_NT35590_para382[] = {0xAA,0xAD};
static char tianma_NT35590_para383[] = {0xAB,0x03};
static char tianma_NT35590_para384[] = {0xAC,0xBD};
static char tianma_NT35590_para385[] = {0xAD,0x03};
static char tianma_NT35590_para386[] = {0xAE,0xDE};
static char tianma_NT35590_para387[] = {0xAF,0x01};
static char tianma_NT35590_para388[] = {0xB0,0x23};
static char tianma_NT35590_para389[] = {0xB1,0x01};
static char tianma_NT35590_para390[] = {0xB2,0x31};
static char tianma_NT35590_para391[] = {0xB3,0x01};
static char tianma_NT35590_para392[] = {0xB4,0x65};
static char tianma_NT35590_para393[] = {0xB5,0x01};
static char tianma_NT35590_para394[] = {0xB6,0x7F};
static char tianma_NT35590_para395[] = {0xB7,0x01};
static char tianma_NT35590_para396[] = {0xB8,0x95};
static char tianma_NT35590_para397[] = {0xB9,0x01};
static char tianma_NT35590_para398[] = {0xBA,0xA4};
static char tianma_NT35590_para399[] = {0xBB,0x01};
static char tianma_NT35590_para400[] = {0xBC,0xB2};
static char tianma_NT35590_para401[] = {0xBD,0x01};
static char tianma_NT35590_para402[] = {0xBE,0xC0};
static char tianma_NT35590_para403[] = {0xBF,0x01};
static char tianma_NT35590_para404[] = {0xC0,0xCB};
static char tianma_NT35590_para405[] = {0xC1,0x01};
static char tianma_NT35590_para406[] = {0xC2,0xE4};
static char tianma_NT35590_para407[] = {0xC3,0x02};
static char tianma_NT35590_para408[] = {0xC4,0x00};
static char tianma_NT35590_para409[] = {0xC5,0x02};
static char tianma_NT35590_para410[] = {0xC6,0x2B};
static char tianma_NT35590_para411[] = {0xC7,0x02};
static char tianma_NT35590_para412[] = {0xC8,0x4F};
static char tianma_NT35590_para413[] = {0xC9,0x02};
static char tianma_NT35590_para414[] = {0xCA,0x85};
static char tianma_NT35590_para415[] = {0xCB,0x02};
static char tianma_NT35590_para416[] = {0xCC,0xB2};
static char tianma_NT35590_para417[] = {0xCD,0x02};
static char tianma_NT35590_para418[] = {0xCE,0xB3};
static char tianma_NT35590_para419[] = {0xCF,0x02};
static char tianma_NT35590_para420[] = {0xD0,0xDA};
static char tianma_NT35590_para421[] = {0xD1,0x03};
static char tianma_NT35590_para422[] = {0xD2,0x05};
static char tianma_NT35590_para423[] = {0xD3,0x03};
static char tianma_NT35590_para424[] = {0xD4,0x1E};
static char tianma_NT35590_para425[] = {0xD5,0x03};
static char tianma_NT35590_para426[] = {0xD6,0x3C};
static char tianma_NT35590_para427[] = {0xD7,0x03};
static char tianma_NT35590_para428[] = {0xD8,0x51};
static char tianma_NT35590_para429[] = {0xD9,0x03};
static char tianma_NT35590_para430[] = {0xDA,0x6A};
static char tianma_NT35590_para431[] = {0xDB,0x03};
static char tianma_NT35590_para432[] = {0xDC,0x72};
static char tianma_NT35590_para433[] = {0xDD,0x03};
static char tianma_NT35590_para434[] = {0xDE,0x7D};
static char tianma_NT35590_para435[] = {0xDF,0x03};
static char tianma_NT35590_para436[] = {0xE0,0x8D};
static char tianma_NT35590_para437[] = {0xE1,0x03};
static char tianma_NT35590_para438[] = {0xE2,0x9C};
static char tianma_NT35590_para439[] = {0xE3,0x03};
static char tianma_NT35590_para440[] = {0xE4,0xA1};
static char tianma_NT35590_para441[] = {0xE5,0x03};
static char tianma_NT35590_para442[] = {0xE6,0xAD};
static char tianma_NT35590_para443[] = {0xE7,0x03};
static char tianma_NT35590_para444[] = {0xE8,0xBD};
static char tianma_NT35590_para445[] = {0xE9,0x03};
static char tianma_NT35590_para446[] = {0xEA,0xDE};
static char tianma_NT35590_para447[] = {0xFF,0x00};
static char tianma_NT35590_para448[] = {0xC2,0x03};
static char tianma_NT35590_para449[] = {0xBA,0x03};


static struct dsi_cmd_desc tianma_nt35590_display_on_cmds[] =  	
{ 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para001), tianma_NT35590_para001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para002), tianma_NT35590_para002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para003), tianma_NT35590_para003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para004), tianma_NT35590_para004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para005), tianma_NT35590_para005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para006), tianma_NT35590_para006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para007), tianma_NT35590_para007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para008), tianma_NT35590_para008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para009), tianma_NT35590_para009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para010), tianma_NT35590_para010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para011), tianma_NT35590_para011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para012), tianma_NT35590_para012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para013), tianma_NT35590_para013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para014), tianma_NT35590_para014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para015), tianma_NT35590_para015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para016), tianma_NT35590_para016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para017), tianma_NT35590_para017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para018), tianma_NT35590_para018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para019), tianma_NT35590_para019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para020), tianma_NT35590_para020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para021), tianma_NT35590_para021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para022), tianma_NT35590_para022},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para21), tianma_NT35590_para21},//add

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para023), tianma_NT35590_para023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para024), tianma_NT35590_para024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para025), tianma_NT35590_para025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para026), tianma_NT35590_para026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para027), tianma_NT35590_para027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para028), tianma_NT35590_para028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para029), tianma_NT35590_para029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para030), tianma_NT35590_para030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para031), tianma_NT35590_para031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para032), tianma_NT35590_para032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para033), tianma_NT35590_para033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para034), tianma_NT35590_para034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para035), tianma_NT35590_para035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para036), tianma_NT35590_para036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para037), tianma_NT35590_para037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para038), tianma_NT35590_para038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para039), tianma_NT35590_para039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para040), tianma_NT35590_para040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para041), tianma_NT35590_para041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para042), tianma_NT35590_para042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para043), tianma_NT35590_para043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para044), tianma_NT35590_para044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para045), tianma_NT35590_para045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para046), tianma_NT35590_para046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para047), tianma_NT35590_para047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para048), tianma_NT35590_para048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para049), tianma_NT35590_para049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para050), tianma_NT35590_para050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para051), tianma_NT35590_para051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para052), tianma_NT35590_para052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para053), tianma_NT35590_para053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para054), tianma_NT35590_para054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para055), tianma_NT35590_para055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para056), tianma_NT35590_para056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para057), tianma_NT35590_para057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para058), tianma_NT35590_para058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para059), tianma_NT35590_para059},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para060), tianma_NT35590_para060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para061), tianma_NT35590_para061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para062), tianma_NT35590_para062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para063), tianma_NT35590_para063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para064), tianma_NT35590_para064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para065), tianma_NT35590_para065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para066), tianma_NT35590_para066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para067), tianma_NT35590_para067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para068), tianma_NT35590_para068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para069), tianma_NT35590_para069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para070), tianma_NT35590_para070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para071), tianma_NT35590_para071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para072), tianma_NT35590_para072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para073), tianma_NT35590_para073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para074), tianma_NT35590_para074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para075), tianma_NT35590_para075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para076), tianma_NT35590_para076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para077), tianma_NT35590_para077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para078), tianma_NT35590_para078},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para079), tianma_NT35590_para079},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para080), tianma_NT35590_para080},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para081), tianma_NT35590_para081},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para082), tianma_NT35590_para082},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para083), tianma_NT35590_para083},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para084), tianma_NT35590_para084},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para085), tianma_NT35590_para085},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para086), tianma_NT35590_para086},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para087), tianma_NT35590_para087},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para088), tianma_NT35590_para088},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para089), tianma_NT35590_para089},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para090), tianma_NT35590_para090},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para091), tianma_NT35590_para091},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para092), tianma_NT35590_para092},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para093), tianma_NT35590_para093},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para094), tianma_NT35590_para094},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para095), tianma_NT35590_para095},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para096), tianma_NT35590_para096},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para097), tianma_NT35590_para097},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para098), tianma_NT35590_para098},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para099), tianma_NT35590_para099},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para100), tianma_NT35590_para100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para101), tianma_NT35590_para101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para102), tianma_NT35590_para102},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para103), tianma_NT35590_para103},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para104), tianma_NT35590_para104},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para105), tianma_NT35590_para105},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para106), tianma_NT35590_para106},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para107), tianma_NT35590_para107},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para108), tianma_NT35590_para108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para109), tianma_NT35590_para109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para110), tianma_NT35590_para110},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para111), tianma_NT35590_para111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para112), tianma_NT35590_para112},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para113), tianma_NT35590_para113},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para114), tianma_NT35590_para114},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para115), tianma_NT35590_para115},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para116), tianma_NT35590_para116},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para117), tianma_NT35590_para117},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para118), tianma_NT35590_para118},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para119), tianma_NT35590_para119},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para120), tianma_NT35590_para120},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para121), tianma_NT35590_para121},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para122), tianma_NT35590_para122},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para123), tianma_NT35590_para123},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para124), tianma_NT35590_para124},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para125), tianma_NT35590_para125},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para126), tianma_NT35590_para126},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para127), tianma_NT35590_para127},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para128), tianma_NT35590_para128},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para129), tianma_NT35590_para129},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para130), tianma_NT35590_para130},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para131), tianma_NT35590_para131},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para132), tianma_NT35590_para132},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para133), tianma_NT35590_para133},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para134), tianma_NT35590_para134},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para135), tianma_NT35590_para135},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para136), tianma_NT35590_para136},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para137), tianma_NT35590_para137},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para138), tianma_NT35590_para138},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para139), tianma_NT35590_para139},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para140), tianma_NT35590_para140},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para141), tianma_NT35590_para141},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para142), tianma_NT35590_para142},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para143), tianma_NT35590_para143},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para144), tianma_NT35590_para144},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para145), tianma_NT35590_para145},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para146), tianma_NT35590_para146},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para147), tianma_NT35590_para147},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para148), tianma_NT35590_para148},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para149), tianma_NT35590_para149},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para150), tianma_NT35590_para150},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para151), tianma_NT35590_para151},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para152), tianma_NT35590_para152},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para153), tianma_NT35590_para153},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para154), tianma_NT35590_para154},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para155), tianma_NT35590_para155},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para156), tianma_NT35590_para156},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para157), tianma_NT35590_para157},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para158), tianma_NT35590_para158},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para159), tianma_NT35590_para159},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para160), tianma_NT35590_para160},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para161), tianma_NT35590_para161},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para162), tianma_NT35590_para162},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para163), tianma_NT35590_para163},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para164), tianma_NT35590_para164},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para165), tianma_NT35590_para165},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para166), tianma_NT35590_para166},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para167), tianma_NT35590_para167},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para168), tianma_NT35590_para168},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para169), tianma_NT35590_para169},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para170), tianma_NT35590_para170},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para171), tianma_NT35590_para171},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para172), tianma_NT35590_para172},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para173), tianma_NT35590_para173},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para174), tianma_NT35590_para174},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para175), tianma_NT35590_para175},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para176), tianma_NT35590_para176},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para177), tianma_NT35590_para177},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para178), tianma_NT35590_para178},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para179), tianma_NT35590_para179},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para180), tianma_NT35590_para180},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para181), tianma_NT35590_para181},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para182), tianma_NT35590_para182},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para183), tianma_NT35590_para183},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para184), tianma_NT35590_para184},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para185), tianma_NT35590_para185},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para186), tianma_NT35590_para186},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para187), tianma_NT35590_para187},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para188), tianma_NT35590_para188},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para189), tianma_NT35590_para189},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para190), tianma_NT35590_para190},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para191), tianma_NT35590_para191},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para192), tianma_NT35590_para192},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para193), tianma_NT35590_para193},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para194), tianma_NT35590_para194},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para195), tianma_NT35590_para195},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para196), tianma_NT35590_para196},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para197), tianma_NT35590_para197},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para198), tianma_NT35590_para198},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para199), tianma_NT35590_para199},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para200), tianma_NT35590_para200},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para201), tianma_NT35590_para201},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para202), tianma_NT35590_para202},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para203), tianma_NT35590_para203},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para204), tianma_NT35590_para204},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para205), tianma_NT35590_para205},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para206), tianma_NT35590_para206},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para207), tianma_NT35590_para207},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para208), tianma_NT35590_para208},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para209), tianma_NT35590_para209},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para210), tianma_NT35590_para210},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para211), tianma_NT35590_para211},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para212), tianma_NT35590_para212},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para213), tianma_NT35590_para213},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para214), tianma_NT35590_para214},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para215), tianma_NT35590_para215},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para216), tianma_NT35590_para216},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para217), tianma_NT35590_para217},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para218), tianma_NT35590_para218},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para219), tianma_NT35590_para219},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para220), tianma_NT35590_para220},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para221), tianma_NT35590_para221},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para222), tianma_NT35590_para222},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para223), tianma_NT35590_para223},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para224), tianma_NT35590_para224},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para225), tianma_NT35590_para225},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para226), tianma_NT35590_para226},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para227), tianma_NT35590_para227},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para228), tianma_NT35590_para228},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para229), tianma_NT35590_para229},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para230), tianma_NT35590_para230},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para231), tianma_NT35590_para231},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para232), tianma_NT35590_para232},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para233), tianma_NT35590_para233},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para234), tianma_NT35590_para234},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para235), tianma_NT35590_para235},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para236), tianma_NT35590_para236},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para237), tianma_NT35590_para237},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para238), tianma_NT35590_para238},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para239), tianma_NT35590_para239},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para240), tianma_NT35590_para240},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para241), tianma_NT35590_para241},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para242), tianma_NT35590_para242},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para243), tianma_NT35590_para243},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para244), tianma_NT35590_para244},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para245), tianma_NT35590_para245},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para246), tianma_NT35590_para246},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para247), tianma_NT35590_para247},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para248), tianma_NT35590_para248},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para249), tianma_NT35590_para249},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para250), tianma_NT35590_para250},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para251), tianma_NT35590_para251},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para252), tianma_NT35590_para252},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para253), tianma_NT35590_para253},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para254), tianma_NT35590_para254},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para255), tianma_NT35590_para255},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para256), tianma_NT35590_para256},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para257), tianma_NT35590_para257},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para258), tianma_NT35590_para258},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para259), tianma_NT35590_para259},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para260), tianma_NT35590_para260},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para261), tianma_NT35590_para261},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para262), tianma_NT35590_para262},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para263), tianma_NT35590_para263},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para264), tianma_NT35590_para264},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para265), tianma_NT35590_para265},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para266), tianma_NT35590_para266},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para267), tianma_NT35590_para267},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para268), tianma_NT35590_para268},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para269), tianma_NT35590_para269},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para270), tianma_NT35590_para270},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para271), tianma_NT35590_para271},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para272), tianma_NT35590_para272},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para273), tianma_NT35590_para273},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para274), tianma_NT35590_para274},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para275), tianma_NT35590_para275},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para276), tianma_NT35590_para276},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para277), tianma_NT35590_para277},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para278), tianma_NT35590_para278},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para279), tianma_NT35590_para279},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para280), tianma_NT35590_para280},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para281), tianma_NT35590_para281},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para282), tianma_NT35590_para282},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para283), tianma_NT35590_para283},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para284), tianma_NT35590_para284},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para285), tianma_NT35590_para285},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para286), tianma_NT35590_para286},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para287), tianma_NT35590_para287},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para288), tianma_NT35590_para288},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para289), tianma_NT35590_para289},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para290), tianma_NT35590_para290},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para291), tianma_NT35590_para291},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para292), tianma_NT35590_para292},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para293), tianma_NT35590_para293},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para294), tianma_NT35590_para294},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para295), tianma_NT35590_para295},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para296), tianma_NT35590_para296},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para297), tianma_NT35590_para297},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para298), tianma_NT35590_para298},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para299), tianma_NT35590_para299},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para300), tianma_NT35590_para300},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para301), tianma_NT35590_para301},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para302), tianma_NT35590_para302},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para303), tianma_NT35590_para303},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para304), tianma_NT35590_para304},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para305), tianma_NT35590_para305},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para306), tianma_NT35590_para306},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para307), tianma_NT35590_para307},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para308), tianma_NT35590_para308},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para309), tianma_NT35590_para309},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para310), tianma_NT35590_para310},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para311), tianma_NT35590_para311},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para312), tianma_NT35590_para312},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para313), tianma_NT35590_para313},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para314), tianma_NT35590_para314},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para315), tianma_NT35590_para315},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para316), tianma_NT35590_para316},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para317), tianma_NT35590_para317},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para318), tianma_NT35590_para318},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para319), tianma_NT35590_para319},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para320), tianma_NT35590_para320},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para321), tianma_NT35590_para321},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para322), tianma_NT35590_para322},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para323), tianma_NT35590_para323},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para324), tianma_NT35590_para324},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para325), tianma_NT35590_para325},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para326), tianma_NT35590_para326},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para327), tianma_NT35590_para327},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para328), tianma_NT35590_para328},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para329), tianma_NT35590_para329},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para330), tianma_NT35590_para330},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para331), tianma_NT35590_para331},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para332), tianma_NT35590_para332},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para333), tianma_NT35590_para333},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para334), tianma_NT35590_para334},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para335), tianma_NT35590_para335},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para336), tianma_NT35590_para336},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para337), tianma_NT35590_para337},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para338), tianma_NT35590_para338},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para339), tianma_NT35590_para339},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para340), tianma_NT35590_para340},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para341), tianma_NT35590_para341},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para342), tianma_NT35590_para342},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para343), tianma_NT35590_para343},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para344), tianma_NT35590_para344},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para345), tianma_NT35590_para345},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para346), tianma_NT35590_para346},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para347), tianma_NT35590_para347},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para348), tianma_NT35590_para348},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para349), tianma_NT35590_para349},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para350), tianma_NT35590_para350},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para351), tianma_NT35590_para351},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para352), tianma_NT35590_para352},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para353), tianma_NT35590_para353},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para354), tianma_NT35590_para354},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para355), tianma_NT35590_para355},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para356), tianma_NT35590_para356},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para357), tianma_NT35590_para357},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para358), tianma_NT35590_para358},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para359), tianma_NT35590_para359},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para360), tianma_NT35590_para360},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para361), tianma_NT35590_para361},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para362), tianma_NT35590_para362},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para363), tianma_NT35590_para363},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para364), tianma_NT35590_para364},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para365), tianma_NT35590_para365},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para366), tianma_NT35590_para366},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para367), tianma_NT35590_para367},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para368), tianma_NT35590_para368},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para369), tianma_NT35590_para369},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para370), tianma_NT35590_para370},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para371), tianma_NT35590_para371},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para372), tianma_NT35590_para372},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para373), tianma_NT35590_para373},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para374), tianma_NT35590_para374},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para375), tianma_NT35590_para375},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para376), tianma_NT35590_para376},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para377), tianma_NT35590_para377},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para378), tianma_NT35590_para378},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para379), tianma_NT35590_para379},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para380), tianma_NT35590_para380},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para381), tianma_NT35590_para381},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para382), tianma_NT35590_para382},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para383), tianma_NT35590_para383},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para384), tianma_NT35590_para384},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para385), tianma_NT35590_para385},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para386), tianma_NT35590_para386},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para387), tianma_NT35590_para387},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para388), tianma_NT35590_para388},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para389), tianma_NT35590_para389},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para390), tianma_NT35590_para390},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para391), tianma_NT35590_para391},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para392), tianma_NT35590_para392},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para393), tianma_NT35590_para393},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para394), tianma_NT35590_para394},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para395), tianma_NT35590_para395},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para396), tianma_NT35590_para396},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para397), tianma_NT35590_para397},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para398), tianma_NT35590_para398},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para399), tianma_NT35590_para399},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para400), tianma_NT35590_para400},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para401), tianma_NT35590_para401},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para402), tianma_NT35590_para402},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para403), tianma_NT35590_para403},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para404), tianma_NT35590_para404},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para405), tianma_NT35590_para405},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para406), tianma_NT35590_para406},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para407), tianma_NT35590_para407},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para408), tianma_NT35590_para408},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para409), tianma_NT35590_para409},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para410), tianma_NT35590_para410},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para411), tianma_NT35590_para411},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para412), tianma_NT35590_para412},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para413), tianma_NT35590_para413},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para414), tianma_NT35590_para414},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para415), tianma_NT35590_para415},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para416), tianma_NT35590_para416},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para417), tianma_NT35590_para417},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para418), tianma_NT35590_para418},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para419), tianma_NT35590_para419},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para420), tianma_NT35590_para420},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para421), tianma_NT35590_para421},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para422), tianma_NT35590_para422},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para423), tianma_NT35590_para423},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para424), tianma_NT35590_para424},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para425), tianma_NT35590_para425},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para426), tianma_NT35590_para426},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para427), tianma_NT35590_para427},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para428), tianma_NT35590_para428},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para429), tianma_NT35590_para429},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para430), tianma_NT35590_para430},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para431), tianma_NT35590_para431},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para432), tianma_NT35590_para432},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para433), tianma_NT35590_para433},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para434), tianma_NT35590_para434},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para435), tianma_NT35590_para435},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para436), tianma_NT35590_para436},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para437), tianma_NT35590_para437},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para438), tianma_NT35590_para438},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para439), tianma_NT35590_para439},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para440), tianma_NT35590_para440},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para441), tianma_NT35590_para441},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para442), tianma_NT35590_para442},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para443), tianma_NT35590_para443},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para444), tianma_NT35590_para444},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para445), tianma_NT35590_para445},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para446), tianma_NT35590_para446},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para447), tianma_NT35590_para447},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para448), tianma_NT35590_para448},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_NT35590_para449), tianma_NT35590_para449},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep),exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},
};
static char cmi_cmd_FF[] = {0xFF, 0x12,0x80,0x01};
static char cmi_shift_FF_80[] = {0x00, 0x80};
static char cmi_cmd_FF_80[] = {0xFF, 0x12,0x80};
static char cmi_shift_B3_A0[] = {0x00,0xa0};
static char cmi_cmd_B3_A0[] = {0xb3,0x38,0x38};

static char cmi_cmd_truly_shift_b0[] = {0x00, 0xb0}; 
static char cmi_cmd_truly_cmd_b0[] = {0xb0, 0x20};

static char cmi_shift_CB_80[] = {0x00,0x80};
static char cmi_cmd_CB_80[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_CB_90[] = {0x00,0x90};
static char cmi_cmd_CB_90[] = {0xcb,0x00,0xc0,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_CB_A0[] = {0x00,0xa0};
static char cmi_cmd_CB_A0[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_CB_B0[] = {0x00,0xb0};
static char cmi_cmd_CB_B0[] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_CB_C0[] = {0x00,0xc0};
static char cmi_cmd_CB_C0[] = {0xcb,0x04,0x00,0x0f,0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x04,0xf4};
static char cmi_shift_CB_D0[] = {0x00,0xd0};
static char cmi_cmd_CB_D0[] = {0xcb,0xf4,0xf4,0x00,0xf4,0x08,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_CB_E0[] = {0x00,0xe0};
static char cmi_cmd_CB_E0[] = {0xcb,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00};
static char cmi_shift_CB_F0[] = {0x00,0xf0};
static char cmi_cmd_CB_F0[] = {0xcb,0x00,0x70,0x01,0x00,0x00};
static char cmi_shift_CC_80[] = {0x00,0x80};
static char cmi_cmd_CC_80[] = {0xcc,0x41,0x42,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x52,0x55,0x43,0x53,0x65,0x51,0x4D,0x4E,0x4F,0x91,0x8D,0x8E,0x8F,0x40,0x40,0x40,0x40};
static char cmi_shift_CC_A0[] = {0x00,0xA0};
static char cmi_cmd_CC_A0[] = {0xcc,0x41,0x42,0x47,0x48,0x4C,0x4B,0x4A,0x49,0x52,0x55,0x43,0x53,0x65,0x51,0x4D,0x4E,0x4F,0x91,0x8D,0x8E,0x8F,0x40,0x40,0x40,0x40,0xFF,0xFF,0xFF,0x01};
static char cmi_shift_CC_C0[] = {0x00,0xC0};
static char cmi_cmd_CC_C0[] = {0xcc,0x41,0x42,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x52,0x55,0x43,0x53,0x54,0x51,0x4D,0x4E,0x4F,0x91,0x8D,0x8E,0x8F,0x40,0x40,0x40,0x40};
static char cmi_shift_CC_E0[] = {0x00,0xE0};
static char cmi_cmd_CC_E0[] = {0xcc,0x41,0x42,0x47,0x48,0x4C,0x4B,0x4A,0x49,0x52,0x55,0x43,0x53,0x54,0x51,0x4D,0x4E,0x4F,0x91,0x8D,0x8E,0x8F,0x40,0x40,0x40,0x40,0xFF,0xFF,0xFF,0x01};
static char cmi_shift_C1_90[] = {0x00,0x90};
static char cmi_cmd_C1_90[] = {0xc1,0x22,0x00,0x00,0x00,0x00};
static char cmi_shift_C0_80[] = {0x00,0x80};
static char cmi_cmd_C0_80[] = {0xc0,0x00,0x87,0x00,0x06,0x0a,0x00,0x87,0x06,0x0a,0x00,0x00,0x00};
static char cmi_shift_C0_90[] = {0x00,0x90};
static char cmi_cmd_C0_90[] = {0xc0,0x00,0x0a,0x00,0x14,0x00,0x2a};
static char cmi_shift_C0_A0[] = {0x00,0xA0};
static char cmi_cmd_C0_A0[] = {0xc0,0x00,0x03,0x01,0x01,0x01,0x01,0x1a,0x03,0x00,0x02};
static char cmi_shift_C2_80[] = {0x00,0x80};
static char cmi_cmd_C2_80[] = {0xc2,0x03,0x02,0x00,0x00,0x00,0x02,0x00,0x22};
static char cmi_shift_C2_90[] = {0x00,0x90};
static char cmi_cmd_C2_90[] = {0xc2,0x03,0x00,0xff,0xff,0x00,0x00,0x00,0x00,0x22};
static char cmi_shift_C2_A0[] = {0x00,0xA0};
static char cmi_cmd_C2_A0[] = {0xc2,0xff,0x00,0xff,0x00,0x00,0x0a,0x00,0x0a};
static char cmi_shift_C2_B0[] = {0x00,0xB0};
static char cmi_cmd_C2_B0[] = {0xc2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_C2_C0[] = {0x00,0xC0};
static char cmi_cmd_C2_C0[] = {0xc2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char cmi_shift_C2_E0[] = {0x00,0xE0};
static char cmi_cmd_C2_E0[] = {0xc2,0x84,0x00,0x10,0x0d};
static char cmi_shift_C2_B3[] = {0x00,0xB3};
static char cmi_cmd_C2_B3[] = {0xc0,0x0f};
static char cmi_shift_C1_A2[] = {0x00,0xA2};
static char cmi_cmd_C1_A2[] = {0xc1,0xff};
static char cmi_shift_C0_B4[] = {0x00,0xB4};
static char cmi_cmd_C0_B4[] = {0xc0,0x54,0x00};
static char cmi_shift_C5_80[] = {0x00,0x80};
static char cmi_cmd_C5_80[] = {0xc5,0x20,0x07,0x00,0xb0,0xb0,0x00,0x08,0x00};
static char cmi_shift_C5_90[] = {0x00,0x90};
static char cmi_cmd_C5_90[] = {0xc5,0x20,0x85,0x02,0x88,0x96,0x15,0x00,0x08,0x44,0x44,0x44};
#ifdef COLOR_ENHANCE
static char cmi_cmd_D4_00[] = {0xD4,
						 0x00,0x40,0x00,0x40,0xff,0x40,0xff,0x40,0xfe,0x40,0xfe,0x40,0xfe,0x40,0xfd,0x40,0xfd,0x40,0xfc,0x40, 
						 0xfc,0x40,0xfb,0x40,0xfa,0x40,0xfa,0x40,0xf9,0x40,0xf9,0x40,0xf9,0x40,0xf9,0x40,0xfa,0x40,0xfa,0x40
};
static char cmi_cmd_D4_40[] = {0xD4,						 
						 0xfa,0x40,0xfa,0x40,0xfa,0x40,0xfc,0x40,0xfc,0x40,0xfc,0x40,0xfc,0x40,0xfc,0x40,0xfd,0x40,0xfd,0x40,
						 0xfd,0x40,0xfd,0x40,0xfc,0x40,0xfb,0x40,0xfa,0x40,0xfa,0x40,0xf9,0x40,0xf9,0x40,0xf8,0x40,0xf8,0x40
};
static char cmi_cmd_D4_80[] = {0xD4,							 
						 0xf7,0x3f,0xf7,0x3f,0xf6,0x3f,0xf6,0x3f,0xf5,0x3f,0xf5,0x3f,0xf6,0x3f,0xf7,0x3f,0xf7,0x3f,0xf8,0x40,
						 0xf9,0x40,0xf9,0x40,0xfa,0x40,0xfb,0x40,0xfc,0x40,0xfd,0x40,0xfe,0x40,0xfe,0x40,0xff,0x40,0x00,0x40
};
static char cmi_cmd_D4_120[] = {0xD4,							 
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x01,0x40,0x01,0x40,0x01,0x40
};
static char cmi_cmd_D4_160[] = {0xD4,							 
						 0x02,0x40,0x02,0x40,0x02,0x40,0x03,0x40,0x03,0x40,0x03,0x40,0x04,0x40,0x04,0x40,0x04,0x40,0x06,0x40,
						 0x06,0x40,0x06,0x40,0x06,0x40,0x07,0x40,0x07,0x40,0x07,0x40,0x07,0x40,0x07,0x40,0x08,0x40,0x08,0x40
};
static char cmi_cmd_D4_200[] = {0xD4,							 
						 0x08,0x40,0x08,0x40,0x08,0x40,0x09,0x3f,0x09,0x3f,0x09,0x3f,0x08,0x40,0x08,0x40,0x07,0x40,0x07,0x40,
						 0x06,0x40,0x06,0x40,0x04,0x40,0x04,0x40,0x03,0x40,0x03,0x40,0x02,0x40,0x02,0x40,0x01,0x40,0x01,0x40
};
static char cmi_cmd_D4_240[] = {0xD4,							 
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40
};
static char cmi_cmd_D4_280[] = {0xD4,							 
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40
};
static char cmi_cmd_D4_320[] = {0xD4,							 
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,
						 0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40,0x00,0x40
};
static char cmi_cmd_D5_00[] = {0xD5,
						 0x00,0x7f,0xff,0x7f,0xfe,0x7f,0xfe,0x7f,0xfc,0x7f,0xfc,0x7e,0xfb,0x7e,0xf9,0x7e,0xf9,0x7e,0xf7,0x7e,
						 0xf7,0x7d,0xf6,0x7d,0xf5,0x7d,0xf5,0x7d,0xf3,0x7c,0xf3,0x7c,0xf3,0x7c,0xf3,0x7d,0xf5,0x7d,0xf5,0x7d
};
static char cmi_cmd_D5_40[] = {0xD5,
						 0xf5,0x7d,0xf5,0x7d,0xf5,0x7e,0xf7,0x7e,0xf7,0x7e,0xf7,0x7e,0xf7,0x7e,0xf7,0x7f,0xf9,0x7f,0xf9,0x7f,
						 0xf9,0x7f,0xf8,0x7f,0xf7,0x7f,0xf6,0x7f,0xf5,0x7f,0xf4,0x7f,0xf3,0x7f,0xf2,0x7f,0xf0,0x7e,0xf0,0x7e
};
static char cmi_cmd_D5_80[] = {0xD5,						 
						 0xee,0x7e,0xee,0x7e,0xec,0x7e,0xec,0x7e,0xea,0x7d,0xea,0x7d,0xec,0x7e,0xed,0x7e,0xee,0x7e,0xf0,0x7e,
						 0xf2,0x7f,0xf3,0x7f,0xf5,0x7f,0xf6,0x7f,0xf7,0x7f,0xf9,0x7f,0xfb,0x7f,0xfc,0x7f,0xfe,0x7f,0xff,0x7f
};
static char cmi_cmd_D5_120[] = {0xD5,						 
						 0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,
						 0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x02,0x7f,0x02,0x7f,0x02,0x7f
};
static char cmi_cmd_D5_160[] = {0xD5,						 
						 0x04,0x7f,0x04,0x7f,0x04,0x7f,0x07,0x7f,0x07,0x7f,0x07,0x7f,0x09,0x7f,0x09,0x7f,0x09,0x7f,0x0b,0x7f,
						 0x0b,0x7f,0x0b,0x7f,0x0b,0x7f,0x0d,0x7f,0x0d,0x7f,0x0d,0x7f,0x0d,0x7f,0x0d,0x7f,0x10,0x7e,0x10,0x7e
};
static char cmi_cmd_D5_200[] = {0xD5,						 
						 0x10,0x7e,0x10,0x7e,0x10,0x7e,0x12,0x7e,0x12,0x7e,0x12,0x7e,0x10,0x7e,0x10,0x7e,0x0d,0x7f,0x0d,0x7f,
						 0x0b,0x7f,0x0b,0x7f,0x09,0x7f,0x08,0x7f,0x07,0x7f,0x06,0x7f,0x04,0x7f,0x03,0x7f,0x02,0x7f,0x01,0x7f
};
static char cmi_cmd_D5_240[] = {0xD5,						 
						 0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,
						 0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7f,0x00,0x7e,0x00,0x7e
};
static char cmi_cmd_D5_280[] = {0xD5,						 
						 0x00,0x7d,0x00,0x7d,0x00,0x7d,0x00,0x7c,0x00,0x7c,0x00,0x7c,0x00,0x7b,0x00,0x7b,0x00,0x7a,0x00,0x7a,
						 0x00,0x7a,0x00,0x79,0x00,0x78,0x00,0x77,0x00,0x76,0x00,0x75,0x00,0x74,0x00,0x74,0x00,0x73,0x00,0x72
};
static char cmi_cmd_D5_320[] = {0xD5,						 
						 0x00,0x71,0x00,0x70,0x00,0x6f,0x00,0x6f,0x00,0x6e,0x00,0x6d,0x00,0x6f,0x00,0x70,0x00,0x71,0x00,0x72,
						 0x00,0x74,0x00,0x75,0x00,0x76,0x00,0x77,0x00,0x79,0x00,0x7a,0x00,0x7b,0x00,0x7c,0x00,0x7e,0x00,0x7f
};
#endif
static char cmi_shift_D8_00[] = {0x00,0x00};
static char cmi_cmd_D8_00[] = {0xd8,0x52,0x00,0x52,0x00};
static char cmi_shift_D9_00[] = {0x00,0x00};
static char cmi_cmd_D9_00[] = {0xd9,0x8f,0x73,0x80};
static char cmi_shift_C0_C0[] = {0x00,0xC0};
static char cmi_cmd_C0_C0[] = {0xc0,0x95};
static char cmi_shift_C0_D0[] = {0x00,0xD0};
static char cmi_cmd_C0_D0[] = {0xc0,0x05};
static char cmi_shift_F5_B6[] = {0x00,0xB6};
static char cmi_cmd_F5_B6[] = {0xf5,0x00,0x00};
static char cmi_shift_B3_B0[] = {0x00,0xB0};
static char cmi_cmd_B3_B0[] = {0xb3,0x11};
static char cmi_shift_F5_B0[] = {0x00,0xB0};
static char cmi_cmd_F5_B0[] = {0xf5,0x00,0x20};
static char cmi_shift_F5_B8[] = {0x00,0xB8};
static char cmi_cmd_F5_B8[] = {0xf5,0x0c,0x12};
static char cmi_shift_F5_94[] = {0x00,0x94};
static char cmi_cmd_F5_94[] = {0xf5,0x0a,0x14,0x06,0x17};
static char cmi_shift_F5_A2[] = {0x00,0xA2};
static char cmi_cmd_F5_A2[] = {0xf5,0x0a,0x14,0x07,0x14};
static char cmi_shift_F5_90[] = {0x00,0x90};
static char cmi_cmd_F5_90[] = {0xf5,0x07,0x16,0x07,0x14};
static char cmi_shift_F5_A0[] = {0x00,0xA0};
static char cmi_cmd_F5_A0[] = {0xf5,0x02,0x12,0x0a,0x12,0x07,0x12,0x06,0x12,0x0b,0x12,0x08,0x12};
static char cmi_cmd_36[] = {0x36,0xd1};
//static char cmi_cmd_35[] = {0x35,0x00};
static char cmi_shift_D6_80[] = {0x00,0x80};
static char cmi_cmd_D6_80[] = {0xd6,0x00};
static char cmi_shift_E1_00[] = {0x00,0x00};
static char cmi_cmd_E1_00[] = {0xE1,0x2C,0x2F,0x36,0x3E,0x0B,0x05,0x14,0x09,0x07,0x08,0x09,0x1C,0x05,0x0B,0x11,0x0E,0x0B,0x0B};
static char cmi_shift_E2_00[] = {0x00,0x00};
static char cmi_cmd_E2_00[] = {0xE2,0x2C,0x2F,0x36,0x3E,0x0B,0x05,0x14,0x09,0x07,0x08,0x09,0x1C,0x05,0x0B,0x11,0x0E,0x0B,0x0B};
static char cmi_shift_E3_00[] = {0x00,0x00};
static char cmi_cmd_E3_00[] = {0xE3,0x2C,0x2E,0x35,0x3C,0x0D,0x06,0x16,0x09,0x07,0x08,0x0A,0x1A,0x05,0x0B,0x12,0x0E,0x0B,0x0B};
static char cmi_shift_E4_00[] = {0x00,0x00};
static char cmi_cmd_E4_00[] = {0xE4,0x2C,0x2E,0x35,0x3C,0x0D,0x06,0x16,0x09,0x07,0x08,0x0A,0x1A,0x05,0x0B,0x12,0x0E,0x0B,0x0B};
static char cmi_shift_E5_00[] = {0x00,0x00};
static char cmi_cmd_E5_00[] = {0xE5,0x0E,0x16,0x23,0x2E,0x0E,0x07,0x1C,0x0A,0x08,0x07,0x09,0x19,0x05,0x0C,0x11,0x0D,0x0B,0x0B};
static char cmi_shift_E6_00[] = {0x00,0x00};
static char cmi_cmd_E6_00[] = {0xE6,0x0E,0x16,0x23,0x2E,0x0E,0x07,0x1C,0x0A,0x08,0x07,0x09,0x19,0x05,0x0C,0x11,0x0D,0x0B,0x0B};

static char cmi_shift_2A_4P[]={0x00, 0x00};
static char cmi_cmd_2A_4P[]={0x2A, 0x00, 0x00, 0x02, 0xcf};
static char cmi_shift_2B_4P[]={0x00, 0x00};
static char cmi_cmd_2B_4P[]={0x2b, 0x00, 0x00, 0x04, 0xff};
static char cmi_shift_C1_80[] = {0x00, 0x80};
static char cmi_cmd_C1_80[] = {0xc1, 0x85};
static char cmi_shift_C5_D0[] = {0x00, 0xD0};
static char cmi_cmd_C5_D0[] = {0xc5,0x38};

static struct dsi_cmd_desc OTM1281_CMI_VIDEO_INIT_PARA[] = {
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_FF),cmi_cmd_FF},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_FF_80),cmi_shift_FF_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_FF_80),cmi_cmd_FF_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_B3_A0),cmi_shift_B3_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_B3_A0),cmi_cmd_B3_A0},

		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_cmd_truly_shift_b0),cmi_cmd_truly_shift_b0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_truly_cmd_b0),cmi_cmd_truly_cmd_b0},
		
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_80),cmi_shift_CB_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_80),cmi_cmd_CB_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_90),cmi_shift_CB_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_90),cmi_cmd_CB_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_A0),cmi_shift_CB_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_A0),cmi_cmd_CB_A0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_B0),cmi_shift_CB_B0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_B0),cmi_cmd_CB_B0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_C0),cmi_shift_CB_C0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_C0),cmi_cmd_CB_C0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_D0),cmi_shift_CB_D0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_D0),cmi_cmd_CB_D0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_E0),cmi_shift_CB_E0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_E0),cmi_cmd_CB_E0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CB_F0),cmi_shift_CB_F0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CB_F0),cmi_cmd_CB_F0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CC_80),cmi_shift_CC_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CC_80),cmi_cmd_CC_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CC_A0),cmi_shift_CC_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CC_A0),cmi_cmd_CC_A0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CC_C0),cmi_shift_CC_C0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CC_C0),cmi_cmd_CC_C0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_CC_E0),cmi_shift_CC_E0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_CC_E0),cmi_cmd_CC_E0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C1_90),cmi_shift_C1_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C1_90),cmi_cmd_C1_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_80),cmi_shift_C0_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_80),cmi_cmd_C0_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_90),cmi_shift_C0_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_90),cmi_cmd_C0_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_A0),cmi_shift_C0_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_A0),cmi_cmd_C0_A0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_80),cmi_shift_C2_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_80),cmi_cmd_C2_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_90),cmi_shift_C2_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_90),cmi_cmd_C2_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_A0),cmi_shift_C2_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_A0),cmi_cmd_C2_A0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_B0),cmi_shift_C2_B0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_B0),cmi_cmd_C2_B0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_C0),cmi_shift_C2_C0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_C0),cmi_cmd_C2_C0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_E0),cmi_shift_C2_E0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_E0),cmi_cmd_C2_E0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C2_B3),cmi_shift_C2_B3},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C2_B3),cmi_cmd_C2_B3},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C1_A2),cmi_shift_C1_A2},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C1_A2),cmi_cmd_C1_A2},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_B4),cmi_shift_C0_B4},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_B4),cmi_cmd_C0_B4},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C5_80),cmi_shift_C5_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C5_80),cmi_cmd_C5_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C5_90),cmi_shift_C5_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C5_90),cmi_cmd_C5_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_D8_00),cmi_shift_D8_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D8_00),cmi_cmd_D8_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_D9_00),cmi_shift_D9_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D9_00),cmi_cmd_D9_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_C0),cmi_shift_C0_C0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_C0),cmi_cmd_C0_C0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C0_D0),cmi_shift_C0_D0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C0_D0),cmi_cmd_C0_D0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_B6),cmi_shift_F5_B6},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_B6),cmi_cmd_F5_B6},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_B3_B0),cmi_shift_B3_B0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_B3_B0),cmi_cmd_B3_B0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_B0),cmi_shift_F5_B0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_B0),cmi_cmd_F5_B0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_B8),cmi_shift_F5_B8},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_B8),cmi_cmd_F5_B8},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_94),cmi_shift_F5_94},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_94),cmi_cmd_F5_94},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_A2),cmi_shift_F5_A2},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_A2),cmi_cmd_F5_A2},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_90),cmi_shift_F5_90},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_90),cmi_cmd_F5_90},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_F5_A0),cmi_shift_F5_A0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_F5_A0),cmi_cmd_F5_A0},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(otm_shift_00),otm_shift_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_cmd_36),cmi_cmd_36},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_D6_80),cmi_shift_D6_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_cmd_D6_80),cmi_cmd_D6_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E1_00),cmi_shift_E1_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E1_00),cmi_cmd_E1_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E2_00),cmi_shift_E2_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E2_00),cmi_cmd_E2_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E3_00),cmi_shift_E3_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E3_00),cmi_cmd_E3_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E4_00),cmi_shift_E4_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E4_00),cmi_cmd_E4_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E5_00),cmi_shift_E5_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E5_00),cmi_cmd_E5_00},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_E6_00),cmi_shift_E6_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_E6_00),cmi_cmd_E6_00},
		
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_2A_4P),cmi_shift_2A_4P},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_2A_4P), cmi_cmd_2A_4P},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_2B_4P),cmi_shift_2B_4P},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_2B_4P), cmi_cmd_2B_4P},


#ifdef COLOR_ENHANCE
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(truly_shift_83),truly_shift_83},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(truly_cmd_F4),truly_cmd_F4},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(otm_shift_00),otm_shift_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_00),cmi_cmd_D4_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_40),cmi_cmd_D4_40},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_80),cmi_cmd_D4_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_120),cmi_cmd_D4_120},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_160),cmi_cmd_D4_160},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_200),cmi_cmd_D4_200},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_240),cmi_cmd_D4_240},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_280),cmi_cmd_D4_280},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D4_320),cmi_cmd_D4_320},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(otm_shift_00),otm_shift_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_00),cmi_cmd_D5_00},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_40),cmi_cmd_D5_40},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_80),cmi_cmd_D5_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_120),cmi_cmd_D5_120},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_200),cmi_cmd_D5_200},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_160),cmi_cmd_D5_160},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_240),cmi_cmd_D5_240},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_280),cmi_cmd_D5_280},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_D5_320),cmi_cmd_D5_320},

#endif
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C1_80),cmi_shift_C1_80},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C1_80),cmi_cmd_C1_80},
		{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(cmi_shift_C5_D0),cmi_shift_C5_D0},
		{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(cmi_cmd_C5_D0),cmi_cmd_C5_D0},
		
			
		{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep),exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},
};


static struct dsi_cmd_desc novatek_cmd_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50,
		sizeof(sw_reset), sw_reset},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50,
		sizeof(novatek_f4), novatek_f4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(novatek_8c), novatek_8c},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50,
		sizeof(novatek_ff), novatek_ff},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(set_num_of_lanes), set_num_of_lanes},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(set_width), set_width},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50,
		sizeof(set_height), set_height},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10,
		sizeof(rgb_888), rgb_888},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(led_pwm2), led_pwm2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(led_pwm3), led_pwm3},
};

static struct dsi_cmd_desc novatek_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)
static char manufacture_id[2] = {0x04, 0x00}; /* DTYPE_DCS_READ */

static struct dsi_cmd_desc novatek_manufacture_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(manufacture_id), manufacture_id};

static u32 manu_id;

static void mipi_novatek_manufacture_cb(u32 data)
{
	manu_id = data;
	pr_info("%s: manufacture_id=%x\n", __func__, manu_id);
}

static uint32 mipi_novatek_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;

	cmdreq.cmds = &novatek_manufacture_id_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = 3;
	cmdreq.cb = mipi_novatek_manufacture_cb; /* call back */
	mipi_dsi_cmdlist_put(&cmdreq);
	/*
	 * blocked here, untill call back called
	 */

	return manu_id;
}

static int fpga_addr;
static int fpga_access_mode;
static bool support_3d;

static void mipi_novatek_3d_init(int addr, int mode)
{
	fpga_addr = addr;
	fpga_access_mode = mode;
}

static void mipi_dsi_enable_3d_barrier(int mode)
{
	void __iomem *fpga_ptr;
	uint32_t ptr_value = 0;

	if (!fpga_addr && support_3d) {
		pr_err("%s: fpga_addr not set. Failed to enable 3D barrier\n",
					__func__);
		return;
	}

	if (fpga_access_mode == FPGA_SPI_INTF) {
		if (mode == LANDSCAPE)
			novatek_fpga_write(fpga_addr, 1);
		else if (mode == PORTRAIT)
			novatek_fpga_write(fpga_addr, 3);
		else
			novatek_fpga_write(fpga_addr, 0);

		mb();
		novatek_fpga_read(fpga_addr);
	} else if (fpga_access_mode == FPGA_EBI2_INTF) {
		fpga_ptr = ioremap_nocache(fpga_addr, sizeof(uint32_t));
		if (!fpga_ptr) {
			pr_err("%s: FPGA ioremap failed."
				"Failed to enable 3D barrier\n",
						__func__);
			return;
		}

		ptr_value = readl_relaxed(fpga_ptr);
		if (mode == LANDSCAPE)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 1),
								fpga_ptr);
		else if (mode == PORTRAIT)
			writel_relaxed(((0xFFFF0000 & ptr_value) | 3),
								fpga_ptr);
		else
			writel_relaxed((0xFFFF0000 & ptr_value),
								fpga_ptr);

		mb();
		iounmap(fpga_ptr);
	} else
		pr_err("%s: 3D barrier not configured correctly\n",
					__func__);
}


static void lcd_panle_reset(void)
{	
	static int gpio_rst;
	gpio_rst = PM8921_GPIO_PM_TO_SYS(43);
	
	gpio_set_value_cansleep(gpio_rst, 1); /* disp enable */
	msleep(10);
	gpio_set_value_cansleep(gpio_rst, 0); /* disp enable */
	msleep(10);
	gpio_set_value_cansleep(gpio_rst, 1); /* disp enable */
	msleep(10);

}

static int first_time_panel_on = 1;

static int mipi_novatek_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct msm_panel_info *pinfo;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	pinfo = &mfd->panel_info;
	if (pinfo->is_3d_panel)
		support_3d = TRUE;

	mipi  = &mfd->panel_info.mipi;
	
	if(first_time_panel_on)
	{
		first_time_panel_on = 0;
		printk("\n first time lcd panel on ----return ");
		return 0;
	}
	
	printk("\n lcd panel on  ");
	
	lcd_panle_reset();
	if (mipi->mode == DSI_VIDEO_MODE) {

		//LcdPanleID = LCD_PANEL_4P3_NT35590_TIANMA;
		switch(LcdPanleID){
			case LCD_PANEL_4P3_OTM1281_OT_TRULY:
				
				mipi_dsi_cmds_tx(&novatek_tx_buf, truly_display_on_cmds,
								ARRAY_SIZE(truly_display_on_cmds));
				printk("\n TRULY OTM1281A AUO panel initialized!");
				break;
			case LCD_PANEL_4P3_OTM1281_OT_CMI:
				
				mipi_dsi_cmds_tx(&novatek_tx_buf, OTM1281_CMI_VIDEO_INIT_PARA,
								ARRAY_SIZE(OTM1281_CMI_VIDEO_INIT_PARA));
				printk("\n CMI OTM1281A panel initialized!");
				break;
				
			case LCD_PANEL_4P3_NT35590_TIANMA:
				mipi_dsi_cmds_tx(&novatek_tx_buf, tianma_nt35590_display_on_cmds,
								ARRAY_SIZE(tianma_nt35590_display_on_cmds));
				printk("\nzhangzailiang, TIANMA NT35590 panel initialized!");
				break;
			default:
				printk("OU!OU! NO PANEL...\n");
				break;
				
		}

	} else {
		cmdreq.cmds = novatek_cmd_on_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(novatek_cmd_on_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);

		/* clean up ack_err_status */
		mipi_dsi_cmd_bta_sw_trigger();
		mipi_novatek_manufacture_id(mfd);
	}
	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	//struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
#if 0
	cmdreq.cmds = novatek_display_off_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(novatek_display_off_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
#endif
	mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_display_off_cmds,
			ARRAY_SIZE(novatek_display_off_cmds));
	printk("\n LCD panel off");
	return 0;
}

DEFINE_LED_TRIGGER(bkl_led_trigger);
#if 0
static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1};
#endif


static bool onewiremode = false;
static int lcd_bkl_ctl=12;
void myudelay(unsigned int usec)
{
	udelay(usec);
}
static void select_1wire_mode(void)
{
	gpio_direction_output(lcd_bkl_ctl, 1);
	myudelay(120);
	gpio_direction_output(lcd_bkl_ctl, 0);
	myudelay(280);				////ZTE_LCD_LUYA_20100226_001
	gpio_direction_output(lcd_bkl_ctl, 1);
	myudelay(650);				////ZTE_LCD_LUYA_20100226_001
	
}


static void send_bkl_address(void)
{
	unsigned int i,j;
	i = 0x72;
	gpio_direction_output(lcd_bkl_ctl, 1);
	myudelay(10);
	printk("[LY] send_bkl_address \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(lcd_bkl_ctl, 0);
			myudelay(10);
			gpio_direction_output(lcd_bkl_ctl, 1);
			myudelay(180);
		}
		else
		{
			gpio_direction_output(lcd_bkl_ctl, 0);
			myudelay(180);
			gpio_direction_output(lcd_bkl_ctl, 1);
			myudelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(lcd_bkl_ctl, 0);
	myudelay(10);
	gpio_direction_output(lcd_bkl_ctl, 1);

}

static void send_bkl_data(int level)
{
	unsigned int i,j;
	i = level & 0x1F;
	gpio_direction_output(lcd_bkl_ctl, 1);
	myudelay(10);
	printk("[LY] send_bkl_data \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(lcd_bkl_ctl, 0);
			myudelay(10);
			gpio_direction_output(lcd_bkl_ctl, 1);
			myudelay(180);
		}
		else
		{
			gpio_direction_output(lcd_bkl_ctl, 0);
			myudelay(180);
			gpio_direction_output(lcd_bkl_ctl, 1);
			myudelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(lcd_bkl_ctl, 0);
	myudelay(10);
	gpio_direction_output(lcd_bkl_ctl, 1);

}
static void mipi_zte_set_backlight(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
	 int current_lel = mfd->bl_level;
  	 unsigned long flags;


    	printk("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);

    	if(!mfd->panel_power_on)
	{
    		gpio_direction_output(lcd_bkl_ctl, 0);
    		onewiremode = FALSE;
	    	return;
    	}

    	if(current_lel < 1)
    	{
        	current_lel = 0;
   	 }

    	if(current_lel > 28)
    	{
        	current_lel = 28;
    	}

    	local_irq_save(flags);
		
   	if(current_lel==0)
    	{
    		gpio_direction_output(lcd_bkl_ctl, 0);
		mdelay(3);
		onewiremode = FALSE;
			
    	}
    	else 
	{
		if(!onewiremode)	
		{
			printk("[LY] before select_1wire_mode\n");
			select_1wire_mode();
			onewiremode = TRUE;
		}
		send_bkl_address();
		send_bkl_data(current_lel-1);

	}
		
    	local_irq_restore(flags);
}




#if 0//back
static void mipi_novatek_set_backlight(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;

	if (mipi_novatek_pdata &&
	    mipi_novatek_pdata->gpio_set_backlight) {
		mipi_novatek_pdata->gpio_set_backlight(mfd->bl_level);
		return;
	}

	if ((mipi_novatek_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		return;
	}

	led_pwm1[1] = (unsigned char)mfd->bl_level;

	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT | CMD_CLK_CTRL;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
}
#endif
static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev);
static int barrier_mode;

static int __devinit mipi_novatek_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;
	struct platform_device *current_pdev;
	static struct mipi_dsi_phy_ctrl *phy_settings;
	static char dlane_swap;

	if (pdev->id == 0) {
		mipi_novatek_pdata = pdev->dev.platform_data;

		if (mipi_novatek_pdata
			&& mipi_novatek_pdata->phy_ctrl_settings) {
			phy_settings = (mipi_novatek_pdata->phy_ctrl_settings);
		}

		if (mipi_novatek_pdata
			&& mipi_novatek_pdata->dlane_swap) {
			dlane_swap = (mipi_novatek_pdata->dlane_swap);
		}

		if (mipi_novatek_pdata
			 && mipi_novatek_pdata->fpga_3d_config_addr)
			mipi_novatek_3d_init(mipi_novatek_pdata
	->fpga_3d_config_addr, mipi_novatek_pdata->fpga_ctrl_mode);

		/* create sysfs to control 3D barrier for the Sharp panel */
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) {
			pr_err("%s: Failed to register 3d Barrier sysfs\n",
						__func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) {
		mfd = platform_get_drvdata(current_pdev);
		if (!mfd)
			return -ENODEV;
		if (mfd->key != MFD_KEY)
			return -EINVAL;

		mipi  = &mfd->panel_info.mipi;

		if (phy_settings != NULL)
			mipi->dsi_phy_db = phy_settings;

		if (dlane_swap)
			mipi->dlane_swap = dlane_swap;
	}
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_novatek_lcd_probe,
	.driver = {
		.name   = "mipi_novatek",
	},
};

static struct msm_fb_panel_data novatek_panel_data = {
	.on		= mipi_novatek_lcd_on,
	.off		= mipi_novatek_lcd_off,
	.set_backlight = mipi_zte_set_backlight,//mipi_novatek_set_backlight,
};

static ssize_t mipi_dsi_3d_barrier_read(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return snprintf((char *)buf, sizeof(buf), "%u\n", barrier_mode);
}

static ssize_t mipi_dsi_3d_barrier_write(struct device *dev,
				struct device_attribute *attr,
				const char *buf,
				size_t count)
{
	int ret = -1;
	u32 data = 0;

	if (sscanf((char *)buf, "%u", &data) != 1) {
		dev_err(dev, "%s\n", __func__);
		ret = -EINVAL;
	} else {
		barrier_mode = data;
		if (data == 1)
			mipi_dsi_enable_3d_barrier(LANDSCAPE);
		else if (data == 2)
			mipi_dsi_enable_3d_barrier(PORTRAIT);
		else
			mipi_dsi_enable_3d_barrier(0);
	}

	return count;
}

static struct device_attribute mipi_dsi_3d_barrier_attributes[] = {
	__ATTR(enable_3d_barrier, 0664, mipi_dsi_3d_barrier_read,
					 mipi_dsi_3d_barrier_write),
};

static int mipi_dsi_3d_barrier_sysfs_register(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mipi_dsi_3d_barrier_attributes); i++)
		if (device_create_file(dev, mipi_dsi_3d_barrier_attributes + i))
			goto error;

	return 0;

error:
	for (; i >= 0 ; i--)
		device_remove_file(dev, mipi_dsi_3d_barrier_attributes + i);
	pr_err("%s: Unable to create interface\n", __func__);

	return -ENODEV;
}

static int ch_used[3];

int mipi_novatek_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_novatek_lcd_init();
	if (ret) {
		pr_err("mipi_novatek_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_novatek", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	novatek_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &novatek_panel_data,
		sizeof(novatek_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_novatek_lcd_init(void)
{
#ifdef CONFIG_SPI_QUP
	int ret;
	ret = spi_register_driver(&panel_3d_spi_driver);

	if (ret) {
		pr_err("%s: spi register failed: rc=%d\n", __func__, ret);
		platform_driver_unregister(&this_driver);
	} else
		pr_info("%s: SUCCESS (SPI)\n", __func__);
#endif

	led_trigger_register_simple("bkl_trigger", &bkl_led_trigger);
	pr_info("%s: SUCCESS (WLED TRIGGER)\n", __func__);
	wled_trigger_initialized = 1;

	mipi_dsi_buf_alloc(&novatek_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&novatek_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
