/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_novatek.h"

static struct msm_panel_info pinfo;
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	
#if 1//back 1540
		/* 720*1280, RGB888, 4 Lane 60	fps video mode */
		/* regulator */
		{0x03, 0x0a, 0x04, 0x00, 0x20},
		/* timing */
		{0x7b, 0x1c, 0x11, 0x00, 0x31, 0x37, 0x16, 0x20,
		0x21, 0x03, 0x04, 0xa0},
		/* phy ctrl */
		{0x5f, 0x00, 0x00, 0x10},
		/* strength */
		{0xff, 0x00, 0x06, 0x00},
		/* pll control */
		{0x01, 0x22, 0x30, 0xc1, 0x00, 0x5f, 0x48, 0x63,
		0x31, 0x0f, 0x03,//0x41, 0x0f, 0x01,
		0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#endif

#if 0//back 1540
	/* 720*1280, RGB888, 4 Lane 60  fps video mode */
	/* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x6f, 0x18, 0x10, 0x00, 0x3b, 0x3f, 0x14, 0x1b,
	0x1b, 0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x01, 0x84, 0x31, 0xda, 0x00, 0x5f, 0x48, 0x63,
	0x31, 0x0f, 0x03,//0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#endif
#if 0//55HZ bob modify
		/* 720*1280, RGB888, 4 Lane 55  fps video mode */
    /* regulator */
	{0x03, 0x0a, 0x04, 0x00, 0x20},
	/* timing */
	{0x6f, 0x18, 0x10, 0x00, 0x3b, 0x3f, 0x14, 0x1b,
	0x1b, 0x03, 0x04, 0xa0},
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0x6D, 0x31, 0xd9, 0x00, 0x50, 0x48, 0x63,
	0x30, 0x07, 0x03,//0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
#endif
#if  0 // back 9100
		 /* bit_clk418.50Mhz, 480*800, RGB888, 2 Lane 60 fps video mode */
    /* regulator */
		 {0x09, 0x08, 0x05, 0x00, 0x20},
		 /* timing */
		 {0x6d, 0x19, 0xf, 0x00, 0x2b, 0x35, 0x14, 0x1d,
		 0x1d, 0x03, 0x04, 0xa0},
    /* phy ctrl */
		 {0x5f, 0x00, 0x00, 0x10},
    /* strength */
		 {0xff, 0x00, 0x06, 0x00},
		 /* pll control */
		 {0x0, 0x1e, 0x30, 0xc1, 0x00, 0x50, 0x48, 0x63,
		 0x41, 0x0f, 0x01,
		 0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01 },
 #endif

};

static int __init mipi_video_novatek_qhd_pt_init(void)
{
	int ret;
	if (msm_fb_detect_client("mipi_video_novatek_qhd"))
		return 0;
	pinfo.xres =720;//480;// 540;
	pinfo.yres =1280;//800;// 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch =120;//60;// 80;
	pinfo.lcdc.h_front_porch =120;//60;// 24;
	pinfo.lcdc.h_pulse_width =8;//10;// 8;
	pinfo.lcdc.v_back_porch = 32;//11;//26;//16;
	pinfo.lcdc.v_front_porch =32;//8;//10;// 8;
	pinfo.lcdc.v_pulse_width =2;//3;//10;// 1;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max =28;// 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;//nt35590 must set true
	pinfo.mipi.bllp_power_stop = FALSE;//set TURE for esd test
	pinfo.mipi.traffic_mode =DSI_NON_BURST_SYNCH_EVENT;// DSI_NON_BURST_SYNCH_PULSE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;//DSI_RGB_SWAP_BGR;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.esc_byte_ratio = 4;
#if defined(NOVATEK_TWO_LANE)
	pinfo.mipi.data_lane1 = TRUE;
#endif
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
	pinfo.mipi.frame_rate = 60;//55;//60;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post =0x04;//0x20;// 0x04;
	pinfo.mipi.t_clk_pre = 0x24;//0x2f;//0x1c;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;

	ret = mipi_novatek_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_QHD_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_novatek_qhd_pt_init);
