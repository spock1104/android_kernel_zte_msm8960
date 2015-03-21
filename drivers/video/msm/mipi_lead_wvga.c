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


/*ic define*/
#define HIMAX_8363 		1
#define HIMAX_8369 		2
#define NOVATEK_35510	3
#define RENESAS_R61408	4
#define CMI_8001		5
#define HIMAX_8369B 	6


#define HIMAX8369_TIANMA_TN_ID		0xB1
#define HIMAX8369_TIANMA_IPS_ID		0xA5
#define HIMAX8369_LEAD_ID				0
#define HIMAX8369_LEAD_HANNSTAR_ID	0x88
#define NT35510_YUSHUN_ID				0
#define NT35510_LEAD_ID				0xA0
#define NT35510_BOE_ID					0xB0
#define HIMAX8369_YUSHUN_IVO_ID	       0x85


/*about himax8363 chip id */
static char hx8363_setpassword_para[4]={0xB9,0xFF,0x83,0x63};
static char hx8363_icid_rd_para[2] = {0xB9, 0x00};   
static char hx8363_panleid_rd_para[2] = {0xda, 0x00};

static struct dsi_cmd_desc hx8363_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8363_icid_rd_para), hx8363_icid_rd_para
};
static struct dsi_cmd_desc hx8363_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8363_setpassword_para),hx8363_setpassword_para},

};
static struct dsi_cmd_desc hx8363_panleid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8363_panleid_rd_para), hx8363_panleid_rd_para
};

/*about himax8369 chip id */
static char hx8369_setpassword_para[4]={0xB9,0xFF,0x83,0x69};
static char hx8369_icid_rd_para[2] = {0xB9, 0x00}; 
static char hx8369_panleid_rd_para[2] = {0xda, 0x00};    


static struct dsi_cmd_desc hx8369_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369_icid_rd_para), hx8369_icid_rd_para
};
static struct dsi_cmd_desc hx8369_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},

};
static struct dsi_cmd_desc hx8369_panleid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369_panleid_rd_para), hx8369_panleid_rd_para
};


/*about Novatek3511 chip id */
static char nt3511_page_ff[5] = {0xff, 0xaa,0x55,0x25,0x01};
static char nt3511_page_f8[20] = {0xf8, 0x01,0x12,0x00,0x20,0x33,0x13,0x00,0x40,0x00,0x00,0x23,0x02,0x99,0xc8,0x00,0x00,0x01,0x00,0x00};
static char nt3511_icid_rd_para[2] = {0xc5, 0x00}; 
static char nt3511_panleid_rd_para[2] = {0xDA, 0x00};    //added by zte_gequn091966 for lead_nt35510,20111226

static struct dsi_cmd_desc nt3511_setpassword_cmd[] = {	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nt3511_page_ff),nt3511_page_ff},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nt3511_page_f8),nt3511_page_f8}
};
static struct dsi_cmd_desc nt3511_icid_rd_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(nt3511_icid_rd_para), nt3511_icid_rd_para};


static struct dsi_cmd_desc nt3511_panleid_rd_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(nt3511_panleid_rd_para), nt3511_panleid_rd_para
};   //added by zte_gequn091966 for lead_nt35510,20111226


/*about RENESAS r61408 chip id */
static char r61408_setpassword_para[2]={0xb0,0x04};
static struct dsi_cmd_desc r61408_setpassword_cmd[] = 
{	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(r61408_setpassword_para),r61408_setpassword_para},

};
static char r61408_icid_rd_para[2] = {0xbf, 0x00}; 
static struct dsi_cmd_desc r61408_icid_rd_cmd = 
{
	DTYPE_GEN_READ1, 1, 0, 0, 1, sizeof(r61408_icid_rd_para), r61408_icid_rd_para
};


/*about himax8369b chip id */
static char hx8369b_setpassword_para[4]={};
static char hx8369b_icid_rd_para[2] = {0x04, 0x00};   
static char hx8369b_panleid_rd_para[2] = {0xdb, 0x00};  

static struct dsi_cmd_desc hx8369b_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8369b_setpassword_para),hx8369b_setpassword_para},
};
static struct dsi_cmd_desc hx8369b_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369b_icid_rd_para), hx8369b_icid_rd_para
};
static struct dsi_cmd_desc hx8369b_panleid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369b_panleid_rd_para), hx8369b_panleid_rd_para
};

static char cmi_icid_rd_para[] = {0xa1,0x00}; //8009
static struct dsi_cmd_desc cmi_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(cmi_icid_rd_para), cmi_icid_rd_para
};

/**************************************
1. hx8363 yassy start 
**************************************/
static char hx8363_yassy_para_0xb1[13]={0xB1,0x78,0x34,0x08,0x34,0x02,0x13,
								0x11,0x11,0x2d,0x35,0x3F,0x3F};  
static char hx8363_yassy_para_0xba[14]={0xBA,0x80,0x00,0x10,0x08,0x08,0x10,0x7c,0x6e,
								0x6d,0x0a,0x01,0x84,0x43};   //TWO LANE
static char hx8363_yassy_para_0x3a[2]={0x3a,0x77};
//static char hx8363_para_0x36[2]={0x36,0x0a};
static char hx8363_yassy_para_0xb2[4]={0xb2,0x33,0x33,0x22};
static char hx8363_yassy_para_0xb3[2]={0xb3,0x00};
static char hx8363_yassy_para_0xb4[10]={0xb4,0x08,0x12,0x72,0x12,0x06,0x03,0x54,0x03,0x4e};
static char hx8363_yassy_para_0xb6[2]={0xb6,0x2c};
static char hx8363_yassy_para_0xcc[2]={0xcc,0x09};
static char hx8363_yassy_para_0xe0[31]={0xe0,0x01,0x09,0x17,0x10,0x10,0x3e,0x07,
	0x8d,0x90,0x54,0x16,0xd5,0x55,0x53,0x19,0x01,0x09,0x17,0x10,0x10,0x3e,0x07,
	0x8d,0x90,0x54,0x16,0xd5,0x55,0x53,0x19};	

static struct dsi_cmd_desc hx8363_yassy_display_on_cmds[] = 
{

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_setpassword_para),hx8363_setpassword_para},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb1), hx8363_yassy_para_0xb1},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb2), hx8363_yassy_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xba), hx8363_yassy_para_0xba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0x3a), hx8363_yassy_para_0x3a},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb3), hx8363_yassy_para_0xb3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb4), hx8363_yassy_para_0xb4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb6), hx8363_yassy_para_0xb6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xcc), hx8363_yassy_para_0xcc},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(hx8363_para_0x36), hx8363_para_0x36},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xe0), hx8363_yassy_para_0xe0},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8363_para_0xc1), hx8363_para_0xc1},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(hx8363_para_0xc2), hx8363_para_0xc2},	
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};


