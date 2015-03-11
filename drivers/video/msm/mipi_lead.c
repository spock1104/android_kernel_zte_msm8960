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

extern u32 LcdPanleID ;

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

/*******************  1 . truly lg lcd nt35516 ***********************************/

//lg lcd
static char para1[]={0xFF,0xAA,0x55,0x25,0x01,0x01};
static char para2[]={0xF2,0x00,0x00,0x4A,0x0A,0xA8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x01};
static char para3[]={0xF3,0x02,0x03,0x07,0x45,0x88,0xD1,0x0D};
//Page 0 
static char para4[]={0xF0,0x55,0xAA,0x52,0x08,0x00};
static char para_0x36[]={0x36,0xd4};//pan add for vertical flip

static char para5[]={0xB1,0xFC,0x00,0x00};//only  set 0x36 = 0xd4 ;or  set 0x36 = 0x00 ,set  0xb101=0x06, 
static char para6[]={0xB8,0x01,0x02,0x02,0x02};//02
static char para7[]={0xC9,0x63,0x06,0x0D,0x1A,0x17,0x00};
//Page 1
static char para8[]={0xF0,0x55,0xAA,0x52,0x08,0x01};
static char para9[]={0xB0,0x05,0x05,0x05};
static char para10[]={0xB1,0x05,0x05,0x05};
static char para11[]={0xB2,0x01,0x01,0x01};
static char para12[]={0xB3,0x0E,0x0E,0x0E};
static char para13[]={0xB4,0x0a,0x0a,0x0a};
static char para14[]={0xB6,0x44,0x44,0x44};
static char para15[]={0xB7,0x34,0x34,0x34};
static char para16[]={0xB8,0x10,0x10,0x10};
static char para17[]={0xB9,0x26,0x26,0x26};
static char para18[]={0xBA,0x24,0x24,0x24};
static char para19[]={0xBC,0x00,0xC8,0x00};
static char para20[]={0xBD,0x00,0xC8,0x00};
static char para21[]={0xBE,0x7b}; //92
static char para22[]={0xC0,0x04,0x00};
static char para23[]={0xCA,0x00};

static char para24[]={  0xD0, 0x0F, 0x0F, 0x10,0x10  };                                                                                    
                                                                                                                      
static char para25[]={0xD1,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para26[]={0xD2,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para27[]={0xD3,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para28[]={0xD4,0x03,0xFF,0x03,0xFF};

static char para29[]={0xD5,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para30[]={0xD6,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para31[]={0xD7,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para32[]={0xD8,0x03,0xFF,0x03,0xFF};

static char para33[]={0xD9,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para34[]={0xDD,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para35[]={0xDE,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para36[]={0xDF,0x03,0xFF,0x03,0xFF};

static char para37[]={0xE0,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para38[]={0xE1,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para39[]={0xE2,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para40[]={0xE3,0x03,0xFF,0x03,0xFF};

static char para41[]={0xE4,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para42[]={0xE5,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para43[]={0xE6,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para44[]={0xE7,0x03,0xFF,0x03,0xFF};

static char para45[]={0xE8,0x00,0x00,0x00,0x72,0x00,0xAA,0x00,0xC6,0x00,0xDA,0x00,0xFD,0x01,0x1A,0x01,0x46};
static char para46[]={0xE9,0x01,0x68,0x01,0x9E,0x01,0xC9,0x02,0x0E,0x02,0x48,0x02,0x4A,0x02,0x82,0x02,0xC2};
static char para47[]={0xEA,0x02,0xEB,0x03,0x1F,0x03,0x40,0x03,0x69,0x03,0x81,0x03,0xA2,0x03,0xB8,0x03,0xE1};
static char para48[]={0xEB,0x03,0xFF,0x03,0xFF};

static char para49[]={0x2C,0x00};
static char para50[]={0x13,0x00};


static struct dsi_cmd_desc truly_display_on_cmds[] = {

		  //for lg
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para1), para1},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para2), para2},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para3), para3},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para4), para4},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para5), para5},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para6), para6},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para7), para7},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para8), para8},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para_0x36), para_0x36},//pan add for vertical flip		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para9), para9},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para10), para10},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para11), para11},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para12), para12},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para13), para13},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para14), para14},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para15), para15},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para16), para16},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para17), para17},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para18), para18},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para19), para19},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para20), para20},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(para21), para21},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para22), para22},
		{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(para23), para23},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para24), para24},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para25), para25},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para26), para26},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para27), para27},	
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para28), para28},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para29), para29},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para30), para30},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para31), para31},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para32), para32},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para33), para33},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para34), para34},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para35), para35},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para36), para36},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para37), para37},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para38), para38},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para39), para39},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para40), para40},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para41), para41},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para42), para42},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para43), para43},		
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para44), para44},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para45), para45},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para46), para46},
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para47), para47},			
		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(para48), para48},
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(para49), para49},			
		{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(para50), para50},

		{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
		{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_on), display_on}	

};

