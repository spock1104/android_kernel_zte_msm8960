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


static struct mipi_dsi_panel_platform_data *mipi_novatek_pdata;

static struct dsi_buf novatek_tx_buf;
static struct dsi_buf novatek_rx_buf;
extern void mipi_set_tx_power_mode(int mode);

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
#if 0
static char r61408_truly_lg_para_0xb0[2]={0xB0,0x04}; 
static char r61408_truly_lg_para_0xb3[3]={0xB3,0x10,0x00}; 
static char r61408_truly_lg_para_0xbd[2]={0xbd,0x00}; 
static char r61408_truly_lg_para_0xc0[3]={0xc0,0x00,0x66};
static char r61408_truly_lg_para_0xc1[16]={0xc1,0x23,0x31,0x99,0x26,0x25,0x00,
	0x10,0x28,0x0c,0x0c,0x00,0x00,0x00,0x21,0x01};
static char r61408_truly_lg_para_0xc2[7]={0xc2,0x10,0x06,0x06,0x01,0x03,0x00};
static char r61408_truly_lg_para_0xc8[25]={0xc8,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xc9[25]={0xc9,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xca[25]={0xca,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xd0[17]={0xd0,0x29,0x03,0xce,0xa6,0x0c,0x43,
	0x20,0x10,0x01,0x00,0x01,0x01,0x00,0x03,0x01,0x00};
static char r61408_truly_lg_para_0xd1[8]={0xd1,0x18,0x0c,0x23,0x03,0x75,0x02,0x50};
static char r61408_truly_lg_para_0xd3[2]={0xd3,0x33};
static char r61408_truly_lg_para_0xd5[3]={0xd5,0x2a,0x2a};
static char r61408_truly_lg_para_0xde[3]={0xde,0x01,0x51};
static char r61408_truly_lg_para_0xe6[2]={0xe6,0x51};//vcomdc flick
static char r61408_truly_lg_para_0xfa[2]={0xfa,0x03};
static char r61408_truly_lg_para_0xd6[2]={0xd6,0x28};
static char r61408_truly_lg_para_0x2a[5]={0x2a,0x00,0x00,0x01,0xdf};
static char r61408_truly_lg_para_0x2b[5]={0x2b,0x00,0x00,0x03,0x1f};
static char r61408_truly_lg_para_0x36[2]={0x36,0x00};
static char r61408_truly_lg_para_0x3a[2]={0x3a,0x77};


static struct dsi_cmd_desc novatek_video_on_cmds[] = 
{
	
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xb0), r61408_truly_lg_para_0xb0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xb3), r61408_truly_lg_para_0xb3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xbd), r61408_truly_lg_para_0xbd},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc0), r61408_truly_lg_para_0xc0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc1), r61408_truly_lg_para_0xc1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc2), r61408_truly_lg_para_0xc2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc8), r61408_truly_lg_para_0xc8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc9), r61408_truly_lg_para_0xc9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xca), r61408_truly_lg_para_0xca},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd0), r61408_truly_lg_para_0xd0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd1), r61408_truly_lg_para_0xd1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd3), r61408_truly_lg_para_0xd3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd5), r61408_truly_lg_para_0xd5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xde), r61408_truly_lg_para_0xde},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xe6), r61408_truly_lg_para_0xe6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xfa), r61408_truly_lg_para_0xfa},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd6), r61408_truly_lg_para_0xd6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x2a), r61408_truly_lg_para_0x2a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x2b), r61408_truly_lg_para_0x2b},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x36), r61408_truly_lg_para_0x36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x3a), r61408_truly_lg_para_0x3a},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
#endif
#if 0
/**************************************
9. boe NT35510 start 
**************************************/
	static char boe_nt35510_para01[]={0xF0,0x55,0xAA,0x52,0x08,0x01};																																																									
	static char boe_nt35510_para02[]={0xB6,0x34,0x34,0x34}; 																																																													 
	static char boe_nt35510_para03[]={0xB0,0x09,0x09,0x09}; 																																																													 
	static char boe_nt35510_para04[]={0xB7,0x24,0x24,0x24}; 																																																													 
	static char boe_nt35510_para05[]={0xB1,0x09,0x09,0x09}; 																																																													 
	static char boe_nt35510_para06[]={0xB8,0x34};																																																													   
	static char boe_nt35510_para07[]={0xB2,0x00};																																																													   
	static char boe_nt35510_para08[]={0xB9,0x24,0x24,0x24}; 																																																													 
	static char boe_nt35510_para09[]={0xB3,0x05,0x05,0x05}; 																																																													 
	static char boe_nt35510_para10[]={0xBF,0x01};																																																													   
	static char boe_nt35510_para11[]={0xBA,0x34,0x34,0x34}; 																																																													 
	static char boe_nt35510_para12[]={0xB5,0x0B,0x0B,0x0B}; 																																																													 
	static char boe_nt35510_para13[]={0xBC,0x00,0xA3,0x00}; 																																																													 
	static char boe_nt35510_para14[]={0xBD,0x00,0xA3,0x00}; 																																																													 
	static char boe_nt35510_para15[]={0xBE,0x00,0x50};																																																														
	static char boe_nt35510_para16[]={0xD1,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para17[]={0xD2,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para18[]={0xD3,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para19[]={0xD4,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para20[]={0xD5,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para21[]={0xD6,0x00,0x37,0x00,0x52,0x00,0x7B,0x00,0x99,0x00,0xB1,0x00,0xD2,0x00,0xF6,0x01,0x27,0x01,0x4E,0x01,0x8C,0x01,0xBE,0x02,0x0B,0x02,0x48,0x02,0x4A,0x02,0x7E,0x02,0xBC,0x02,0xE1,0x03,0x10,0x03,0x31,0x03,0x5A,0x03,0x73,0x03,0x94,0x03,0x9F,0x03,0xB3,0x03,0xB9,0x03,0xC1}; 
	static char boe_nt35510_para22[]={0xF0,0x55,0xAA,0x52,0x08,0x00};																																																								   
	static char boe_nt35510_para23[]={0xB0,0x00,0x05,0x02,0x05,0x02};																																																								   
	static char boe_nt35510_para24[]={0xB6,0x0A};																																																													   
	static char boe_nt35510_para25[]={0xB7,0x00,0x00};																																																														
	static char boe_nt35510_para26[]={0xB8,0x01,0x05,0x05,0x05};																																																								  
	static char boe_nt35510_para27[]={0xBC,0x00,0x00,0x00}; 																																																													 
	static char boe_nt35510_para28[]={0xCC,0x03,0x00,0x00}; 																																																													 
	static char boe_nt35510_para29[]={0xBD,0x01,0x84,0x07,0x31,0x00};																																																								   
	static char boe_nt35510_para30[]={0xBA,0x01};																																																													   
	static char boe_nt35510_para31[]={0x3A,0x77};																																																													   
	static char boe_nt35510_para32[]={0xB4,0x10,0x00};																																																								   
	static char boe_nt35510_para33[]={0xB1,0xf8,0x00};

	static struct dsi_cmd_desc novatek_video_on_cmds[] = {

	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para01), boe_nt35510_para01}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para02), boe_nt35510_para02}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para03), boe_nt35510_para03}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para04), boe_nt35510_para04}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para05), boe_nt35510_para05}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para06), boe_nt35510_para06}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para07), boe_nt35510_para07}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para08), boe_nt35510_para08}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para09), boe_nt35510_para09}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para10), boe_nt35510_para10}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para11), boe_nt35510_para11}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para12), boe_nt35510_para12}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para13), boe_nt35510_para13}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para14), boe_nt35510_para14}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para15), boe_nt35510_para15}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para16), boe_nt35510_para16}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para17), boe_nt35510_para17}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para18), boe_nt35510_para18}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para19), boe_nt35510_para19}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para20), boe_nt35510_para20}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para21), boe_nt35510_para21}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para22), boe_nt35510_para22}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para23), boe_nt35510_para23}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para24), boe_nt35510_para24}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para25), boe_nt35510_para25}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para26), boe_nt35510_para26}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para27), boe_nt35510_para27}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para28), boe_nt35510_para28}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para29), boe_nt35510_para29}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para30), boe_nt35510_para30}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_nt35510_para31), boe_nt35510_para31}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para32), boe_nt35510_para32}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(boe_nt35510_para33), boe_nt35510_para33}, 
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
#endif

static char cmi_nt35590_para_add_01[]={0xFF,0xee};
static char cmi_nt35590_para_add_02[]={0x26,0x08};
static char cmi_nt35590_para_add_03[]={0x26,0x00};
static char cmi_nt35590_para_add_04[]={0xFF,0x00};
static struct dsi_cmd_desc cmi_nt35590_display_on_add_cmds[] = {

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para_add_01), cmi_nt35590_para_add_01},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(cmi_nt35590_para_add_02), cmi_nt35590_para_add_02},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para_add_03), cmi_nt35590_para_add_03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(cmi_nt35590_para_add_04), cmi_nt35590_para_add_04},
};

