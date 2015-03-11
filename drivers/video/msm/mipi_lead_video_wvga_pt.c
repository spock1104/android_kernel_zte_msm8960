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
/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
// from ics 1540 
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
};

extern u32 LcdPanleID ;

static int __init mipi_video_novatek_wvga_pt_init(void)
{
	int ret;

	if (msm_fb_detect_client("mipi_video_novatek_wvga"))
		return 0;

	pinfo.xres =480;// 540;
	pinfo.yres =800;// 960;
	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch =100;        //50;// 80;
	pinfo.lcdc.h_front_porch =100;//60  //50;// 80;
	pinfo.lcdc.h_pulse_width =10;          //8; 
	pinfo.lcdc.v_back_porch =26;        //11; 
	pinfo.lcdc.v_front_porch =10;          //5;
	pinfo.lcdc.v_pulse_width =10;         //8;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0x0;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max =27;// 15;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	//pinfo.clk_rate =482000000;//526684032;//pan
	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = TRUE;
	pinfo.mipi.hfp_power_stop = TRUE;
	pinfo.mipi.hbp_power_stop = TRUE;
	pinfo.mipi.hsa_power_stop = TRUE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;//nt35590 must set true
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode =DSI_NON_BURST_SYNCH_PULSE;// DSI_NON_BURST_SYNCH_PULSE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;//DSI_RGB_SWAP_BGR;
	pinfo.mipi.esc_byte_ratio = 4;	
	pinfo.mipi.data_lane0 = TRUE;	
	pinfo.mipi.data_lane1 = TRUE;	
	pinfo.mipi.data_lane2 = FALSE;
	pinfo.mipi.data_lane3 = FALSE;
	pinfo.mipi.tx_eot_append = TRUE;
	pinfo.mipi.t_clk_post =0x03;// 0x04;
	pinfo.mipi.t_clk_pre = 0x24;//0x1c;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	
	if (LcdPanleID ==LCD_PANEL_4P0_R61408_TRULY_LG)//this panel is different from others
	{
		pinfo.lcdc.h_back_porch  = 70;
		pinfo.lcdc.h_front_porch  = 250;	//150 on ics
		pinfo.lcdc.v_back_porch = 10;	
		pinfo.lcdc.v_front_porch = 12;
		pinfo.lcdc.v_pulse_width = 2;
		//pinfo.clk_rate = 444960000;

	}
	ret = mipi_novatek_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_WVGA_PT);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	return ret;
}

module_init(mipi_video_novatek_wvga_pt_init);