/******************* 2. lead auo lcd  nt35516************************************/
static char auo_para0_1[5]={0xFF,0xAA,0x55,0x25,0x01};
static char auo_para0_2[5]={0xF3,0x02,0x03,0x07,0x45};
// Select CMD2, Page 0 
static char auo_para1[6]={0xF0,0x55,0xAA,0x52,0x08,0x00};
// Video mode Enable
static char auo_para1_1[2]={0xB1,0xFC};
// Source EQ
static char auo_para2[5]={0xB8,0x01,0x02,0x02,0x02};
// Z Inversion
static char auo_para3[4]={0xBC,0x05,0x05,0x05};
// Vivid Color
static char auo_para3_1[2]={0xD8,0x40};
static char auo_para3_2[17]={0xD6,0x00,0x03,0x05,0x08,0x0A,0x0D,0x0F,0x12,0x14,0x17,0x1A,0x1C,0x1F,0x21,0x24,0x26};static char auo_para3_3[9]={0xD7,0x29,0x2C,0x2E,0x31,0x33,0x36,0x38,0x3D};

static char auo_para_0x36[2]={0x36,0xd4};//pan add for vertical flip


// Select CMD2, Page 1
static char auo_para4[6]={0xF0,0x55,0xAA,0x52,0x08,0x01};
// AVDD: 6.0V
static char auo_para5[4]={0xB0,0x05,0x05,0x05};
// AVEE: -6.0V
static char auo_para6[4]={0xB1,0x05,0x05,0x05};
// VGH: 15V
static char auo_para6_1[4]={0xB3,0x10,0x10,0x10};
// VGL: -12V
static char auo_para6_2[4]={0xB4,0x0A,0x0A,0x0A};
// AVDD: 2.5x VPNL
static char auo_para7[4]={0xB6,0x44,0x44,0x44};
// AVEE: -2.5x VPNL
static char auo_para8[4]={0xB7,0x34,0x34,0x34};
// VGH: 2AVDD-AVEE
static char auo_para81[4]={0xB9,0x34,0x34,0x34};
// VGLX: AVEE - AVDD + VCL
static char auo_para9[4]={0xBA,0x34,0x34,0x34};
// VGMP: 5.0V, VGSP=0V
static char auo_para10[4]={0xBC,0x00,0xA0,0x00};
// VGMN: 5.0V, VGSN=-0V
static char auo_para11[4]={0xBD,0x00,0xA0,0x00};
// VCOM
static char auo_para12[2]={0xBE,0x4F};
//Vivid color Enable
static char auo_para12_1[2]={0x4C,0x11};

// Gamma Code