/**************************************
9. hx8363 YUSHUN IVO start 
**************************************/
static char hx8363_yushun_para_0xb1[13]={0xB1,0x78,0x34,0x07,0x33,0x02,0x13,0x0F,0x00,0x1C,0x24,0x3F,0x3F};  
static char hx8363_yushun_para_0xba[14]={0xBA,0x80,0x00,0x10,0x08,0x08,0x10,0x7C,0x6E,0x6D,0x0A,0x01,0x84,0x43};   //TWO LANE
static char hx8363_yushun_para_0x3a[2]={0x3A,0x70};
static char hx8363_yushun_para_0xb2[4]={0xB2,0x33,0x33,0x22};
static char hx8363_yushun_para_0xb4[10]={0xB4,0x04,0x12,0x72,0x12,0x06,0x03,0x54,0x03,0x4E};
static char hx8363_yushun_para_0xb6[2]={0xB6,0x1C};
static char hx8363_yushun_para_0xcc[2]={0xCC,0x09};
static char hx8363_yushun_para_0xe0[31]={0xE0,0x00,0x1f,0x61,0x0A,0x0A,0x3C,0x0d,0x91,0x50,0x15,0x17,0xD5,0x16,0x13,0x1A,0x00,0x1f,0x61,0x0A,0x0A,0x3C,0x0d,0x91,0x50,0x15,0x17,0xD5,0x16,0x13,0x1A};	
static struct dsi_cmd_desc hx8363_yushun_display_on_cmds[] = 
{
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_setpassword_para),hx8363_setpassword_para},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xb1), hx8363_yushun_para_0xb1},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xb2), hx8363_yushun_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xba), hx8363_yushun_para_0xba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0x3a), hx8363_yushun_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xb4), hx8363_yushun_para_0xb4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xb6), hx8363_yushun_para_0xb6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xcc), hx8363_yushun_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yushun_para_0xe0), hx8363_yushun_para_0xe0},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
};

/**************************************
2. hx8369 lead start 
**************************************/
static char hx8369_lead_tn_para_0xb0[3]={0xb0,0x01,0x09};
static char hx8369_lead_tn_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0F,0x0F,
	0x21,0x28,0x3F,0x3F,0x07,0x23,0x01,0xE6,0xE6,0xE6,0xE6,0xE6};  
static char hx8369_lead_tn_para_0xb2[16]={0xB2,0x00,0x23,0x0A,0x0A,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
//static char para_0xb2[16]={0xB2,0x00,0x20,0x0A,0x0A,0x70,0x00,0xFF,
//	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //CMD MODE
static char hx8369_lead_tn_para_0xb4[6]={0xB4,0x00,0x0C,0x84,0x0C,0x01}; 
static char hx8369_lead_tn_para_0xb6[3]={0xB6,0x2c,0x2c};
static char hx8369_lead_tn_para_0xd5[27]={0xD5,0x00,0x05,0x03,0x00,0x01,0x09,0x10,
	0x80,0x37,0x37,0x20,0x31,0x46,0x8A,0x57,0x9B,0x20,0x31,0x46,0x8A,
	0x57,0x9B,0x07,0x0F,0x07,0x00}; 
static char hx8369_lead_tn_para_0xe0[35]={0xE0,0x00,0x06,0x06,0x29,0x2d,0x3F,0x13,0x32,
	0x08,0x0c,0x0D,0x11,0x14,0x11,0x14,0x0e,0x15,0x00,0x06,0x06,0x29,0x2d,
	0x3F,0x13,0x32,0x08,0x0c,0x0D,0x11,0x14,0x11,0x14,0x0e,0x15};
static char hx8369_lead_tn_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_lead_tn_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,
	0x6C,0x02,0x11,0x18,0x40};   //TWO LANE
//static char para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,
	//0x6C,0x02,0x10,0x18,0x40};   //ONE LANE

static struct dsi_cmd_desc hx8369_lead_display_on_cmds[] = {
	 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb0), hx8369_lead_tn_para_0xb0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb1), hx8369_lead_tn_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb2), hx8369_lead_tn_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb4), hx8369_lead_tn_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb6), hx8369_lead_tn_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xd5), hx8369_lead_tn_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xe0), hx8369_lead_tn_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0x3a), hx8369_lead_tn_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xba), hx8369_lead_tn_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
/**************************************
3. HANNSTAR hx8369 lead start 
**************************************/
static char hx8369_lead_hannstar_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0E,0x0E,
	0x21,0x29,0x3F,0x3F,0x01,0x63,0x01,0xE6,0xE6,0xE6,0xE6,0xE6};  
static char hx8369_lead_hannstar_para_0xb2[16]={0xB2,0x00,0x23,0x07,0x07,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_lead_hannstar_para_0xb4[6]={0xB4,0x02,0x18,0x80,0x13,0x05}; 
static char hx8369_lead_hannstar_para_0xb6[3]={0xB6,0x1F,0x1F};
static char hx8369_lead_hannstar_para_0xcc[2]={0xcc,0x00}; 
static char hx8369_lead_hannstar_para_0xd5[27]={0xD5,0x00,0x0c,0x00,0x00,0x01,0x0f,0x10,
	0x60,0x33,0x37,0x23,0x01,0xB9,0x75,0xA8,0x64,0x00,0x00,0x41,0x06,
	0x50,0x07,0x07,0x0F,0x07,0x00};
static char hx8369_lead_hannstar_para_0xe0[35]={0xE0,0x00,0x03,0x00,0x09,0x09,0x21,0x1B,0x2D,
	0x06,0x0c,0x10,0x15,0x16,0x14,0x16,0x12,0x18,0x00,0x03,0x00,0x09,0x09,
	0x21,0x1B,0x2D,0x06,0x0c,0x10,0x15,0x16,0x14,0x16,0x12,0x18};
static char hx8369_lead_hannstar_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_lead_hannstar_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x02,0x10,0x30,
	0x6F,0x02,0x11,0x18,0x40};   //TWO LANE

static struct dsi_cmd_desc hx8369_lead_hannstar_display_on_cmds[] = {
	 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb1), hx8369_lead_hannstar_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb2), hx8369_lead_hannstar_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb4), hx8369_lead_hannstar_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb6), hx8369_lead_hannstar_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xcc), hx8369_lead_hannstar_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xd5), hx8369_lead_hannstar_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xe0), hx8369_lead_hannstar_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0x3a), hx8369_lead_hannstar_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xba), hx8369_lead_hannstar_para_0xba},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