static char cmi_nt35590_para001[]={0xFF,0x00};
static char cmi_nt35590_para002[]={0xBA,0x03};
static char cmi_nt35590_para003[]={0xC2,0x03};
static char cmi_nt35590_para_color[]={0x55,0xb0};//color_enhancement  0x80 low;0x90 middle;0xa0 high
static char cmi_nt35590_para0031[]={0x3b,0x03,0x07,0x04,0x46,0x3c};//mode,(vbp+v_width)/2,vfp/2,(hbp+h_width),hfp
static char cmi_nt35590_para004[]={0xFF,0x01};
static char cmi_nt35590_para005[]={0x00,0x3A};
static char cmi_nt35590_para006[]={0x01,0x33};
static char cmi_nt35590_para007[]={0x02,0x53};
static char cmi_nt35590_para008[]={0x09,0x85};
static char cmi_nt35590_para009[]={0x0e,0x25};
static char cmi_nt35590_para010[]={0x0f,0x0a};
static char cmi_nt35590_para011[]={0x0b,0x97};
static char cmi_nt35590_para012[]={0x0c,0x97};
static char cmi_nt35590_para013[]={0x11,0x8a};
static char cmi_nt35590_para014[]={0x36,0x7b};
static char cmi_nt35590_para015[]={0x71,0x2c};
static char cmi_nt35590_para016[]={0xff,0x05};
static char cmi_nt35590_para017[]={0x01,0x00};
static char cmi_nt35590_para018[]={0x02,0x8D};
static char cmi_nt35590_para019[]={0x03,0x8D};
static char cmi_nt35590_para020[]={0x04,0x8D};
static char cmi_nt35590_para021[]={0x05,0x30};
static char cmi_nt35590_para022[]={0x06,0x33};
static char cmi_nt35590_para023[]={0x07,0x77};
static char cmi_nt35590_para024[]={0x08,0x00};
static char cmi_nt35590_para025[]={0x09,0x00};
static char cmi_nt35590_para026[]={0x0A,0x00};
static char cmi_nt35590_para027[]={0x0B,0x80};
static char cmi_nt35590_para028[]={0x0C,0xC8};
static char cmi_nt35590_para029[]={0x0D,0x00};
static char cmi_nt35590_para030[]={0x0E,0x1B};
static char cmi_nt35590_para031[]={0x0F,0x07};
static char cmi_nt35590_para032[]={0x10,0x57};
static char cmi_nt35590_para033[]={0x11,0x00};
static char cmi_nt35590_para034[]={0x12,0x00};
static char cmi_nt35590_para035[]={0x13,0x1E};
static char cmi_nt35590_para036[]={0x14,0x00};
static char cmi_nt35590_para037[]={0x15,0x1A};
static char cmi_nt35590_para038[]={0x16,0x05};
static char cmi_nt35590_para039[]={0x17,0x00};
static char cmi_nt35590_para040[]={0x18,0x1E};
static char cmi_nt35590_para041[]={0x19,0xFF};
static char cmi_nt35590_para042[]={0x1A,0x00};
static char cmi_nt35590_para043[]={0x1B,0xFC};
static char cmi_nt35590_para044[]={0x1C,0x80};
static char cmi_nt35590_para045[]={0x1D,0x00};
static char cmi_nt35590_para046[]={0x1E,0x10};
static char cmi_nt35590_para047[]={0x1F,0x77};
static char cmi_nt35590_para048[]={0x20,0x00};
static char cmi_nt35590_para049[]={0x21,0x00};
static char cmi_nt35590_para050[]={0x22,0x55};
static char cmi_nt35590_para051[]={0x23,0x0D};
static char cmi_nt35590_para052[]={0x31,0xA0};
static char cmi_nt35590_para053[]={0x32,0x00};
static char cmi_nt35590_para054[]={0x33,0xB8};
static char cmi_nt35590_para055[]={0x34,0xBB};
static char cmi_nt35590_para056[]={0x35,0x11};
static char cmi_nt35590_para057[]={0x36,0x01};
static char cmi_nt35590_para058[]={0x37,0x0B};
static char cmi_nt35590_para059[]={0x38,0x01};
static char cmi_nt35590_para060[]={0x39,0x0B};
static char cmi_nt35590_para061[]={0x44,0x08};
static char cmi_nt35590_para062[]={0x45,0x80};
static char cmi_nt35590_para063[]={0x46,0xCC};
static char cmi_nt35590_para064[]={0x47,0x04};
static char cmi_nt35590_para065[]={0x48,0x00};
static char cmi_nt35590_para066[]={0x49,0x00};
static char cmi_nt35590_para067[]={0x4A,0x01};
static char cmi_nt35590_para068[]={0x6C,0x03};
static char cmi_nt35590_para069[]={0x6D,0x03};
static char cmi_nt35590_para070[]={0x6E,0x2F};
static char cmi_nt35590_para071[]={0x43,0x00};
static char cmi_nt35590_para072[]={0x4B,0x23};
static char cmi_nt35590_para073[]={0x4C,0x01};
static char cmi_nt35590_para074[]={0x50,0x23};
static char cmi_nt35590_para075[]={0x51,0x01};
static char cmi_nt35590_para076[]={0x58,0x23};
static char cmi_nt35590_para077[]={0x59,0x01};
static char cmi_nt35590_para078[]={0x5D,0x23};
static char cmi_nt35590_para079[]={0x5E,0x01};
static char cmi_nt35590_para080[]={0x62,0x23};
static char cmi_nt35590_para081[]={0x63,0x01};
static char cmi_nt35590_para082[]={0x67,0x23};
static char cmi_nt35590_para083[]={0x68,0x01};
static char cmi_nt35590_para084[]={0x89,0x00};
static char cmi_nt35590_para085[]={0x8D,0x01};
static char cmi_nt35590_para086[]={0x8E,0x64};
static char cmi_nt35590_para087[]={0x8F,0x20};
static char cmi_nt35590_para088[]={0x97,0x8E};
static char cmi_nt35590_para089[]={0x82,0x8C};
static char cmi_nt35590_para090[]={0x83,0x02};
static char cmi_nt35590_para091[]={0xBB,0x0A};
static char cmi_nt35590_para092[]={0xBC,0x0A};
static char cmi_nt35590_para093[]={0x24,0x25};
static char cmi_nt35590_para094[]={0x25,0x55};
static char cmi_nt35590_para095[]={0x26,0x05};
static char cmi_nt35590_para096[]={0x27,0x23};
static char cmi_nt35590_para097[]={0x28,0x01};
static char cmi_nt35590_para098[]={0x29,0x31};
static char cmi_nt35590_para099[]={0x2A,0x5D};
static char cmi_nt35590_para100[]={0x2B,0x01};
static char cmi_nt35590_para101[]={0x2F,0x00};
static char cmi_nt35590_para102[]={0x30,0x10};
static char cmi_nt35590_para103[]={0xA7,0x12};
static char cmi_nt35590_para104[]={0x2D,0x03};
static char cmi_nt35590_para105[]={0xFF,0x01};
static char cmi_nt35590_para106[]={0x75,0x00};
static char cmi_nt35590_para107[]={0x76,0x42};
static char cmi_nt35590_para108[]={0x77,0x00};
static char cmi_nt35590_para109[]={0x78,0x56};
static char cmi_nt35590_para110[]={0x79,0x00};
static char cmi_nt35590_para111[]={0x7A,0x79};
static char cmi_nt35590_para112[]={0x7B,0x00};
static char cmi_nt35590_para113[]={0x7C,0x97};
static char cmi_nt35590_para114[]={0x7D,0x00};
static char cmi_nt35590_para115[]={0x7E,0xb1};
static char cmi_nt35590_para116[]={0x7F,0x00};
static char cmi_nt35590_para117[]={0x80,0xc8};
static char cmi_nt35590_para118[]={0x81,0x00};
static char cmi_nt35590_para119[]={0x82,0xdb};
static char cmi_nt35590_para120[]={0x83,0x00};
static char cmi_nt35590_para121[]={0x84,0xec};
static char cmi_nt35590_para122[]={0x85,0x00};
static char cmi_nt35590_para123[]={0x86,0xfb};
static char cmi_nt35590_para124[]={0x87,0x01};
static char cmi_nt35590_para125[]={0x88,0x26};
static char cmi_nt35590_para126[]={0x89,0x01};
static char cmi_nt35590_para127[]={0x8A,0x49};
static char cmi_nt35590_para128[]={0x8B,0x01};
static char cmi_nt35590_para129[]={0x8C,0x86};
static char cmi_nt35590_para130[]={0x8D,0x01};
static char cmi_nt35590_para131[]={0x8E,0xb3};
static char cmi_nt35590_para132[]={0x8F,0x01};
static char cmi_nt35590_para133[]={0x90,0xfc};
static char cmi_nt35590_para134[]={0x91,0x02};
static char cmi_nt35590_para135[]={0x92,0x37};
static char cmi_nt35590_para136[]={0x93,0x02};
static char cmi_nt35590_para137[]={0x94,0x39};
static char cmi_nt35590_para138[]={0x95,0x02};
static char cmi_nt35590_para139[]={0x96,0x6f};
static char cmi_nt35590_para140[]={0x97,0x02};
static char cmi_nt35590_para141[]={0x98,0xaa};
static char cmi_nt35590_para142[]={0x99,0x02};
static char cmi_nt35590_para143[]={0x9A,0xc9};
static char cmi_nt35590_para144[]={0x9B,0x02};
static char cmi_nt35590_para145[]={0x9C,0xfc};
static char cmi_nt35590_para146[]={0x9D,0x03};
static char cmi_nt35590_para147[]={0x9E,0x20};
static char cmi_nt35590_para148[]={0x9F,0x03};
static char cmi_nt35590_para149[]={0xA0,0x52};
static char cmi_nt35590_para150[]={0xA2,0x03};
static char cmi_nt35590_para151[]={0xA3,0x62};
static char cmi_nt35590_para152[]={0xA4,0x03};
static char cmi_nt35590_para153[]={0xA5,0x75};
static char cmi_nt35590_para154[]={0xA6,0x03};
static char cmi_nt35590_para155[]={0xA7,0x8a};
static char cmi_nt35590_para156[]={0xA9,0x03};
static char cmi_nt35590_para157[]={0xAA,0xa1};
static char cmi_nt35590_para158[]={0xAB,0x03};
static char cmi_nt35590_para159[]={0xAC,0xb5};
static char cmi_nt35590_para160[]={0xAD,0x03};
static char cmi_nt35590_para161[]={0xAE,0xc6};
static char cmi_nt35590_para162[]={0xAF,0x03};
static char cmi_nt35590_para163[]={0xB0,0xcf};
static char cmi_nt35590_para164[]={0xB1,0x03};
static char cmi_nt35590_para165[]={0xB2,0xd1};
static char cmi_nt35590_para166[]={0xB3,0x00};
static char cmi_nt35590_para167[]={0xB4,0x42};
static char cmi_nt35590_para168[]={0xB5,0x00};
static char cmi_nt35590_para169[]={0xB6,0x56};
static char cmi_nt35590_para170[]={0xB7,0x00};
static char cmi_nt35590_para171[]={0xB8,0x79};
static char cmi_nt35590_para172[]={0xB9,0x00};
static char cmi_nt35590_para173[]={0xBA,0x97};
static char cmi_nt35590_para174[]={0xBB,0x00};
static char cmi_nt35590_para175[]={0xBC,0xB1};
static char cmi_nt35590_para176[]={0xBD,0x00};
static char cmi_nt35590_para177[]={0xBE,0xC8};
static char cmi_nt35590_para178[]={0xBF,0x00};
static char cmi_nt35590_para179[]={0xC0,0xDB};
static char cmi_nt35590_para180[]={0xC1,0x00};
static char cmi_nt35590_para181[]={0xC2,0xEC};
static char cmi_nt35590_para182[]={0xC3,0x00};
static char cmi_nt35590_para183[]={0xC4,0xFB};
static char cmi_nt35590_para184[]={0xC5,0x01};
static char cmi_nt35590_para185[]={0xC6,0x26};
static char cmi_nt35590_para186[]={0xC7,0x01};
static char cmi_nt35590_para187[]={0xC8,0x49};
static char cmi_nt35590_para188[]={0xC9,0x01};
static char cmi_nt35590_para189[]={0xCA,0x86};
static char cmi_nt35590_para190[]={0xCB,0x01};
static char cmi_nt35590_para191[]={0xCC,0xB3};
static char cmi_nt35590_para192[]={0xCD,0x01};
static char cmi_nt35590_para193[]={0xCE,0xFC};
static char cmi_nt35590_para194[]={0xCF,0x02};
static char cmi_nt35590_para195[]={0xD0,0x37};
static char cmi_nt35590_para196[]={0xD1,0x02};
static char cmi_nt35590_para197[]={0xD2,0x39};
static char cmi_nt35590_para198[]={0xD3,0x02};
static char cmi_nt35590_para199[]={0xD4,0x6F};
static char cmi_nt35590_para200[]={0xD5,0x02};
static char cmi_nt35590_para201[]={0xD6,0xAA};
static char cmi_nt35590_para202[]={0xD7,0x02};
static char cmi_nt35590_para203[]={0xD8,0xC9};
static char cmi_nt35590_para204[]={0xD9,0x02};
static char cmi_nt35590_para205[]={0xDA,0xFC};
static char cmi_nt35590_para206[]={0xDB,0x03};
static char cmi_nt35590_para207[]={0xDC,0x20};
static char cmi_nt35590_para208[]={0xDD,0x03};
static char cmi_nt35590_para209[]={0xDE,0x52};
static char cmi_nt35590_para210[]={0xDF,0x03};
static char cmi_nt35590_para211[]={0xE0,0x62};
static char cmi_nt35590_para212[]={0xE1,0x03};
static char cmi_nt35590_para213[]={0xE2,0x75};
static char cmi_nt35590_para214[]={0xE3,0x03};
static char cmi_nt35590_para215[]={0xE4,0x8A};
static char cmi_nt35590_para216[]={0xE5,0x03};
static char cmi_nt35590_para217[]={0xE6,0xA1};
static char cmi_nt35590_para218[]={0xE7,0x03};
static char cmi_nt35590_para219[]={0xE8,0xB5};
static char cmi_nt35590_para220[]={0xE9,0x03};
static char cmi_nt35590_para221[]={0xEA,0xC6};
static char cmi_nt35590_para222[]={0xEB,0x03};
static char cmi_nt35590_para223[]={0xEC,0xCF};
static char cmi_nt35590_para224[]={0xED,0x03};
static char cmi_nt35590_para225[]={0xEE,0xD1};
static char cmi_nt35590_para226[]={0xEF,0x00};
static char cmi_nt35590_para227[]={0xF0,0x42};
static char cmi_nt35590_para228[]={0xF1,0x00};
static char cmi_nt35590_para229[]={0xF2,0x56};
static char cmi_nt35590_para230[]={0xF3,0x00};
static char cmi_nt35590_para231[]={0xF4,0x79};
static char cmi_nt35590_para232[]={0xF5,0x00};
static char cmi_nt35590_para233[]={0xF6,0x97};
static char cmi_nt35590_para234[]={0xF7,0x00};
static char cmi_nt35590_para235[]={0xF8,0xB1};
static char cmi_nt35590_para236[]={0xF9,0x00};
static char cmi_nt35590_para237[]={0xFA,0xC8};
static char cmi_nt35590_para238[]={0xFF,0x02};
static char cmi_nt35590_para239[]={0x00,0x00};
static char cmi_nt35590_para240[]={0x01,0xDB};
static char cmi_nt35590_para241[]={0x02,0x00};
static char cmi_nt35590_para242[]={0x03,0xEC};
static char cmi_nt35590_para243[]={0x04,0x00};
static char cmi_nt35590_para244[]={0x05,0xFB};
static char cmi_nt35590_para245[]={0x06,0x01};
static char cmi_nt35590_para246[]={0x07,0x26};
static char cmi_nt35590_para247[]={0x08,0x01};
static char cmi_nt35590_para248[]={0x09,0x49};
static char cmi_nt35590_para249[]={0x0A,0x01};
static char cmi_nt35590_para250[]={0x0B,0x86};
static char cmi_nt35590_para251[]={0x0C,0x01};
static char cmi_nt35590_para252[]={0x0D,0xB3};
static char cmi_nt35590_para253[]={0x0E,0x01};
static char cmi_nt35590_para254[]={0x0F,0xFC};
static char cmi_nt35590_para255[]={0x10,0x02};
static char cmi_nt35590_para256[]={0x11,0x37};
static char cmi_nt35590_para257[]={0x12,0x02};
static char cmi_nt35590_para258[]={0x13,0x39};
static char cmi_nt35590_para259[]={0x14,0x02};
static char cmi_nt35590_para260[]={0x15,0x6F};
static char cmi_nt35590_para261[]={0x16,0x02};
static char cmi_nt35590_para262[]={0x17,0xAA};
static char cmi_nt35590_para263[]={0x18,0x02};
static char cmi_nt35590_para264[]={0x19,0xC9};
static char cmi_nt35590_para265[]={0x1A,0x02};
static char cmi_nt35590_para266[]={0x1B,0xFC};
static char cmi_nt35590_para267[]={0x1C,0x03};
static char cmi_nt35590_para268[]={0x1D,0x20};
static char cmi_nt35590_para269[]={0x1E,0x03};
static char cmi_nt35590_para270[]={0x1F,0x52};
static char cmi_nt35590_para271[]={0x20,0x03};
static char cmi_nt35590_para272[]={0x21,0x62};
static char cmi_nt35590_para273[]={0x22,0x03};
static char cmi_nt35590_para274[]={0x23,0x75};
static char cmi_nt35590_para275[]={0x24,0x03};
static char cmi_nt35590_para276[]={0x25,0x8A};
static char cmi_nt35590_para277[]={0x26,0x03};
static char cmi_nt35590_para278[]={0x27,0xA1};
static char cmi_nt35590_para279[]={0x28,0x03};
static char cmi_nt35590_para280[]={0x29,0xB5};
static char cmi_nt35590_para281[]={0x2A,0x03};
static char cmi_nt35590_para282[]={0x2B,0xC6};
static char cmi_nt35590_para283[]={0x2D,0x03};
static char cmi_nt35590_para284[]={0x2F,0xCF};
static char cmi_nt35590_para285[]={0x30,0x03};
static char cmi_nt35590_para286[]={0x31,0xD1};
static char cmi_nt35590_para287[]={0x32,0x00};
static char cmi_nt35590_para288[]={0x33,0x42};
static char cmi_nt35590_para289[]={0x34,0x00};
static char cmi_nt35590_para290[]={0x35,0x56};
static char cmi_nt35590_para291[]={0x36,0x00};
static char cmi_nt35590_para292[]={0x37,0x79};
static char cmi_nt35590_para293[]={0x38,0x00};
static char cmi_nt35590_para294[]={0x39,0x97};
static char cmi_nt35590_para295[]={0x3A,0x00};
static char cmi_nt35590_para296[]={0x3B,0xB1};
static char cmi_nt35590_para297[]={0x3D,0x00};
static char cmi_nt35590_para298[]={0x3F,0xC8};
static char cmi_nt35590_para299[]={0x40,0x00};
static char cmi_nt35590_para300[]={0x41,0xDB};
static char cmi_nt35590_para301[]={0x42,0x00};
static char cmi_nt35590_para302[]={0x43,0xEC};
static char cmi_nt35590_para303[]={0x44,0x00};
static char cmi_nt35590_para304[]={0x45,0xFB};
static char cmi_nt35590_para305[]={0x46,0x01};
static char cmi_nt35590_para306[]={0x47,0x26};
static char cmi_nt35590_para307[]={0x48,0x01};
static char cmi_nt35590_para308[]={0x49,0x49};
static char cmi_nt35590_para309[]={0x4A,0x01};
static char cmi_nt35590_para310[]={0x4B,0x86};
static char cmi_nt35590_para311[]={0x4C,0x01};
static char cmi_nt35590_para312[]={0x4D,0xB3};
static char cmi_nt35590_para313[]={0x4E,0x01};
static char cmi_nt35590_para314[]={0x4F,0xFC};
static char cmi_nt35590_para315[]={0x50,0x02};
static char cmi_nt35590_para316[]={0x51,0x37};
static char cmi_nt35590_para317[]={0x52,0x02};
static char cmi_nt35590_para318[]={0x53,0x39};
static char cmi_nt35590_para319[]={0x54,0x02};
static char cmi_nt35590_para320[]={0x55,0x6F};
static char cmi_nt35590_para321[]={0x56,0x02};
static char cmi_nt35590_para322[]={0x58,0xAA};
static char cmi_nt35590_para323[]={0x59,0x02};
static char cmi_nt35590_para324[]={0x5A,0xC9};
static char cmi_nt35590_para325[]={0x5B,0x02};
static char cmi_nt35590_para326[]={0x5C,0xFC};
static char cmi_nt35590_para327[]={0x5D,0x03};
static char cmi_nt35590_para328[]={0x5E,0x20};
static char cmi_nt35590_para329[]={0x5F,0x03};
static char cmi_nt35590_para330[]={0x60,0x52};
static char cmi_nt35590_para331[]={0x61,0x03};
static char cmi_nt35590_para332[]={0x62,0x62};
static char cmi_nt35590_para333[]={0x63,0x03};
static char cmi_nt35590_para334[]={0x64,0x75};
static char cmi_nt35590_para335[]={0x65,0x03};
static char cmi_nt35590_para336[]={0x66,0x8A};
static char cmi_nt35590_para337[]={0x67,0x03};
static char cmi_nt35590_para338[]={0x68,0xA1};
static char cmi_nt35590_para339[]={0x69,0x03};
static char cmi_nt35590_para340[]={0x6A,0xB5};
static char cmi_nt35590_para341[]={0x6B,0x03};
static char cmi_nt35590_para342[]={0x6C,0xC6};
static char cmi_nt35590_para343[]={0x6D,0x03};
static char cmi_nt35590_para344[]={0x6E,0xCF};
static char cmi_nt35590_para345[]={0x6F,0x03};
static char cmi_nt35590_para346[]={0x70,0xD1};
static char cmi_nt35590_para347[]={0x71,0x00};
static char cmi_nt35590_para348[]={0x72,0x42};
static char cmi_nt35590_para349[]={0x73,0x00};
static char cmi_nt35590_para350[]={0x74,0x56};
static char cmi_nt35590_para351[]={0x75,0x00};
static char cmi_nt35590_para352[]={0x76,0x79};
static char cmi_nt35590_para353[]={0x77,0x00};
static char cmi_nt35590_para354[]={0x78,0x97};
static char cmi_nt35590_para355[]={0x79,0x00};
static char cmi_nt35590_para356[]={0x7A,0xB1};
static char cmi_nt35590_para357[]={0x7B,0x00};
static char cmi_nt35590_para358[]={0x7C,0xC8};
static char cmi_nt35590_para359[]={0x7D,0x00};
static char cmi_nt35590_para360[]={0x7E,0xDB};
static char cmi_nt35590_para361[]={0x7F,0x00};
static char cmi_nt35590_para362[]={0x80,0xEC};
static char cmi_nt35590_para363[]={0x81,0x00};
static char cmi_nt35590_para364[]={0x82,0xFB};
static char cmi_nt35590_para365[]={0x83,0x01};
static char cmi_nt35590_para366[]={0x84,0x26};
static char cmi_nt35590_para367[]={0x85,0x01};
static char cmi_nt35590_para368[]={0x86,0x49};
static char cmi_nt35590_para369[]={0x87,0x01};
static char cmi_nt35590_para370[]={0x88,0x86};
static char cmi_nt35590_para371[]={0x89,0x01};
static char cmi_nt35590_para372[]={0x8A,0xB3};
static char cmi_nt35590_para373[]={0x8B,0x01};
static char cmi_nt35590_para374[]={0x8C,0xFC};
static char cmi_nt35590_para375[]={0x8D,0x02};
static char cmi_nt35590_para376[]={0x8E,0x37};
static char cmi_nt35590_para377[]={0x8F,0x02};
static char cmi_nt35590_para378[]={0x90,0x39};
static char cmi_nt35590_para379[]={0x91,0x02};
static char cmi_nt35590_para380[]={0x92,0x6F};
static char cmi_nt35590_para381[]={0x93,0x02};
static char cmi_nt35590_para382[]={0x94,0xAA};
static char cmi_nt35590_para383[]={0x95,0x02};
static char cmi_nt35590_para384[]={0x96,0xC9};
static char cmi_nt35590_para385[]={0x97,0x02};
static char cmi_nt35590_para386[]={0x98,0xFC};
static char cmi_nt35590_para387[]={0x99,0x03};
static char cmi_nt35590_para388[]={0x9A,0x20};
static char cmi_nt35590_para389[]={0x9B,0x03};
static char cmi_nt35590_para390[]={0x9C,0x52};
static char cmi_nt35590_para391[]={0x9D,0x03};
static char cmi_nt35590_para392[]={0x9E,0x62};
static char cmi_nt35590_para393[]={0x9F,0x03};
static char cmi_nt35590_para394[]={0xA0,0x75};
static char cmi_nt35590_para395[]={0xA2,0x03};
static char cmi_nt35590_para396[]={0xA3,0x8A};
static char cmi_nt35590_para397[]={0xA4,0x03};
static char cmi_nt35590_para398[]={0xA5,0xA1};
static char cmi_nt35590_para399[]={0xA6,0x03};
static char cmi_nt35590_para400[]={0xA7,0xB5};
static char cmi_nt35590_para401[]={0xA9,0x03};
static char cmi_nt35590_para402[]={0xAA,0xC6};
static char cmi_nt35590_para403[]={0xAB,0x03};
static char cmi_nt35590_para404[]={0xAC,0xCF};
static char cmi_nt35590_para405[]={0xAD,0x03};
static char cmi_nt35590_para406[]={0xAE,0xD1};
static char cmi_nt35590_para407[]={0xAF,0x00};
static char cmi_nt35590_para408[]={0xB0,0x42};
static char cmi_nt35590_para409[]={0xB1,0x00};
static char cmi_nt35590_para410[]={0xB2,0x56};
static char cmi_nt35590_para411[]={0xB3,0x00};
static char cmi_nt35590_para412[]={0xB4,0x79};
static char cmi_nt35590_para413[]={0xB5,0x00};
static char cmi_nt35590_para414[]={0xB6,0x97};
static char cmi_nt35590_para415[]={0xB7,0x00};
static char cmi_nt35590_para416[]={0xB8,0xB1};
static char cmi_nt35590_para417[]={0xB9,0x00};
static char cmi_nt35590_para418[]={0xBA,0xC8};
static char cmi_nt35590_para419[]={0xBB,0x00};
static char cmi_nt35590_para420[]={0xBC,0xDB};
static char cmi_nt35590_para421[]={0xBD,0x00};
static char cmi_nt35590_para422[]={0xBE,0xEC};
static char cmi_nt35590_para423[]={0xBF,0x00};
static char cmi_nt35590_para424[]={0xC0,0xFB};
static char cmi_nt35590_para425[]={0xC1,0x01};
static char cmi_nt35590_para426[]={0xC2,0x26};
static char cmi_nt35590_para427[]={0xC3,0x01};
static char cmi_nt35590_para428[]={0xC4,0x49};
static char cmi_nt35590_para429[]={0xC5,0x01};
static char cmi_nt35590_para430[]={0xC6,0x86};
static char cmi_nt35590_para431[]={0xC7,0x01};
static char cmi_nt35590_para432[]={0xC8,0xB3};
static char cmi_nt35590_para433[]={0xC9,0x01};
static char cmi_nt35590_para434[]={0xCA,0xFC};
static char cmi_nt35590_para435[]={0xCB,0x02};
static char cmi_nt35590_para436[]={0xCC,0x37};
static char cmi_nt35590_para437[]={0xCD,0x02};
static char cmi_nt35590_para438[]={0xCE,0x39};
static char cmi_nt35590_para439[]={0xCF,0x02};
static char cmi_nt35590_para440[]={0xD0,0x6F};
static char cmi_nt35590_para441[]={0xD1,0x02};
static char cmi_nt35590_para442[]={0xD2,0xAA};
static char cmi_nt35590_para443[]={0xD3,0x02};
static char cmi_nt35590_para444[]={0xD4,0xC9};
static char cmi_nt35590_para445[]={0xD5,0x02};
static char cmi_nt35590_para446[]={0xD6,0xFC};
static char cmi_nt35590_para447[]={0xD7,0x03};
static char cmi_nt35590_para448[]={0xD8,0x20};
static char cmi_nt35590_para449[]={0xD9,0x03};
static char cmi_nt35590_para450[]={0xDA,0x52};
static char cmi_nt35590_para451[]={0xDB,0x03};
static char cmi_nt35590_para452[]={0xDC,0x62};
static char cmi_nt35590_para453[]={0xDD,0x03};
static char cmi_nt35590_para454[]={0xDE,0x75};
static char cmi_nt35590_para455[]={0xDF,0x03};
static char cmi_nt35590_para456[]={0xE0,0x8A};
static char cmi_nt35590_para457[]={0xE1,0x03};
static char cmi_nt35590_para458[]={0xE2,0xA1};
static char cmi_nt35590_para459[]={0xE3,0x03};
static char cmi_nt35590_para460[]={0xE4,0xB5};
static char cmi_nt35590_para461[]={0xE5,0x03};
static char cmi_nt35590_para462[]={0xE6,0xC6};
static char cmi_nt35590_para463[]={0xE7,0x03};
static char cmi_nt35590_para464[]={0xE8,0xCF};
static char cmi_nt35590_para465[]={0xE9,0x03};
static char cmi_nt35590_para466[]={0xEA,0xD1};
static char cmi_nt35590_para467[]={0xFF,0x00};
static char cmi_nt35590_para468[]={0xFB,0x01};
static char cmi_nt35590_para469[]={0xFF,0x01};
static char cmi_nt35590_para470[]={0xFB,0x01};
static char cmi_nt35590_para471[]={0xFF,0x02};
static char cmi_nt35590_para472[]={0xFB,0x01};
static char cmi_nt35590_para473[]={0xFF,0x03};
static char cmi_nt35590_para474[]={0xFB,0x01};
static char cmi_nt35590_para475[]={0xFF,0x04};
static char cmi_nt35590_para476[]={0xFB,0x01};
static char cmi_nt35590_para477[]={0xFF,0x05};
static char cmi_nt35590_para478[]={0xFB,0x01};
static char cmi_nt35590_para479[]={0xFF,0x00};
//static char cmi_nt35590_para480[]={0x51,0xFF};
//static char cmi_nt35590_para481[]={0x53,0x2C};
//static char cmi_nt35590_para482[]={0x55,0x00};
static char cmi_nt35590_para483[]={0xFF,0x00};
static char cmi_nt35590_para484[]={0x35,0x00};