// Positive Red Gamma
static char auo_para13[17]={0xD1,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para14[17]={0xD2,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para15[17]={0xD3,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para16[5]={0xD4,0x03,0xFF,0x03,0xFF};

// Positive Green Gamma
static char auo_para17[17]={0xD5,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para18[17]={0xD6,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para19[17]={0xD7,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para20[5]={0xD8,0x03,0xFF,0x03,0xFF};

// Positive Blue Gamma
static char auo_para21[17]={0xD9,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para22[17]={0xDD,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para23[17]={0xDE,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para24[5]={0xDF,0x03,0xFF,0x03,0xFF};

// Negative Red Gamma
static char auo_para25[17]={0xE0,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para26[17]={0xE1,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para27[17]={0xE2,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para28[5]={0xE3,0x03,0xFF,0x03,0xFF};

// Negative Green Gamma
static char auo_para29[17]={0xE4,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para30[17]={0xE5,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para31[17]={0xE6,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para32[5]={0xE7,0x03,0xFF,0x03,0xFF};

// Negative Blue Gamma
static char auo_para33[17]={0xE8,0x00,0x70,0x00,0x7B,0x00,0x8E,0x00,0x9F,0x00,0xAD,0x00,0xC8,0x00,0xE0,0x01,0x04};
static char auo_para34[17]={0xE9,0x01,0x22,0x01,0x51,0x01,0x76,0x01,0xAF,0x01,0xDC,0x01,0xDD,0x02,0x05,0x02,0x2E};
static char auo_para35[17]={0xEA,0x02,0x43,0x02,0x64,0x02,0x7A,0x02,0xA6,0x02,0xC9,0x03,0x1B,0x03,0x51,0x03,0xFF};
static char auo_para36[5]={0xEB,0x03,0xFF,0x03,0xFF};
// TE On
static char auo_para37[2]={0x35,0x00};


static struct dsi_cmd_desc auo_display_on_cmds[] = {

	   //for auo
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para0_1), auo_para0_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para0_2), auo_para0_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para1), auo_para1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para1_1), auo_para1_1},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para2), auo_para2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para3), auo_para3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para3_1), auo_para3_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para3_2), auo_para3_2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para3_3), auo_para3_3},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para4), auo_para4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para_0x36), auo_para_0x36},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para5), auo_para5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para6), auo_para6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para6_1), auo_para6_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para6_2), auo_para6_2},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para7), auo_para7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para8), auo_para8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para81), auo_para81},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para9), auo_para9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para10), auo_para10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para11), auo_para11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_para12), auo_para12},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_para12_1), auo_para12_1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para13), auo_para13},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para14), auo_para14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para15), auo_para15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para16), auo_para16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para17), auo_para17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para18), auo_para18},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para19), auo_para19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para20), auo_para20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para21), auo_para21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para22), auo_para22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para23), auo_para23},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para24), auo_para24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para25), auo_para25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para26), auo_para26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para27), auo_para27},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para28), auo_para28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para29), auo_para29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para30), auo_para30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para31), auo_para31},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para32), auo_para32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para33), auo_para33},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para34), auo_para34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para35), auo_para35},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(auo_para36), auo_para36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(auo_para37), auo_para37},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_on), display_on}

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

static uint32 mipi_novatek_manufacture_id(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	struct dsi_cmd_desc *cmd;
	uint32 *lp;

	tp = &novatek_tx_buf;
	rp = &novatek_rx_buf;
	cmd = &novatek_manufacture_id_cmd;
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 3);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=%x\n", __func__, *lp);
	return *lp;
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
		printk("\n LCD first time power on  -- return!");
		first_time_panel_on = 0;		
		return 0;		
	}
	
	printk("\n lcd panel on  start");
	
	lcd_panle_reset();
	
	if (mipi->mode == DSI_VIDEO_MODE) {
		switch(LcdPanleID)
		{
			case (u32)LCD_PANEL_4P3_NT35516_AUO_LEAD:
				mipi_dsi_cmds_tx(&novatek_tx_buf, auo_display_on_cmds,
					ARRAY_SIZE(auo_display_on_cmds));
				printk("\n LCD LEAD AUO NT35516 panel initialized!");
				break;
			case (u32)LCD_PANEL_4P3_NT35516_LG_TRULY:
				mipi_dsi_cmds_tx(&novatek_tx_buf, truly_display_on_cmds,
					ARRAY_SIZE(truly_display_on_cmds));
				printk("\n TRULY LG NT35516 panel initialized!");
				break;
			default:
				mipi_dsi_cmds_tx(&novatek_tx_buf, auo_display_on_cmds,
					ARRAY_SIZE(auo_display_on_cmds));
				printk("\n LCD error! no panel id,set default panel lead auo nt35516 \n");	
				break;
		}
		
	} else {
		mipi_dsi_cmds_tx(&novatek_tx_buf, novatek_cmd_on_cmds,
				ARRAY_SIZE(novatek_cmd_on_cmds));

		/* clean up ack_err_status */
		mipi_dsi_cmd_bta_sw_trigger();
		mipi_novatek_manufacture_id(mfd);
	}
	return 0;
}

static int mipi_novatek_lcd_off(struct platform_device *pdev)
{
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

    	if(current_lel > 32)
    	{
        	current_lel = 32;
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