/**************************************
4. hx8369 tianma TN start 
**************************************/
static char hx8369_tianma_tn_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x0A,0x00,0x11,0x12,0x21,0x29,0x3F,0x3F,
	0x01,0x1a,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_tn_para_0xb2[16]={0xB2,0x00,0x23,0x03,0x03,0x70,0x00,0xFF,0x00,0x00,0x00,0x00,
	0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_tianma_tn_para_0xb4[6]={0xB4,0x02,0x18,0x70,0x13,0x05}; 
static char hx8369_tianma_tn_para_0xb6[3]={0xB6,0x4a,0x4a};
static char hx8369_tianma_tn_para_0xd5[27]={0xD5,0x00,0x0e,0x03,0x29,0x01,0x0f,0x28,0x60,0x11,0x13,0x00,
	0x00,0x40,0x26,0x51,0x37,0x00,0x00,0x71,0x35,0x60,0x24,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_tn_para_0xe0[35]={0xE0,0x00,0x02,0x0b,0x0a,0x09,0x18,0x1d,0x2a,0x08,0x11,0x0d,
	0x13,0x15,0x14,0x15,0x0f,0x14,0x00,0x02,0x0b,0x0a,0x09,0x18,0x1d,0x2a,0x08,0x11,0x0d,0x13,0x15,
	0x14,0x15,0x0f,0x14};
static char hx8369_tianma_tn_para_0xcc[2]={0xcc,0x00}; 
static char hx8369_tianma_tn_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_tn_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE
static struct dsi_cmd_desc hx8369_tianma_tn_display_on_cmds[] = 
{

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb1), hx8369_tianma_tn_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb2), hx8369_tianma_tn_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb4), hx8369_tianma_tn_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb6), hx8369_tianma_tn_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xd5),hx8369_tianma_tn_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xe0), hx8369_tianma_tn_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0x3a), hx8369_tianma_tn_para_0x3a},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xcc), hx8369_tianma_tn_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xba), hx8369_tianma_tn_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
/**************************************
5. hx8369 tianma IPS start 
**************************************/
static char hx8369_tianma_ips_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0D,0x0D,0x1A,0x22,0x3F,
	0x3F,0x01,0x23,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_ips_para_0xb2[16]={0xB2,0x00,0x23,0x05,0x05,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_tianma_ips_para_0xb4[6]={0xB4,0x00,0x18,0x80,0x06,0x02}; 
static char hx8369_tianma_ips_para_0xb6[3]={0xB6,0x42,0x42};
static char hx8369_tianma_ips_para_0xd5[27]={0xD5,0x00,0x09,0x03,0x29,0x01,0x0A,0x28,0x70,
	0x11,0x13,0x00,0x00,0x40,0x26,0x51,0x37,0x00,0x00,0x71,0x35,0x60,0x24,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_ips_para_0xe0[35]={0xE0,0x00,0x0A,0x0F,0x2E,0x33,0x3F,0x1D,0x3E,0x07,0x0D,0x0F,
	0x12,0x15,0x13,0x15,0x10,0x17,0x00,0x0A,0x0F,0x2E,0x33,0x3F,0x1D,0x3E,0x07,0x0D,
	0x0F,0x12,0x15,0x13,0x15,0x10,0x17};
static char hx8369_tianma_ips_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_ips_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE

static struct dsi_cmd_desc hx8369_tianma_ips_display_on_cmds[] = 
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


/**************************************
6. nt35510 lead start 
**************************************/

static char nt35510_lead_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt35510_lead_cmd_page1_bc[4] = {0xbc, 0x00,0x78,0x1A};
static char nt35510_lead_cmd_page1_bd[4] = {0xbd, 0x00,0x78,0x1A};
static char nt35510_lead_cmd_page1_be[3] = {0xbe, 0x00,0x4E};
static char nt35510_lead_cmd_page1_b0[4] = {0xb0, 0x00,0x00,0x00};
static char nt35510_lead_cmd_page1_b1[4] = {0xb1, 0x00,0x00,0x00};
//static char lead_cmd_page1_b7[4] = {0xb7, 0x44,0x44,0x44};
//static char lead_cmd_page1_b3[4] = {0xb1, 0x0B,0x0B,0x0B};
static char nt35510_lead_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};
static char nt35510_lead_cmd_page1_ba[4] = {0xba, 0x16,0x16,0x16};

static char nt35510_lead_cmd_page1_b6[4] = {0xb6, 0x36,0x36,0x36};
static char nt35510_lead_cmd_page1_b7[4] = {0xb7, 0x26,0x26,0x26};
static char nt35510_lead_cmd_page1_b8[4] = {0xb8, 0x26,0x26,0x26};
static char nt35510_lead_cmd_page1_d1[] = {0xD1,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d2[] = {0xD2,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d3[] = {0xD3,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d4[] = {0xD4,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d5[] = {0xD5,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d6[] = {0xD6,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};

static char nt35510_lead_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt35510_lead_cmd_page0_b1[2] = {0xb1, 0xf8};   //0xcc
static char nt35510_lead_cmd_page0_b4[2] = {0xb4, 0x10};
static char nt35510_lead_cmd_page0_b6[2] = {0xb6, 0x02};
static char nt35510_lead_cmd_page0_b7[3] = {0xb7, 0x71,0x71};
static char nt35510_lead_cmd_page0_b8[5] = {0xb8, 0x01,0x0A,0x0A,0x0A};
static char nt35510_lead_cmd_page0_bc[4] = {0xbc, 0x05,0x05,0x05};
static char nt35510_lead_cmd_page0_bd[6] = {0xbd, 0x01,0x84,0x07,0x31,0x00};
static char nt35510_lead_cmd_page0_be[6] = {0xbe, 0x01,0x84,0x07,0x31,0x00};
static char nt35510_lead_cmd_page0_bf[6] = {0xbf, 0x01,0x84,0x07,0x31,0x00};

static char nt35510_lead_cmd_page0_c7[2] = {0xc7, 0x02};
static char nt35510_lead_cmd_page0_c9[5] = {0xc9, 0x11,0x00,0x00,0x00};
static char nt35510_lead_cmd_page0_3a[2] = {0x3a, 0x77};
static char nt35510_lead_cmd_page0_35[2] = {0x35, 0x00};
static char nt35510_lead_cmd_page0_21[2] = {0x21, 0x00};




static struct dsi_cmd_desc nt35510_lead_display_on_cmds[] = {

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_f0), nt35510_lead_cmd_page1_f0},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bc), nt35510_lead_cmd_page1_bc},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bd), nt35510_lead_cmd_page1_bd},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_be), nt35510_lead_cmd_page1_be},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b0), nt35510_lead_cmd_page1_b0},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b1), nt35510_lead_cmd_page1_b1},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b9), nt35510_lead_cmd_page1_b9},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_ba), nt35510_lead_cmd_page1_ba},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b6), nt35510_lead_cmd_page1_b6},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b7), nt35510_lead_cmd_page1_b7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b8), nt35510_lead_cmd_page1_b8},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d1), nt35510_lead_cmd_page1_d1},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d2), nt35510_lead_cmd_page1_d2},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d3), nt35510_lead_cmd_page1_d3},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d4), nt35510_lead_cmd_page1_d4},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d5), nt35510_lead_cmd_page1_d5},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d6), nt35510_lead_cmd_page1_d6},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_f0), nt35510_lead_cmd_page0_f0},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b1), nt35510_lead_cmd_page0_b1},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b4), nt35510_lead_cmd_page0_b4},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b6), nt35510_lead_cmd_page0_b6},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b7), nt35510_lead_cmd_page0_b7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b8), nt35510_lead_cmd_page0_b8},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bc), nt35510_lead_cmd_page0_bc},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bd), nt35510_lead_cmd_page0_bd},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_be), nt35510_lead_cmd_page0_be},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bf), nt35510_lead_cmd_page0_bf},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_f0), nt35510_lead_cmd_page0_f0},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_c7), nt35510_lead_cmd_page0_c7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_c9), nt35510_lead_cmd_page0_c9},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_3a), nt35510_lead_cmd_page0_3a},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_35), nt35510_lead_cmd_page0_35},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_21), nt35510_lead_cmd_page0_21},
//{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(lead_cmd_page0_36), lead_cmd_page0_36},
{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

/**************************************
7. nt35510 yushun start 
**************************************/
//static char cmd_page_f3[9] = {0xf3, 0x00,0x32,0x00,0x38,0x31,0x08,0x11,0x00};
static char nt3511_yushun_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt3511_yushun_cmd_page0_b0[6] = {0xb0, 0x04,0x0a,0x0e,0x09,0x04};
static char nt3511_yushun_cmd_page0_b1[3] = {0xb1, 0x18,0x04};
static char nt3511_yushun_cmd_page0_36[2] = {0x36, 0x90};
static char nt3511_yushun_cmd_page0_b3[2] = {0xb3, 0x00};
static char nt3511_yushun_cmd_page0_b6[2] = {0xb6, 0x03};
static char nt3511_yushun_cmd_page0_b7[3] = {0xb7, 0x70,0x70};
static char nt3511_yushun_cmd_page0_b8[5] = {0xb8, 0x00,0x06,0x06,0x06};
static char nt3511_yushun_cmd_page0_bc[4] = {0xbc, 0x00,0x00,0x00};
static char nt3511_yushun_cmd_page0_bd[6] = {0xbd, 0x01,0x84,0x06,0x50,0x00};
static char nt3511_yushun_cmd_page0_cc[4] = {0xcc, 0x03,0x01,0x06};

static char nt3511_yushun_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt3511_yushun_cmd_page1_b0[4] = {0xb0, 0x05,0x05,0x05};
static char nt3511_yushun_cmd_page1_b1[4] = {0xb1, 0x05,0x05,0x05};
static char nt3511_yushun_cmd_page1_b2[4] = {0xb2, 0x03,0x03,0x03};
static char nt3511_yushun_cmd_page1_b8[4] = {0xb8, 0x25,0x25,0x25};
static char nt3511_yushun_cmd_page1_b3[4] = {0xb3, 0x0b,0x0b,0x0b};
static char nt3511_yushun_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};
static char nt3511_yushun_cmd_page1_bf[2] = {0xbf, 0x01};
static char nt3511_yushun_cmd_page1_b5[4] = {0xb5, 0x08,0x08,0x08};
static char nt3511_yushun_cmd_page1_ba[4] = {0xba, 0x24,0x24,0x24};
static char nt3511_yushun_cmd_page1_b4[4] = {0xb4, 0x2e,0x2e,0x2e};
static char nt3511_yushun_cmd_page1_bc[4] = {0xbc, 0x00,0x68,0x00};
static char nt3511_yushun_cmd_page1_bd[4] = {0xbd, 0x00,0x7c,0x00};
static char nt3511_yushun_cmd_page1_be[3] = {0xbe, 0x00,0x45};
static char nt3511_yushun_cmd_page1_d0[5] = {0xd0, 0x0c,0x15,0x0b,0x0e};