static struct dsi_cmd_desc cmi_nt35590_display_on_cmds[] = {

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para001), cmi_nt35590_para001},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para002), cmi_nt35590_para002},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para003), cmi_nt35590_para003},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para_color), cmi_nt35590_para_color},//color enhancement
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(cmi_nt35590_para0031), cmi_nt35590_para0031},//porch setting
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para004), cmi_nt35590_para004},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para005), cmi_nt35590_para005},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para006), cmi_nt35590_para006},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para007), cmi_nt35590_para007},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para008), cmi_nt35590_para008},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para009), cmi_nt35590_para009},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para010), cmi_nt35590_para010},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para011), cmi_nt35590_para011},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para012), cmi_nt35590_para012},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para013), cmi_nt35590_para013},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para014), cmi_nt35590_para014},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para015), cmi_nt35590_para015},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para016), cmi_nt35590_para016},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para017), cmi_nt35590_para017},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para018), cmi_nt35590_para018},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para019), cmi_nt35590_para019},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para020), cmi_nt35590_para020},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para021), cmi_nt35590_para021},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para022), cmi_nt35590_para022},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para023), cmi_nt35590_para023},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para024), cmi_nt35590_para024},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para025), cmi_nt35590_para025},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para026), cmi_nt35590_para026},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para027), cmi_nt35590_para027},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para028), cmi_nt35590_para028},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para029), cmi_nt35590_para029},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para030), cmi_nt35590_para030},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para031), cmi_nt35590_para031},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para032), cmi_nt35590_para032},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para033), cmi_nt35590_para033},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para034), cmi_nt35590_para034},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para035), cmi_nt35590_para035},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para036), cmi_nt35590_para036},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para037), cmi_nt35590_para037},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para038), cmi_nt35590_para038},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para039), cmi_nt35590_para039},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para040), cmi_nt35590_para040},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para041), cmi_nt35590_para041},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para042), cmi_nt35590_para042},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para043), cmi_nt35590_para043},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para044), cmi_nt35590_para044},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para045), cmi_nt35590_para045},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para046), cmi_nt35590_para046},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para047), cmi_nt35590_para047},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para048), cmi_nt35590_para048},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para049), cmi_nt35590_para049},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para050), cmi_nt35590_para050},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para051), cmi_nt35590_para051},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para052), cmi_nt35590_para052},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para053), cmi_nt35590_para053},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para054), cmi_nt35590_para054},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para055), cmi_nt35590_para055},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para056), cmi_nt35590_para056},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para057), cmi_nt35590_para057},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para058), cmi_nt35590_para058},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para059), cmi_nt35590_para059},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para060), cmi_nt35590_para060},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para061), cmi_nt35590_para061},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para062), cmi_nt35590_para062},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para063), cmi_nt35590_para063},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para064), cmi_nt35590_para064},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para065), cmi_nt35590_para065},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para066), cmi_nt35590_para066},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para067), cmi_nt35590_para067},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para068), cmi_nt35590_para068},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para069), cmi_nt35590_para069},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para070), cmi_nt35590_para070},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para071), cmi_nt35590_para071},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para072), cmi_nt35590_para072},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para073), cmi_nt35590_para073},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para074), cmi_nt35590_para074},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para075), cmi_nt35590_para075},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para076), cmi_nt35590_para076},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para077), cmi_nt35590_para077},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para078), cmi_nt35590_para078},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para079), cmi_nt35590_para079},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para080), cmi_nt35590_para080},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para081), cmi_nt35590_para081},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para082), cmi_nt35590_para082},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para083), cmi_nt35590_para083},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para084), cmi_nt35590_para084},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para085), cmi_nt35590_para085},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para086), cmi_nt35590_para086},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para087), cmi_nt35590_para087},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para088), cmi_nt35590_para088},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para089), cmi_nt35590_para089},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para090), cmi_nt35590_para090},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para091), cmi_nt35590_para091},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para092), cmi_nt35590_para092},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para093), cmi_nt35590_para093},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para094), cmi_nt35590_para094},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para095), cmi_nt35590_para095},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para096), cmi_nt35590_para096},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para097), cmi_nt35590_para097},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para098), cmi_nt35590_para098},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para099), cmi_nt35590_para099},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para100), cmi_nt35590_para100},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para101), cmi_nt35590_para101},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para102), cmi_nt35590_para102},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para103), cmi_nt35590_para103},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para104), cmi_nt35590_para104},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para105), cmi_nt35590_para105},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para106), cmi_nt35590_para106},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para107), cmi_nt35590_para107},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para108), cmi_nt35590_para108},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para109), cmi_nt35590_para109},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para110), cmi_nt35590_para110},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para111), cmi_nt35590_para111},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para112), cmi_nt35590_para112},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para113), cmi_nt35590_para113},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para114), cmi_nt35590_para114},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para115), cmi_nt35590_para115},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para116), cmi_nt35590_para116},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para117), cmi_nt35590_para117},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para118), cmi_nt35590_para118},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para119), cmi_nt35590_para119},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para120), cmi_nt35590_para120},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para121), cmi_nt35590_para121},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para122), cmi_nt35590_para122},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para123), cmi_nt35590_para123},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para124), cmi_nt35590_para124},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para125), cmi_nt35590_para125},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para126), cmi_nt35590_para126},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para127), cmi_nt35590_para127},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para128), cmi_nt35590_para128},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para129), cmi_nt35590_para129},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para130), cmi_nt35590_para130},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para131), cmi_nt35590_para131},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para132), cmi_nt35590_para132},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para133), cmi_nt35590_para133},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para134), cmi_nt35590_para134},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para135), cmi_nt35590_para135},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para136), cmi_nt35590_para136},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para137), cmi_nt35590_para137},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para138), cmi_nt35590_para138},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para139), cmi_nt35590_para139},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para140), cmi_nt35590_para140},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para141), cmi_nt35590_para141},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para142), cmi_nt35590_para142},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para143), cmi_nt35590_para143},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para144), cmi_nt35590_para144},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para145), cmi_nt35590_para145},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para146), cmi_nt35590_para146},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para147), cmi_nt35590_para147},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para148), cmi_nt35590_para148},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para149), cmi_nt35590_para149},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para150), cmi_nt35590_para150},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para151), cmi_nt35590_para151},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para152), cmi_nt35590_para152},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para153), cmi_nt35590_para153},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para154), cmi_nt35590_para154},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para155), cmi_nt35590_para155},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para156), cmi_nt35590_para156},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para157), cmi_nt35590_para157},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para158), cmi_nt35590_para158},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para159), cmi_nt35590_para159},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para160), cmi_nt35590_para160},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para161), cmi_nt35590_para161},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para162), cmi_nt35590_para162},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para163), cmi_nt35590_para163},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para164), cmi_nt35590_para164},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para165), cmi_nt35590_para165},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para166), cmi_nt35590_para166},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para167), cmi_nt35590_para167},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para168), cmi_nt35590_para168},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para169), cmi_nt35590_para169},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para170), cmi_nt35590_para170},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para171), cmi_nt35590_para171},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para172), cmi_nt35590_para172},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para173), cmi_nt35590_para173},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para174), cmi_nt35590_para174},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para175), cmi_nt35590_para175},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para176), cmi_nt35590_para176},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para177), cmi_nt35590_para177},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para178), cmi_nt35590_para178},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para179), cmi_nt35590_para179},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para180), cmi_nt35590_para180},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para181), cmi_nt35590_para181},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para182), cmi_nt35590_para182},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para183), cmi_nt35590_para183},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para184), cmi_nt35590_para184},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para185), cmi_nt35590_para185},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para186), cmi_nt35590_para186},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para187), cmi_nt35590_para187},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para188), cmi_nt35590_para188},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para189), cmi_nt35590_para189},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para190), cmi_nt35590_para190},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para191), cmi_nt35590_para191},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para192), cmi_nt35590_para192},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para193), cmi_nt35590_para193},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para194), cmi_nt35590_para194},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para195), cmi_nt35590_para195},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para196), cmi_nt35590_para196},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para197), cmi_nt35590_para197},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para198), cmi_nt35590_para198},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para199), cmi_nt35590_para199},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para200), cmi_nt35590_para200},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para201), cmi_nt35590_para201},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para202), cmi_nt35590_para202},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para203), cmi_nt35590_para203},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para204), cmi_nt35590_para204},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para205), cmi_nt35590_para205},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para206), cmi_nt35590_para206},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para207), cmi_nt35590_para207},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para208), cmi_nt35590_para208},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para209), cmi_nt35590_para209},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para210), cmi_nt35590_para210},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para211), cmi_nt35590_para211},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para212), cmi_nt35590_para212},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para213), cmi_nt35590_para213},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para214), cmi_nt35590_para214},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para215), cmi_nt35590_para215},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para216), cmi_nt35590_para216},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para217), cmi_nt35590_para217},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para218), cmi_nt35590_para218},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para219), cmi_nt35590_para219},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para220), cmi_nt35590_para220},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para221), cmi_nt35590_para221},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para222), cmi_nt35590_para222},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para223), cmi_nt35590_para223},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para224), cmi_nt35590_para224},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para225), cmi_nt35590_para225},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para226), cmi_nt35590_para226},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para227), cmi_nt35590_para227},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para228), cmi_nt35590_para228},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para229), cmi_nt35590_para229},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para230), cmi_nt35590_para230},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para231), cmi_nt35590_para231},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para232), cmi_nt35590_para232},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para233), cmi_nt35590_para233},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para234), cmi_nt35590_para234},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para235), cmi_nt35590_para235},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para236), cmi_nt35590_para236},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para237), cmi_nt35590_para237},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para238), cmi_nt35590_para238},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para239), cmi_nt35590_para239},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para240), cmi_nt35590_para240},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para241), cmi_nt35590_para241},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para242), cmi_nt35590_para242},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para243), cmi_nt35590_para243},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para244), cmi_nt35590_para244},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para245), cmi_nt35590_para245},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para246), cmi_nt35590_para246},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para247), cmi_nt35590_para247},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para248), cmi_nt35590_para248},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para249), cmi_nt35590_para249},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para250), cmi_nt35590_para250},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para251), cmi_nt35590_para251},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para252), cmi_nt35590_para252},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para253), cmi_nt35590_para253},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para254), cmi_nt35590_para254},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para255), cmi_nt35590_para255},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para256), cmi_nt35590_para256},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para257), cmi_nt35590_para257},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para258), cmi_nt35590_para258},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para259), cmi_nt35590_para259},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para260), cmi_nt35590_para260},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para261), cmi_nt35590_para261},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para262), cmi_nt35590_para262},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para263), cmi_nt35590_para263},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para264), cmi_nt35590_para264},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para265), cmi_nt35590_para265},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para266), cmi_nt35590_para266},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para267), cmi_nt35590_para267},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para268), cmi_nt35590_para268},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para269), cmi_nt35590_para269},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para270), cmi_nt35590_para270},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para271), cmi_nt35590_para271},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para272), cmi_nt35590_para272},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para273), cmi_nt35590_para273},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para274), cmi_nt35590_para274},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para275), cmi_nt35590_para275},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para276), cmi_nt35590_para276},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para277), cmi_nt35590_para277},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para278), cmi_nt35590_para278},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para279), cmi_nt35590_para279},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para280), cmi_nt35590_para280},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para281), cmi_nt35590_para281},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para282), cmi_nt35590_para282},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para283), cmi_nt35590_para283},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para284), cmi_nt35590_para284},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para285), cmi_nt35590_para285},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para286), cmi_nt35590_para286},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para287), cmi_nt35590_para287},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para288), cmi_nt35590_para288},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para289), cmi_nt35590_para289},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para290), cmi_nt35590_para290},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para291), cmi_nt35590_para291},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para292), cmi_nt35590_para292},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para293), cmi_nt35590_para293},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para294), cmi_nt35590_para294},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para295), cmi_nt35590_para295},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para296), cmi_nt35590_para296},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para297), cmi_nt35590_para297},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para298), cmi_nt35590_para298},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para299), cmi_nt35590_para299},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para300), cmi_nt35590_para300},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para301), cmi_nt35590_para301},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para302), cmi_nt35590_para302},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para303), cmi_nt35590_para303},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para304), cmi_nt35590_para304},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para305), cmi_nt35590_para305},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para306), cmi_nt35590_para306},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para307), cmi_nt35590_para307},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para308), cmi_nt35590_para308},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para309), cmi_nt35590_para309},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para310), cmi_nt35590_para310},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para311), cmi_nt35590_para311},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para312), cmi_nt35590_para312},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para313), cmi_nt35590_para313},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para314), cmi_nt35590_para314},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para315), cmi_nt35590_para315},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para316), cmi_nt35590_para316},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para317), cmi_nt35590_para317},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para318), cmi_nt35590_para318},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para319), cmi_nt35590_para319},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para320), cmi_nt35590_para320},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para321), cmi_nt35590_para321},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para322), cmi_nt35590_para322},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para323), cmi_nt35590_para323},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para324), cmi_nt35590_para324},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para325), cmi_nt35590_para325},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para326), cmi_nt35590_para326},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para327), cmi_nt35590_para327},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para328), cmi_nt35590_para328},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para329), cmi_nt35590_para329},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para330), cmi_nt35590_para330},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para331), cmi_nt35590_para331},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para332), cmi_nt35590_para332},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para333), cmi_nt35590_para333},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para334), cmi_nt35590_para334},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para335), cmi_nt35590_para335},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para336), cmi_nt35590_para336},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para337), cmi_nt35590_para337},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para338), cmi_nt35590_para338},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para339), cmi_nt35590_para339},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para340), cmi_nt35590_para340},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para341), cmi_nt35590_para341},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para342), cmi_nt35590_para342},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para343), cmi_nt35590_para343},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para344), cmi_nt35590_para344},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para345), cmi_nt35590_para345},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para346), cmi_nt35590_para346},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para347), cmi_nt35590_para347},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para348), cmi_nt35590_para348},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para349), cmi_nt35590_para349},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para350), cmi_nt35590_para350},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para351), cmi_nt35590_para351},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para352), cmi_nt35590_para352},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para353), cmi_nt35590_para353},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para354), cmi_nt35590_para354},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para355), cmi_nt35590_para355},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para356), cmi_nt35590_para356},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para357), cmi_nt35590_para357},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para358), cmi_nt35590_para358},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para359), cmi_nt35590_para359},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para360), cmi_nt35590_para360},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para361), cmi_nt35590_para361},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para362), cmi_nt35590_para362},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para363), cmi_nt35590_para363},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para364), cmi_nt35590_para364},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para365), cmi_nt35590_para365},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para366), cmi_nt35590_para366},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para367), cmi_nt35590_para367},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para368), cmi_nt35590_para368},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para369), cmi_nt35590_para369},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para370), cmi_nt35590_para370},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para371), cmi_nt35590_para371},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para372), cmi_nt35590_para372},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para373), cmi_nt35590_para373},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para374), cmi_nt35590_para374},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para375), cmi_nt35590_para375},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para376), cmi_nt35590_para376},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para377), cmi_nt35590_para377},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para378), cmi_nt35590_para378},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para379), cmi_nt35590_para379},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para380), cmi_nt35590_para380},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para381), cmi_nt35590_para381},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para382), cmi_nt35590_para382},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para383), cmi_nt35590_para383},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para384), cmi_nt35590_para384},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para385), cmi_nt35590_para385},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para386), cmi_nt35590_para386},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para387), cmi_nt35590_para387},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para388), cmi_nt35590_para388},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para389), cmi_nt35590_para389},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para390), cmi_nt35590_para390},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para391), cmi_nt35590_para391},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para392), cmi_nt35590_para392},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para393), cmi_nt35590_para393},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para394), cmi_nt35590_para394},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para395), cmi_nt35590_para395},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para396), cmi_nt35590_para396},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para397), cmi_nt35590_para397},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para398), cmi_nt35590_para398},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para399), cmi_nt35590_para399},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para400), cmi_nt35590_para400},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para401), cmi_nt35590_para401},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para402), cmi_nt35590_para402},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para403), cmi_nt35590_para403},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para404), cmi_nt35590_para404},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para405), cmi_nt35590_para405},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para406), cmi_nt35590_para406},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para407), cmi_nt35590_para407},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para408), cmi_nt35590_para408},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para409), cmi_nt35590_para409},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para410), cmi_nt35590_para410},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para411), cmi_nt35590_para411},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para412), cmi_nt35590_para412},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para413), cmi_nt35590_para413},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para414), cmi_nt35590_para414},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para415), cmi_nt35590_para415},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para416), cmi_nt35590_para416},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para417), cmi_nt35590_para417},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para418), cmi_nt35590_para418},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para419), cmi_nt35590_para419},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para420), cmi_nt35590_para420},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para421), cmi_nt35590_para421},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para422), cmi_nt35590_para422},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para423), cmi_nt35590_para423},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para424), cmi_nt35590_para424},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para425), cmi_nt35590_para425},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para426), cmi_nt35590_para426},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para427), cmi_nt35590_para427},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para428), cmi_nt35590_para428},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para429), cmi_nt35590_para429},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para430), cmi_nt35590_para430},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para431), cmi_nt35590_para431},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para432), cmi_nt35590_para432},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para433), cmi_nt35590_para433},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para434), cmi_nt35590_para434},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para435), cmi_nt35590_para435},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para436), cmi_nt35590_para436},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para437), cmi_nt35590_para437},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para438), cmi_nt35590_para438},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para439), cmi_nt35590_para439},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para440), cmi_nt35590_para440},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para441), cmi_nt35590_para441},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para442), cmi_nt35590_para442},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para443), cmi_nt35590_para443},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para444), cmi_nt35590_para444},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para445), cmi_nt35590_para445},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para446), cmi_nt35590_para446},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para447), cmi_nt35590_para447},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para448), cmi_nt35590_para448},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para449), cmi_nt35590_para449},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para450), cmi_nt35590_para450},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para451), cmi_nt35590_para451},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para452), cmi_nt35590_para452},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para453), cmi_nt35590_para453},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para454), cmi_nt35590_para454},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para455), cmi_nt35590_para455},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para456), cmi_nt35590_para456},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para457), cmi_nt35590_para457},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para458), cmi_nt35590_para458},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para459), cmi_nt35590_para459},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para460), cmi_nt35590_para460},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para461), cmi_nt35590_para461},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para462), cmi_nt35590_para462},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para463), cmi_nt35590_para463},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para464), cmi_nt35590_para464},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para465), cmi_nt35590_para465},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para466), cmi_nt35590_para466},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para467), cmi_nt35590_para467},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para468), cmi_nt35590_para468},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para469), cmi_nt35590_para469},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para470), cmi_nt35590_para470},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para471), cmi_nt35590_para471},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para472), cmi_nt35590_para472},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para473), cmi_nt35590_para473},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para474), cmi_nt35590_para474},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para475), cmi_nt35590_para475},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para476), cmi_nt35590_para476},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para477), cmi_nt35590_para477},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para478), cmi_nt35590_para478},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para479), cmi_nt35590_para479},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para480), cmi_nt35590_para480},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para481), cmi_nt35590_para481},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para482), cmi_nt35590_para482},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para483), cmi_nt35590_para483},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(cmi_nt35590_para484), cmi_nt35590_para484},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},
};