static char nt3511_yushun_cmd_page1_d1[53] = {0xd1, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d2[53] = {0xd2, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d3[53] = {0xd3, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d4[53] = {0xd4, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d5[53] = {0xd5, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d6[53] = {0xd6, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};

static struct dsi_cmd_desc nt3511_yushun_display_on_cmds[] = {

       // yushun nt35510
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_page_ff),cmd_page_ff},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_page_f3),cmd_page_f3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_f0),nt3511_yushun_cmd_page0_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b0),nt3511_yushun_cmd_page0_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b1),nt3511_yushun_cmd_page0_b1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_36),nt3511_yushun_cmd_page0_36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b3),nt3511_yushun_cmd_page0_b3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b6),nt3511_yushun_cmd_page0_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b7),nt3511_yushun_cmd_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b8),nt3511_yushun_cmd_page0_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_bc),nt3511_yushun_cmd_page0_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_bd),nt3511_yushun_cmd_page0_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_cc),nt3511_yushun_cmd_page0_cc},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_f0),nt3511_yushun_cmd_page1_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b0),nt3511_yushun_cmd_page1_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b1),nt3511_yushun_cmd_page1_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b2),nt3511_yushun_cmd_page1_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b8),nt3511_yushun_cmd_page1_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b3),nt3511_yushun_cmd_page1_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b9),nt3511_yushun_cmd_page1_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bf),nt3511_yushun_cmd_page1_bf},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b5),nt3511_yushun_cmd_page1_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_ba),nt3511_yushun_cmd_page1_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b4),nt3511_yushun_cmd_page1_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bc),nt3511_yushun_cmd_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bd),nt3511_yushun_cmd_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_be),nt3511_yushun_cmd_page1_be},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d0),nt3511_yushun_cmd_page1_d0},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d1), nt3511_yushun_cmd_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d2), nt3511_yushun_cmd_page1_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d3), nt3511_yushun_cmd_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d4), nt3511_yushun_cmd_page1_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d5), nt3511_yushun_cmd_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d6), nt3511_yushun_cmd_page1_d6},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};


/**************************************
8. 461408 truly start 
**************************************/
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


static struct dsi_cmd_desc r61408_truly_lg_display_on_cmds[] = 
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

static char HX8369B_TM04YVHP12_para_01[]= {0xB9,0xFF,0x83,0x69};
static char HX8369B_TM04YVHP12_para_02[]= {0xB1,0x13,0x83,0x77,0x00,0x0F,0x0F,0x1F,0x1F};
static char HX8369B_TM04YVHP12_para_03[]= {0xB2,0x00,0x10,0x02};
static char HX8369B_TM04YVHP12_para_04[]= {0xB3,0x83,0x00,0x31,0x03};
static char HX8369B_TM04YVHP12_para_05[]= {0xB4,0x12};
static char HX8369B_TM04YVHP12_para_06[]= {0xB5,0x15,0x15,0x3E};
static char HX8369B_TM04YVHP12_para_07[]= {0xB6,0xA7,0xA7};
static char HX8369B_TM04YVHP12_para_08[]= 
{
0xE0,0x00,0x11,0x1C,0x35,0x3B,0x3F,0x33,
0x49,0x08,0x0D,0x10,0x14,0x16,0x13,0x14,
0x19,0x1F,0x00,0x11,0x1C,0x35,0x3B,0x3F,
0x33,0x49,0x08,0x0D,0x10,0x14,0x16,0x13,
0x14,0x19,0x1F,0x01
};
static char HX8369B_TM04YVHP12_para_09[]= {0xBC,0x5E};
static char HX8369B_TM04YVHP12_para_10[]= {0xC6,0x40};
static char HX8369B_TM04YVHP12_para_11[]= 
{
0xD5,0x00,0x00,0x12,0x03,0x33,0x00,0x00,
0x10,0x01,0x00,0x00,0x00,0x10,0x40,0x14,
0x00,0x00,0x23,0x10,0x3e,0x13,0x00,0x00,
0x00,0xC3,0x00,0x00,0x00,0x00,0x03,0x00,
0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x89,
0x00,0x66,0x11,0x33,0x11,0x00,0x00,0x00,
0x98,0x00,0x00,0x22,0x00,0x44,0x00,0x00,
0x00,0x89,0x00,0x44,0x00,0x22,0x99,0x00,
0x00,0x00,0x98,0x00,0x11,0x33,0x11,0x55,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x01,0x5A
};
static char HX8369B_TM04YVHP12_para_12[]= {0xEA,0x62};
static char HX8369B_TM04YVHP12_para_13[]= 
{
0xBA,0x41,0x00,0x16,0xC6,0x80,0x0A,0x00,
0x10,0x24,0x02,0x21,0x21,0x9A,0x11,0x14
};
static char HX8369B_TM04YVHP12_para_14[]= 
{
0xcc,0x02
};