#if 1
static char boe_hx8394_para1[]={0xB9,0xFF,0x83,0x94};
static char boe_hx8394_para2[]={0xBA,0x13};
static char boe_hx8394_para3[]={0xB1,0x7C,0x00,0x34,0x09,0x01,0x11,0x11,0x36,0x3E,0x26,0x26,0x57,0x12,0x01,0xE6};
static char boe_hx8394_para4[]={0xB4,0x00,0x00,0x00,0x05,0x06,0x41,0x42,0x02,0x41,0x42,0x43,0x47,0x19,0x58,0x60,0x08,0x85,0x10};
static char boe_hx8394_para5[]={0xD5,0x4C,0x01,0x00,0x01,0xCD,0x23,0xEF,0x45,0x67,0x89,0xAB,0x11,0x00,0xDC,0x10,0xFE,0x32,0xBA,0x98,0x76,0x54,0x00,0x11,0x40};
static char boe_hx8394_para6[]={0xE0,0x24,0x33,0x36,0x3F,0x3F,0x3F,0x3C,0x56,0x05,0x0C,0x0E,0x11,0x13,0x12,0x14,0x12,0x1E,0x24,0x33,0x36,0x3F,0x3F,0x3F,0x3C,0x56,0x05,
																0x0C,0x0E,0x11,0x13,0x12,0x14,0x12,0x1E};
static char boe_hx8394_para7[]={0xCC,0x01};
static char boe_hx8394_para8[]={0xB6,0x2A};
static char boe_hx8394_para9[]={0x36,0x02};

static struct dsi_cmd_desc boe_hx8394_video_on_cmds[] = {

	{DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(boe_hx8394_para1), boe_hx8394_para1}, 
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_hx8394_para2), boe_hx8394_para2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(boe_hx8394_para3), boe_hx8394_para3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(boe_hx8394_para4), boe_hx8394_para4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(boe_hx8394_para5), boe_hx8394_para5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(boe_hx8394_para6), boe_hx8394_para6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50, sizeof(boe_hx8394_para7), boe_hx8394_para7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_hx8394_para8), boe_hx8394_para8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(boe_hx8394_para9), boe_hx8394_para9},
	
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_on), display_on}, 
	 
};
#endif
#if 0//back
/**************************************
5. hx8369 tianma IPS start 
**************************************/
static char hx8369_setpassword_para[4]={0xB9,0xFF,0x83,0x69};
static char hx8369_tianma_ips_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x03,0x00,0x11,0x11,0x2f,0x37,0x3F, 
        0x3F,0x07,0x3a,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_ips_para_0xb2[16]={0xB2,0x00,0x2b,0x03,0x03,0x70,0x00,0xFF, 
        0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE 
static char hx8369_tianma_ips_para_0xb4[6]={0xB4,0x00,0x18,0x70,0x13,0x05}; 
static char hx8369_tianma_ips_para_0xb6[3]={0xB6,0x32,0x32}; //flick
static char hx8369_tianma_ips_para_0xd5[27]={0xD5,0x00,0x0e,0x03,0x2b,0x01,0x11,0x28,0x60, 
        0x11,0x13,0x00,0x00,0x60,0xc4,0x71,0xc5,0x00,0x00,0x71,0x35,0x60,0x24,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_ips_para_0xe0[35]={0xE0,0x00,0x0d,0x19,0x2f,0x3b,0x3d,0x2e,0x4a,0x08,0x0e,0x0F, 
        0x14,0x16,0x14,0x14,0x14,0x1e,0x00,0x0d,0x19,0x2f,0x3b,0x3d,0x2e,0x4a,0x08,0x0e,0x0F, 
        0x14,0x16,0x14,0x14,0x14,0x1e};  
static char hx8369_tianma_ips_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_ips_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE 

static struct dsi_cmd_desc novatek_video_on_cmds[] = 
{

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb1), hx8369_tianma_ips_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb2), hx8369_tianma_ips_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb4), hx8369_tianma_ips_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb6), hx8369_tianma_ips_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xd5), hx8369_tianma_ips_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xe0), hx8369_tianma_ips_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0x3a), hx8369_tianma_ips_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xba), hx8369_tianma_ips_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
#endif

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