static struct dsi_cmd_desc HX8369B_TM04YVHP12_display_on_cmds[] = 
{
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_01), HX8369B_TM04YVHP12_para_01},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_02), HX8369B_TM04YVHP12_para_02},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_03), HX8369B_TM04YVHP12_para_03},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_04), HX8369B_TM04YVHP12_para_04},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_05), HX8369B_TM04YVHP12_para_05},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_06), HX8369B_TM04YVHP12_para_06},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_07), HX8369B_TM04YVHP12_para_07},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_08), HX8369B_TM04YVHP12_para_08},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_09), HX8369B_TM04YVHP12_para_09},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_10), HX8369B_TM04YVHP12_para_10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_11), HX8369B_TM04YVHP12_para_11},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_12), HX8369B_TM04YVHP12_para_12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_13), HX8369B_TM04YVHP12_para_13},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(HX8369B_TM04YVHP12_para_14), HX8369B_TM04YVHP12_para_14},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_on), display_on}
};

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

	static struct dsi_cmd_desc boe_nt35510_display_on_cmds[] = {

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

static char hx8363_cmi_para_01[]={0x00,0x00};		  // shift address	
static char hx8363_cmi_para_02[]={0xff,0x80,0x09,0x01}; // enable EXTC
static char hx8363_cmi_para_03[]={0x00,0x80};		  // shift address
static char hx8363_cmi_para_04[]={0xff,0x80,0x09};	  // enable Orise mode
static char hx8363_cmi_para_05[]={0x00,0xB4};		  // Column Inversion
static char hx8363_cmi_para_06[]={0xc0,0x50};
static char hx8363_cmi_para_07[]={0x00,0x90};			// pump
static char hx8363_cmi_para_08[]={0xc5,0x96,0x76};
static char hx8363_cmi_para_09[]={0x00,0x82};			//	REG-Pump23 AVEE VCL
static char hx8363_cmi_para_10[]={0xc5,0xb3};
static char hx8363_cmi_para_11[]={0x00,0x00};			// GVDD 4.75V NGVDD 4.75V
static char hx8363_cmi_para_12[]={0xD8,0x83,0x83};
static char hx8363_cmi_para_13[]={0x00,0x00};			// VCOM
static char hx8363_cmi_para_14[]={0xD9,0x79};			// -1.8
static char hx8363_cmi_para_15[]={0x00,0x92};
static char hx8363_cmi_para_16[]={0xC5,0x01};
static char hx8363_cmi_para_17[]={0x00,0xB1};			// VDD_18V=1.6V GVDD test on
static char hx8363_cmi_para_18[]={0xc5,0xA9};
static char hx8363_cmi_para_19[]={0x00,0x90};
static char hx8363_cmi_para_20[]={0xc0,0x00,0x44,0x00,0x00,0x00,0x03};	
static char hx8363_cmi_para_21[]={0x00,0xa6};
static char hx8363_cmi_para_22[]={0xc1,0x01,0x00,0x00};
static char hx8363_cmi_para_23[]={0x00,0x80};  // shift address
static char hx8363_cmi_para_24[]={0xce,0x85,0x03,0x00,0x84,0x03,0x00,0x87,0x03,0x00,0x86,0x03,0x00};
static char hx8363_cmi_para_25[]={0x00,0x90};  // shift address
static char hx8363_cmi_para_26[]={0xce,0x33,0x26,0x00,0x33,0x27,0x00};
static char hx8363_cmi_para_27[]={0x00,0xa0};  // shift address
static char hx8363_cmi_para_28[]={0xCE,0x38,0x03,0x03,0x20,0x00,0x00,0x00,0x38,0x02,0x03,0x21,0x00,0x00,0x00};
static char hx8363_cmi_para_29[]={0x00,0xb0};  // shift address
static char hx8363_cmi_para_30[]={0xCE,0x38,0x01,0x03,0x22,0x00,0x00,0x00,0x38,0x00,0x03,0x23,0x00,0x00,0x00};
static char hx8363_cmi_para_31[]={0x00,0xc0};  // shift address
static char hx8363_cmi_para_32[]={0xCE,0x30,0x00,0x03,0x24,0x00,0x00,0x00,0x30,0x01,0x03,0x25,0x00,0x00,0x00};
static char hx8363_cmi_para_33[]={0x00,0xd0};  // shift address
static char hx8363_cmi_para_34[]={0xCE,0x30,0x02,0x03,0x26,0x00,0x00,0x00,0x30,0x03,0x03,0x27,0x00,0x00,0x00};
static char hx8363_cmi_para_35[]={0x00,0xc7};  // shift address
static char hx8363_cmi_para_36[]={0xCF,0x00};
static char hx8363_cmi_para_37[]={0x00,0xc0};  // shift address
static char hx8363_cmi_para_38[]={0xCB,0x00,0x00,0x00,0x00,0x54,0x54,0x54,0x54,0x00,0x54,0x00,0x54,0x00,0x00,0x00};
static char hx8363_cmi_para_39[]={0x00,0xD0};  // shift address
static char hx8363_cmi_para_40[]={0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x54,0x54,0x54,0x54,0x00,0x54};
static char hx8363_cmi_para_41[]={0x00,0xE0};  // shift address
static char hx8363_cmi_para_42[]={0xCB,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00};		
static char hx8363_cmi_para_43[]={0x00,0x80};  // shift address
static char hx8363_cmi_para_44[]={0xCC,0x00,0x00,0x00,0x00,0x0c,0x0a,0x10,0x0e,0x00,0x02};
static char hx8363_cmi_para_45[]={0x00,0x90};  // shift address
static char hx8363_cmi_para_46[]={0xCC,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0b};
static char hx8363_cmi_para_47[]={0x00,0xA0};  // shift address
static char hx8363_cmi_para_48[]={0xCC,0x09,0x0f,0x0d,0x00,0x01,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char hx8363_cmi_para_49[]={0x00,0xB0};  // shift address
static char hx8363_cmi_para_50[]={0xCC,0x00,0x00,0x00,0x00,0x0d,0x0f,0x09,0x0b,0x00,0x05};
static char hx8363_cmi_para_51[]={0x00,0xC0};  // shift address
static char hx8363_cmi_para_52[]={0xCC,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e};
static char hx8363_cmi_para_53[]={0x00,0xD0};  // shift address
static char hx8363_cmi_para_54[]={0xCC,0x10,0x0a,0x0c,0x00,0x06,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char hx8363_cmi_para_55[]={0x00,0xB2};
static char hx8363_cmi_para_56[]={0xF5,0x15,0x00,0x15,0x00};
static char hx8363_cmi_para_57[]={0x00,0x87};
static char hx8363_cmi_para_58[]={0xC4,0x00,0x80,0x00};
static char hx8363_cmi_para_59[]={0x00,0xC0};
static char hx8363_cmi_para_60[]={0xC5,0x00};
static char hx8363_cmi_para_61[]={0x00,0x8B};
static char hx8363_cmi_para_62[]={0xB0,0x40};
static char hx8363_cmi_para_63[]={0x00,0xA0};
static char hx8363_cmi_para_64[]={0xC1,0xEA};
static char hx8363_cmi_para_65[]={0x00,0xa6};			// turn off zigzag
static char hx8363_cmi_para_66[]={0xb3,0x20,0x01};
static char hx8363_cmi_para_67[]={0x00,0x00};
static char hx8363_cmi_para_68[]={0xE1,0x00,0x0C,0x0E,0x10,0x06,0x0F,0x09,0x09,0x05,0x07,0x0E,0x08,0x0F,0x0F,0x0B,0x06};
static char hx8363_cmi_para_69[]={0x00,0x00};
static char hx8363_cmi_para_70[]={0xE2,0x00,0x0C,0x0E,0x10,0x06,0x0F,0x09,0x09,0x05,0x07,0x0E,0x08,0x0F,0x0F,0x0B,0x06};
static char hx8363_cmi_para_71[]={0x00,0x00};		  // shift address	
static char hx8363_cmi_para_72[]={0xff,0x00,0x00,0x00}; // enable EXTC
static char hx8363_cmi_para_73[]={0x00,0x80};		  // shift address
static char hx8363_cmi_para_74[]={0xff,0x00,0x00};	  // enable Orise mode
static char hx8363_cmi_para_0x11[]={0x11,0x00};
static char hx8363_cmi_para_0x29[]={0x29,0x00};
static struct dsi_cmd_desc hx8363_cmi_display_on_cmds[] = 
{
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_01), hx8363_cmi_para_01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_02), hx8363_cmi_para_02},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_03), hx8363_cmi_para_03},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_04), hx8363_cmi_para_04},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_05), hx8363_cmi_para_05},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_06), hx8363_cmi_para_06},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_07), hx8363_cmi_para_07},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_08), hx8363_cmi_para_08},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_09), hx8363_cmi_para_09},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_10), hx8363_cmi_para_10},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_11), hx8363_cmi_para_11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_12), hx8363_cmi_para_12},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_13), hx8363_cmi_para_13},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_14), hx8363_cmi_para_14},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_15), hx8363_cmi_para_15},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_16), hx8363_cmi_para_16},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_17), hx8363_cmi_para_17},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_18), hx8363_cmi_para_18},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_19), hx8363_cmi_para_19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_20), hx8363_cmi_para_20},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_21), hx8363_cmi_para_21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_22), hx8363_cmi_para_22},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_23), hx8363_cmi_para_23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_24), hx8363_cmi_para_24},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_25), hx8363_cmi_para_25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_26), hx8363_cmi_para_26},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_27), hx8363_cmi_para_27},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_28), hx8363_cmi_para_28},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_29), hx8363_cmi_para_29},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_30), hx8363_cmi_para_30},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_31), hx8363_cmi_para_31},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_32), hx8363_cmi_para_32},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_33), hx8363_cmi_para_33},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_34), hx8363_cmi_para_34},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_35), hx8363_cmi_para_35},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_36), hx8363_cmi_para_36},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_37), hx8363_cmi_para_37},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_38), hx8363_cmi_para_38},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_39), hx8363_cmi_para_39},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_40), hx8363_cmi_para_40},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_41), hx8363_cmi_para_41},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_42), hx8363_cmi_para_42},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_43), hx8363_cmi_para_43},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_44), hx8363_cmi_para_44},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_45), hx8363_cmi_para_45},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_46), hx8363_cmi_para_46},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_47), hx8363_cmi_para_47},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_48), hx8363_cmi_para_48},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_49), hx8363_cmi_para_49},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_50), hx8363_cmi_para_50},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_51), hx8363_cmi_para_51},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_52), hx8363_cmi_para_52},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_53), hx8363_cmi_para_53},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_54), hx8363_cmi_para_54},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_55), hx8363_cmi_para_55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_56), hx8363_cmi_para_56},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_57), hx8363_cmi_para_57},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_58), hx8363_cmi_para_58},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_59), hx8363_cmi_para_59},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_60), hx8363_cmi_para_60},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_61), hx8363_cmi_para_61},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_62), hx8363_cmi_para_62},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_63), hx8363_cmi_para_63},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_64), hx8363_cmi_para_64},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_65), hx8363_cmi_para_65},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_66), hx8363_cmi_para_66},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_67), hx8363_cmi_para_67},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_68), hx8363_cmi_para_68},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_69), hx8363_cmi_para_69},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_70), hx8363_cmi_para_70},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_71), hx8363_cmi_para_71},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_72), hx8363_cmi_para_72},
	{DTYPE_GEN_WRITE2, 1, 0, 0, 0, sizeof(hx8363_cmi_para_73), hx8363_cmi_para_73},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(hx8363_cmi_para_74), hx8363_cmi_para_74},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(hx8363_cmi_para_0x11), hx8363_cmi_para_0x11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(hx8363_cmi_para_0x29), hx8363_cmi_para_0x29},
};