#define HX8394_IC_ID	0X94
#define IC_ID			5

#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

extern u32 LcdPanleID ;

struct mipi_manufacture_ic {
	struct dsi_cmd_desc *readid_tx;
	int readid_len_tx;
	struct dsi_cmd_desc *readid_rx;
	int readid_len_rx;
	int mode;
};

static char hx8394_setpassword_para[4]={0xB9,0xFF,0x83,0x94};
static struct dsi_cmd_desc hx8394_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8394_setpassword_para),hx8394_setpassword_para},

};

static char hx8394_rd_ic_id_para[2] = {0xf4, 0x00}; /* DTYPE_DCS_READ */
static struct dsi_cmd_desc hx8394_rd_ic_id_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 1, sizeof(hx8394_rd_ic_id_para), hx8394_rd_ic_id_para};
	

static uint32 mipi_get_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	//struct dsi_cmd_desc *cmd;
	uint32 *lp,i,id;
	
	 struct mipi_manufacture_ic mipi_manufacture_icid[1] = 
	 {	 	
		{hx8394_setpassword_cmd,ARRAY_SIZE(hx8394_setpassword_cmd),&hx8394_rd_ic_id_cmd,2,1},
	 };
	 
	 for(i = 0; i < ARRAY_SIZE(mipi_manufacture_icid) ; i++)	
	 {
		tp = &novatek_tx_buf;
		rp = &novatek_rx_buf;
		
		lcd_panle_reset();
		mipi_set_tx_power_mode(1);		
		//cmd = &hx8394_rd_ic_id_cmd;
		if(mipi_manufacture_icid[i].readid_tx)
			mipi_dsi_cmds_tx(&novatek_tx_buf, mipi_manufacture_icid[i].readid_tx,mipi_manufacture_icid[i].readid_len_tx);
		
		if(!mipi_manufacture_icid[i].mode)
			mipi_set_tx_power_mode(0);	
		
		mipi_dsi_cmd_bta_sw_trigger(); 
		
		mipi_dsi_cmds_rx(mfd, tp, rp,  mipi_manufacture_icid[i].readid_rx, mipi_manufacture_icid[i].readid_len_rx);

		//if(mipi_manufacture_icid[i].mode)
		//	mipi_set_tx_power_mode(0);
				
		lp = (uint32 *)rp->data;
		//pr_info("%s: manufacture_id=%x\n", __func__, *lp);
		id = *lp;
		printk("\n lcd panel read id value =%x",id);

		if ((id & 0xff )== HX8394_IC_ID)		
			return LCD_PANEL_5P0_HX8394_BOE_BOE;
		else
			return LCD_PANEL_5P0_NT35590_CMI_CMI;
	 }
	 
	 return 0;
	 
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
	msleep(20);

}



static int pm8921_get_io_val(int gpio_num)
{
	int rc,gpio5,val;
	gpio5 = PM8921_GPIO_PM_TO_SYS(gpio_num); 
	rc = gpio_request(gpio5, "disp_pwr_en_n");
	if (rc) 
	{
		printk("\n lcd request gpio 5 failed, rc=%d\n", rc);
		return -ENODEV;
	}
	val = gpio_get_value_cansleep(gpio5);
	printk("\n lcd get id val(gpio 5) =%d ",val);
	return val;
}
static void get_panel_id(void)
{
	int panel_id;
	return ;//disable this functon 
	
	panel_id = pm8921_get_io_val(IC_ID);
	if (panel_id ==0)
		LcdPanleID = LCD_PANEL_5P0_HX8394_BOE_BOE;
	else
		LcdPanleID = LCD_PANEL_5P0_NT35590_CMI_CMI;
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
		
	//LcdPanleID = LCD_PANEL_5P0_HX8394_BOE_BOE;//pan temp
	
	if (first_time_panel_on)
	{		
		first_time_panel_on = 0;
		return 0;//do not read id,because of power consumption
		if( LcdPanleID == LCD_PANEL_NOPANEL)
			LcdPanleID = mipi_get_manufacture_id(mfd);
		else		
			return 0;   
	}
	
	lcd_panle_reset();

	
	if (mipi->mode == DSI_VIDEO_MODE) 
	{
		switch(LcdPanleID) 
		{
			
			case LCD_PANEL_5P0_HX8394_BOE_BOE:
				cmdreq.cmds = boe_hx8394_video_on_cmds;
				cmdreq.cmds_cnt = ARRAY_SIZE(boe_hx8394_video_on_cmds);
				cmdreq.flags = CMD_REQ_COMMIT;
				cmdreq.rlen = 0;
				cmdreq.cb = NULL;
				mipi_dsi_cmdlist_put(&cmdreq);
				return 0;
				
				mipi_dsi_cmds_tx(&novatek_tx_buf, boe_hx8394_video_on_cmds,
				ARRAY_SIZE(boe_hx8394_video_on_cmds));
				printk("\n lcd boe(hx8394)  initialize ");
				break;
			case LCD_PANEL_5P0_NT35590_CMI_CMI:				
				cmdreq.cmds = cmi_nt35590_display_on_add_cmds;
				cmdreq.cmds_cnt = ARRAY_SIZE(cmi_nt35590_display_on_add_cmds);
				cmdreq.flags = CMD_REQ_COMMIT;
				cmdreq.rlen = 0;
				cmdreq.cb = NULL;
				mipi_dsi_cmdlist_put(&cmdreq);
				lcd_panle_reset();			
				cmdreq.cmds = cmi_nt35590_display_on_cmds;
				cmdreq.cmds_cnt = ARRAY_SIZE(cmi_nt35590_display_on_cmds);
				mipi_dsi_cmdlist_put(&cmdreq);
				return 0;
				
				mipi_dsi_cmds_tx(&novatek_tx_buf, cmi_nt35590_display_on_cmds,
				ARRAY_SIZE(cmi_nt35590_display_on_cmds));
				printk("\n lcd cmi(nt35590) initialize ");
				break;
			default:
				printk("\n lcd Error, No panel initialize  ");
				break;

		}
		
	} 
	else 
	{
		mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_cmd_on_cmds,
				ARRAY_SIZE(novatek_cmd_on_cmds));

		/* clean up ack_err_status */
		mipi_dsi_cmd_bta_sw_trigger();
		mipi_get_manufacture_id(mfd);
	}

	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
{
#if 0
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_display_off_cmds,
			ARRAY_SIZE(novatek_display_off_cmds));
	printk("\n LCD panel off");
	return 0;