static uint32 mipi_get_commic_panleid_cmi(struct msm_fb_data_type *mfd,struct dsi_cmd_desc *para,uint32 len,int mode)
{
	uint32 panelid = 0;
	mipi_dsi_buf_init(&novatek_tx_buf);
	mipi_dsi_buf_init(&novatek_rx_buf);
	mipi_dsi_cmd_bta_sw_trigger(); 
	if(mode)
		mipi_set_tx_power_mode(1);
	else 
		mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_rx(mfd,&novatek_tx_buf, &novatek_rx_buf, para,len);
	if(mode)
		mipi_set_tx_power_mode(0);
	panelid = *(uint32 *)(novatek_rx_buf.data+7);
	printk("lizhiye, debug mipi_get_commic_panleid_cmi panelid is %x\n",panelid);
	
	return panelid;
}

static uint32 mipi_get_CMI_panleid(struct msm_fb_data_type *mfd)
{
	uint32 panleid =  mipi_get_commic_panleid_cmi(mfd,&cmi_icid_rd_cmd,4,0);
	return panleid;
}

struct mipi_manufacture_ic {
	struct dsi_cmd_desc *readid_tx;
	int readid_len_tx;
	struct dsi_cmd_desc *readid_rx;
	int readid_len_rx;
	int mode;
};

static int mipi_get_manufacture_icid(struct msm_fb_data_type *mfd)
{
	uint32 icid = 0;
	int i ;
	uint32 icid_cmi = 0;
	

	 struct mipi_manufacture_ic mipi_manufacture_icid[] = {	 	
	 	
		{hx8363_setpassword_cmd,ARRAY_SIZE(hx8363_setpassword_cmd),&hx8363_icid_rd_cmd,3,1},
		{nt3511_setpassword_cmd,ARRAY_SIZE(nt3511_setpassword_cmd),&nt3511_icid_rd_cmd,3,0},
		{hx8369b_setpassword_cmd,ARRAY_SIZE(hx8369b_setpassword_cmd),&hx8369b_icid_rd_cmd,3,1},
		{hx8369_setpassword_cmd,ARRAY_SIZE(hx8369_setpassword_cmd),&hx8369_icid_rd_cmd,3,1},
		{r61408_setpassword_cmd,ARRAY_SIZE(r61408_setpassword_cmd),&r61408_icid_rd_cmd,4,1},
	 };

	for(i = 0; i < ARRAY_SIZE(mipi_manufacture_icid) ; i++)		
	{	lcd_panle_reset();	
		mipi_dsi_buf_init(&novatek_tx_buf);
		mipi_dsi_buf_init(&novatek_rx_buf);
		mipi_set_tx_power_mode(1);		
		mipi_dsi_cmds_tx(&novatek_tx_buf, mipi_manufacture_icid[i].readid_tx,mipi_manufacture_icid[i].readid_len_tx);
		mipi_dsi_cmd_bta_sw_trigger(); 
		
		if(!mipi_manufacture_icid[i].mode)
			mipi_set_tx_power_mode(0);		
		mipi_dsi_cmds_rx(mfd,&novatek_tx_buf, &novatek_rx_buf, mipi_manufacture_icid[i].readid_rx,mipi_manufacture_icid[i].readid_len_rx);

		if(mipi_manufacture_icid[i].mode)
			mipi_set_tx_power_mode(0);
		
		icid = *(uint32 *)(novatek_rx_buf.data);
		
		printk("debug read icid is %x\n",icid & 0xffffff);

		switch(icid & 0xffffff){
			case 0x1055:
						return NOVATEK_35510;
			case 0x6383ff:
						return HIMAX_8363;
						
			case 0x6983ff:
						return HIMAX_8369;
			case 0x142201:
						return RENESAS_R61408;
			case 0x5a6983:
						return HIMAX_8369B;
			default:
						break;			
		}

	}
	icid_cmi = mipi_get_CMI_panleid(mfd);
	if((icid_cmi & 0xffff) == 0x0980)
		return CMI_8001;
	else
	return 0;
}

static uint32 mipi_get_commic_panleid(struct msm_fb_data_type *mfd,struct dsi_cmd_desc *para,uint32 len,int mode)
{
	uint32 panelid = 0;
	mipi_dsi_buf_init(&novatek_tx_buf);
	mipi_dsi_buf_init(&novatek_rx_buf);
	mipi_dsi_cmd_bta_sw_trigger(); 
	if(mode)
		mipi_set_tx_power_mode(1);
	else 
		mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_rx(mfd,&novatek_tx_buf, &novatek_rx_buf, para,len);
	if(mode)
		mipi_set_tx_power_mode(0);
	panelid = *(uint32 *)(novatek_rx_buf.data);
	printk("debug read panelid is %x\n",panelid & 0xffffffff);
	return panelid;
}

static uint32 mipi_get_himax8369_panleid(struct msm_fb_data_type *mfd)
{

	uint32 panleid;
	panleid =  mipi_get_commic_panleid(mfd,&hx8369_panleid_rd_cmd,1,1);
	switch((panleid>>8) & 0xff){
		case HIMAX8369_TIANMA_TN_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN;
		case HIMAX8369_TIANMA_IPS_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_IPS;
		case HIMAX8369_LEAD_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD;
		case HIMAX8369_LEAD_HANNSTAR_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD_HANNSTAR;
		default:
				return (u32)LCD_PANEL_NOPANEL;
	}
}