#endif
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
	printk("LCD OFF");
	
	cmdreq.cmds = novatek_display_off_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(novatek_display_off_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	return 0;

	
	
}

DEFINE_LED_TRIGGER(bkl_led_trigger);
#if 0
static char led_pwm1[2] = {0x51, 0x0};	/* DTYPE_DCS_WRITE1 */
static struct dsi_cmd_desc backlight_cmd = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1};
#endif
struct dcs_cmd_req cmdreq;


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
#ifdef CONFIG_MACH_QUANTUM		//9810 
    	if(current_lel > 32)
    	{
        	current_lel = 32;
    	}
#else
    	if(current_lel > 28)
    	{
        	current_lel = 28;
    	}
#endif
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

	if ((mipi_novatek_pdata->enable_wled_bl_ctrl)
	    && (wled_trigger_initialized)) {
		led_trigger_event(bkl_led_trigger, mfd->bl_level);
		return;
	}

	led_pwm1[1] = (unsigned char)mfd->bl_level;

	cmdreq.cmds = &backlight_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = 0;
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

	if (pdev->id == 0) 
	{
		
		mipi_novatek_pdata = pdev->dev.platform_data;
		
		get_panel_id();//get panel id
		//mipi_get_manufacture_id(mfd);
		
		if (mipi_novatek_pdata&& mipi_novatek_pdata->phy_ctrl_settings) 
		{
			phy_settings = (mipi_novatek_pdata->phy_ctrl_settings);
		}

		if (mipi_novatek_pdata&& mipi_novatek_pdata->dlane_swap) 
		{
			dlane_swap = (mipi_novatek_pdata->dlane_swap);
		}

		if (mipi_novatek_pdata&& mipi_novatek_pdata->fpga_3d_config_addr)
			mipi_novatek_3d_init(mipi_novatek_pdata->fpga_3d_config_addr, mipi_novatek_pdata->fpga_ctrl_mode);

		/* create sysfs to control 3D barrier for the Sharp panel */
		if (mipi_dsi_3d_barrier_sysfs_register(&pdev->dev)) 
		{
			pr_err("%s: Failed to register 3d Barrier sysfs\n",__func__);
			return -ENODEV;
		}
		barrier_mode = 0;

		return 0;
	}

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev) 
	{
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