static uint32 mipi_get_himax8369B_panleid(struct msm_fb_data_type *mfd)
{

	uint32 panleid;
	panleid =  mipi_get_commic_panleid(mfd,&hx8369b_panleid_rd_cmd,1,1);
	switch((panleid>>8) & 0xff){
		case 0x69:
				return (u32)LCD_PANEL_4P0_HX8369B_TM04YVHP12;
		default:
				return (u32)LCD_PANEL_NOPANEL;
	}
}


static uint32 mipi_get_nt35510_panleid(struct msm_fb_data_type *mfd)
{

	uint32 panleid =  mipi_get_commic_panleid(mfd,&nt3511_panleid_rd_cmd,1,0);
	switch(panleid&0xff){
		case NT35510_YUSHUN_ID:
				return  (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN	;
		case NT35510_LEAD_ID:
				return (u32)LCD_PANEL_4P0_NT35510_LEAD;
		case NT35510_BOE_ID:
				return (u32)LCD_PANEL_4P0_NT35510_BOE_BOE;
		default:
				return (u32)LCD_PANEL_NOPANEL;
	}
}

static uint32 mipi_get_himax8363_panleid(struct msm_fb_data_type *mfd)
{

	uint32 panleid =  mipi_get_commic_panleid(mfd, &hx8363_panleid_rd_cmd,1,1);
	
	switch((panleid >> 8) & 0xff)
	{
		case HIMAX8369_YUSHUN_IVO_ID:	//85	--
			return (u32)LCD_PANEL_4P0_HX8363_IVO_YUSHUN;
		default:
			return (u32)LCD_PANEL_4P0_HX8363_CMI_YASSY;		//00	--
	}
}

static uint32 mipi_get_icpanleid(struct msm_fb_data_type *mfd )
{
	int icid = 0;

	lcd_panle_reset();

	icid = mipi_get_manufacture_icid(mfd);


	switch(icid){
		case HIMAX_8363:					
           				 LcdPanleID = mipi_get_himax8363_panleid(mfd);
					break;
		case HIMAX_8369:
					LcdPanleID = mipi_get_himax8369_panleid(mfd);
					break;
		case NOVATEK_35510:
					LcdPanleID = mipi_get_nt35510_panleid(mfd);
					break;
		case RENESAS_R61408:
					LcdPanleID = LCD_PANEL_4P0_R61408_TRULY_LG;
			break;
		case HIMAX_8369B:
					LcdPanleID = mipi_get_himax8369B_panleid(mfd);
					break;
		case CMI_8001:
					LcdPanleID = LCD_PANEL_4P0_HX8363_IVO_CMI;
					break;
		default:
					LcdPanleID = (u32)LCD_PANEL_NOPANEL;
					printk("warnning cann't indentify the chip\n");
					break;
	}
		
	return LcdPanleID;
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
		
		if (LcdPanleID != LCD_PANEL_NOPANEL)
			return 0;
		else
			mipi_get_icpanleid(mfd);				
	}
	
	printk("\n lcd panel on  start");
	
	lcd_panle_reset();
	
	if (mipi->mode == DSI_VIDEO_MODE) {
		switch(LcdPanleID)
		{
		case (u32)LCD_PANEL_4P0_HX8363_CMI_YASSY:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8363_yassy_display_on_cmds,ARRAY_SIZE(hx8363_yassy_display_on_cmds));
				printk("HIMAX8363_YASS init ok !!\n");
				break;
		case (u32)LCD_PANEL_4P0_HX8363_IVO_YUSHUN:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8363_yushun_display_on_cmds,ARRAY_SIZE(hx8363_yushun_display_on_cmds));
				printk("HIMAX8363_yushun init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_LEAD:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8369_lead_display_on_cmds,ARRAY_SIZE(hx8369_lead_display_on_cmds));
				printk("HIMAX8369_LEAD init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_LEAD_HANNSTAR:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8369_lead_hannstar_display_on_cmds,ARRAY_SIZE(hx8369_lead_hannstar_display_on_cmds));
				printk("HIMAX8369_LEAD_HANNSTAR init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8369_tianma_tn_display_on_cmds,ARRAY_SIZE(hx8369_tianma_tn_display_on_cmds));
				printk("HIMAX8369_TIANMA_TN init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_IPS:
				mipi_dsi_cmds_tx(&novatek_tx_buf, hx8369_tianma_ips_display_on_cmds,ARRAY_SIZE(hx8369_tianma_ips_display_on_cmds));
				printk("HIMAX8369_TIANMA_IPS init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_NT35510_LEAD:
				mipi_dsi_cmds_tx(&novatek_tx_buf, nt35510_lead_display_on_cmds,ARRAY_SIZE(nt35510_lead_display_on_cmds));
				printk("NT35510_LEAD init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN:
				mipi_dsi_cmds_tx(&novatek_tx_buf, nt3511_yushun_display_on_cmds,ARRAY_SIZE(nt3511_yushun_display_on_cmds));
				printk("NT35510_HYDIS_YUSHUN init ok !!\n");
				break;
		case (u32)LCD_PANEL_4P0_R61408_TRULY_LG:
				mipi_dsi_cmds_tx( &novatek_tx_buf, r61408_truly_lg_display_on_cmds,ARRAY_SIZE(r61408_truly_lg_display_on_cmds));
				printk("R61408 TRULY LG  init ok !!\n");
			break;
		case (u32)LCD_PANEL_4P0_HX8369B_TM04YVHP12:
				mipi_dsi_cmds_tx( &novatek_tx_buf, HX8369B_TM04YVHP12_display_on_cmds,ARRAY_SIZE(HX8369B_TM04YVHP12_display_on_cmds));
				printk("lizhiye, LCD_PANEL_4P0_HX8369B_TM04YVHP12  init ok !!\n");
			break;
		case (u32)LCD_PANEL_4P0_NT35510_BOE_BOE:
				mipi_dsi_cmds_tx( &novatek_tx_buf, boe_nt35510_display_on_cmds,ARRAY_SIZE(boe_nt35510_display_on_cmds));
				printk(" boe boe net5510  init ok !!\n");
			break;	
		case (u32)LCD_PANEL_4P0_HX8363_IVO_CMI:
				mipi_dsi_cmds_tx( &novatek_tx_buf, hx8363_cmi_display_on_cmds,ARRAY_SIZE(hx8363_cmi_display_on_cmds));
				printk("lizhiye, HIMAX8363_cmi init ok !!\n");
				break;	
			
		default:
				printk("can't get jfldjfldicpanelid value\n");
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

struct dcs_cmd_req cmdreq;


static bool onewiremode = false;
static int lcd_bkl_ctl=12;
static void myudelay(unsigned int usec)
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

    	if(current_lel > 27)
    	{
        	current_lel = 27;
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
	return snprintf((char *)buf, sizeof(*buf), "%u\n", barrier_mode);
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
