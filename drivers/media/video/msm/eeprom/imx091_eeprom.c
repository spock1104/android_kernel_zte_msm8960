/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include <linux/module.h>
#include "msm_camera_eeprom.h"
#include "msm_camera_i2c.h"

DEFINE_MUTEX(imx091_eeprom_mutex);
static struct msm_eeprom_ctrl_t imx091_eeprom_t;

static const struct i2c_device_id imx091_eeprom_i2c_id[] = {
	{"imx091_eeprom", (kernel_ulong_t)&imx091_eeprom_t},
	{ }
};

static struct i2c_driver imx091_eeprom_i2c_driver = {
	.id_table = imx091_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __exit_p(imx091_eeprom_i2c_remove),
	.driver = {
		.name = "imx091_eeprom",
	},
};

static int __init imx091_eeprom_i2c_add_driver(void)
{
	int rc = 0;
	rc = i2c_add_driver(imx091_eeprom_t.i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops imx091_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops imx091_eeprom_subdev_ops = {
	.core = &imx091_eeprom_subdev_core_ops,
};

static uint8_t imx091_wbcalib_data[6];
static uint8_t imx091_wbcalib_gold_data[6];//golden sample value
static uint8_t imx091_afcalib_data[6];
static uint8_t imx091_lscalib_blk1_data[51*5];
static uint8_t imx091_lscalib_blk2_data[51*5];
static uint8_t imx091_lscalib_blk3_data[51*5];
static uint8_t imx091_lscalib_blk4_data[51*5];
static uint8_t imx091_lscalib_blk5_data[17*5];

static uint8_t  * ptr[5] = { imx091_lscalib_blk1_data,imx091_lscalib_blk2_data,imx091_lscalib_blk3_data,
	                                     imx091_lscalib_blk4_data,imx091_lscalib_blk5_data};

static struct msm_calib_wb imx091_wb_data;
static struct msm_calib_wb imx091_wb_gold_data;//golden sample value
static struct msm_calib_af imx091_af_data;
static struct msm_calib_lsc imx091_lsc_data;//221*4 word

//modified by zhang.yu_1 for support eeprom to imx091 sensor
static uint8_t imx091_module_info_data[6];

 static struct imx091_module_info_t{
	uint16_t module_vendor;
	uint16_t lens_id;
	uint16_t lsc_version;
}imx091_module_info;


static struct msm_camera_eeprom_info_t imx091_calib_supp_info = {
	{TRUE, 6, 1, 1},//AF
	{TRUE, 6, 0, 1023},//AWB
	{TRUE, 221*4*2, 2, 4},//LSC
	{FALSE, 0, 0, 1},
	{FALSE, 0, 0, 1},
};

/****************************************************************
                                        eeprom partition
*****************************************************************
blk        i2c addr (8bit)                              info
              (1010B2B1B0 r/w)          
-----------------------------------------------------------------
mcnex:
blk0          0xA0                                module id/lens id/lsc ver/awb/af/lsc size
blk1          0xA2                                ROI1  -        51
blk2          0xA4                                ROI52  -     102
blk3          0xA6                                ROI103  -   153
blk4          0xA8                                ROI1154  - 204
blk5          0xAA                                ROI1205 -  221
blk6          0xAC                                not used
blk7          0xAE                                not used

sunny:
blk0          0xA0                                module id/lens id/lsc ver/awb/af/lsc size
blk1          0xA2     			          not used
blk2          0xA4                                ROI1  -        51
blk3          0xA6                                ROI52  -     102
blk4          0xA8                                ROI103  -   153
blk5          0xAA                                ROI1154  - 204
blk6          0xAC                                ROI1205 -  221
blk7          0xAE                                not used

****************************************************************/
static uint8_t eeprom_blk_i2c_addr[] = {
         //0xA0,  blk0 i2c  addr must be equal  with i2c_addr below
	0xA2, //blk1 addr
	0xA4, //blk2 addr
	0xA6, //blk3 addr
	0xA8, //blk4 addr
	0xAA, //blk5 addr
};

//modified by zhang.yu_1 for support eeprom to imx091 sensor	
static struct msm_camera_eeprom_read_t imx091_eeprom_read_tbl[] = {
	{0x00, &imx091_module_info_data[0], 6, 0},
	{0x06, &imx091_wbcalib_data[0], 6, 0},
	{0x0c, &imx091_wbcalib_gold_data[0], 6, 0},	
	{0x12, &imx091_afcalib_data[0], 6, 0}, 
};

//modified by zhang.yu_1 for support eeprom to imx091 sensor	
static struct msm_camera_eeprom_read_t imx091_eeprom_blk_tbl[] = {
	{0x00, &imx091_lscalib_blk1_data[0], 51*5, 0},  //i2c addr is 0xA2
	{0x00, &imx091_lscalib_blk2_data[0], 51*5, 0},  //i2c addr is 0xA4
	{0x00, &imx091_lscalib_blk3_data[0], 51*5, 0},  //i2c addr is 0xA6
	{0x00, &imx091_lscalib_blk4_data[0], 51*5, 0}, //i2c addr is 0xA8
	{0x00, &imx091_lscalib_blk5_data[0], 17*5, 0}, //i2c addr is 0xAA
};


static struct msm_camera_eeprom_data_t imx091_eeprom_data_tbl[] = {
	{&imx091_wb_data, sizeof(struct msm_calib_wb)},
	{&imx091_af_data, sizeof(struct msm_calib_af)},
	{&imx091_lsc_data, sizeof(struct msm_calib_lsc)},	
};

//modified by zhang.yu_1 for support eeprom to imx091 sensor	
static struct msm_camera_eeprom_data_t imx091_eeprom_data_mod_info = {
	&imx091_module_info, 
	sizeof(struct imx091_module_info_t),
};
//added by zhang.yu_1 for display module vendor
uint16_t   imx091_get_module_vendor(void)
{
	return imx091_module_info.module_vendor;
}

static void imx091_get_module_info(void)
{
	imx091_module_info.module_vendor = imx091_module_info_data[0] | (uint16_t)imx091_module_info_data[1]<<8;
	imx091_module_info.lens_id = imx091_module_info_data[2] | (uint16_t)imx091_module_info_data[3]<<8;
	imx091_module_info.lsc_version = imx091_module_info_data[4] | (uint16_t)imx091_module_info_data[5]<<8;

	printk("vendor id: 0x%x, lens id: 0x%x, lsc ver: 0x%x \n",imx091_module_info.module_vendor, imx091_module_info.lens_id, imx091_module_info.lsc_version);		 
}

//modified by zhang.yu_1 for support eeprom to imx091 sensor	
static void imx091_format_wbdata(void)
{
	imx091_wb_data.r_over_g = (uint16_t)(imx091_wbcalib_data[1] << 8) |	imx091_wbcalib_data[0];		
	imx091_wb_data.b_over_g = (uint16_t)(imx091_wbcalib_data[3] << 8) |	imx091_wbcalib_data[2];	
	imx091_wb_data.gr_over_gb =  (uint16_t)(imx091_wbcalib_data[5] << 8) |imx091_wbcalib_data[4];	
	
	printk("r_over_g: 0x%x, b_over_g: 0x%x, gr_over_gb: 0x%x\n ",imx091_wb_data.r_over_g , imx091_wb_data.b_over_g, imx091_wb_data.gr_over_gb);		 	
	
	//for golden sample
	imx091_wb_gold_data.r_over_g = (uint16_t)(imx091_wbcalib_gold_data[1] << 8) | imx091_wbcalib_gold_data[0];		
	imx091_wb_gold_data.b_over_g = (uint16_t)(imx091_wbcalib_gold_data[3] << 8) | imx091_wbcalib_gold_data[2];	
	imx091_wb_gold_data.gr_over_gb =  (uint16_t)(imx091_wbcalib_gold_data[5] << 8) | imx091_wbcalib_gold_data[4];	
	
	printk("r_over_g_golden: 0x%x, b_over_g_golden: 0x%x, gr_over_gb_golden: 0x%x\n ",imx091_wb_gold_data.r_over_g , imx091_wb_gold_data.b_over_g, imx091_wb_gold_data.gr_over_gb);		 		
}

static void imx091_format_afdata(void)
{
	imx091_af_data.inf_dac = (uint16_t)(imx091_afcalib_data[3] << 8) |imx091_afcalib_data[2];
	imx091_af_data.macro_dac = (uint16_t)(imx091_afcalib_data[1] << 8) |	imx091_afcalib_data[0];		 

	printk("inf_dac: 0x%x, macro_dac: 0x%x\n ",imx091_af_data.inf_dac , imx091_af_data.macro_dac);		 		
}

static void imx091_format_lscdata(void)
{
	uint8_t i = 0, j =0;

	for(i=0; i<17*13; i++){
		imx091_lsc_data.r_gain[i] = *(ptr[i/51]+5*j) |( (uint16_t)(*(ptr[i/51]+5*j+4)  & 0xc0)) << 2;
		imx091_lsc_data.gr_gain[i] = *(ptr[i/51]+5*j+1) | ((uint16_t)(*(ptr[i/51]+5*j+4) & 0x30)) << 4;
		imx091_lsc_data.gb_gain[i] = *(ptr[i/51]+5*j+2) | ((uint16_t)(*(ptr[i/51]+5*j+4) & 0x0c))<< 6;	
		imx091_lsc_data.b_gain[i] = *(ptr[i/51]+5*j+3) | ((uint16_t)(*(ptr[i/51]+5*j+4) & 0x03)) << 8 ;	
		j += 1;
		if(j == 51)
			j = 0;
		
		printk("r_gain is : 0x%x, gr_gain is : 0x%x,  gb_gain is: 0x%x,  b_gain is: 0x%x\n",imx091_lsc_data.r_gain[i],imx091_lsc_data.gr_gain[i],imx091_lsc_data.gb_gain[i],imx091_lsc_data.b_gain[i]);			
	}
}

void imx091_format_calibrationdata(void)
{
	imx091_format_wbdata();
	imx091_format_afdata();
	//modified by zhang.yu_1 for support eeprom to imx091 sensor		
	imx091_get_module_info();
	//imx091_format_lscdata();
}

//modified by zhang.yu_1 for support eeprom to imx091 sensor	
void imx091_format_lens_calibrationdata(void)
{
	imx091_format_lscdata();
}

static struct msm_eeprom_ctrl_t imx091_eeprom_t = {
	.i2c_driver = &imx091_eeprom_i2c_driver,
	.i2c_addr = 0xA0,
	.eeprom_v4l2_subdev_ops = &imx091_eeprom_subdev_ops,

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	},

	.eeprom_mutex = &imx091_eeprom_mutex,

	.func_tbl = {
		.eeprom_init = NULL,
		.eeprom_release = NULL,
		.eeprom_get_info = msm_camera_eeprom_get_info,
		.eeprom_get_data = msm_camera_eeprom_get_data,
		.eeprom_set_dev_addr = NULL,
		.eeprom_format_data = imx091_format_calibrationdata,
		.eeprom_format_lsc_data = imx091_format_lens_calibrationdata,
	},
	.info = &imx091_calib_supp_info,
	.info_size = sizeof(struct msm_camera_eeprom_info_t),
	.read_tbl = imx091_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(imx091_eeprom_read_tbl),
	.data_tbl = imx091_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(imx091_eeprom_data_tbl),
	//modified by zhang.yu_1 for support eeprom to imx091 sensor	
	.eeprom_blk_tbl = imx091_eeprom_blk_tbl,
	.eeprom_blk_tbl_size = ARRAY_SIZE(imx091_eeprom_blk_tbl),
	.eeprom_blk_i2c_addr_tbl  = eeprom_blk_i2c_addr,
	.data_mod_info = &imx091_eeprom_data_mod_info,
};

subsys_initcall(imx091_eeprom_i2c_add_driver);
MODULE_DESCRIPTION("imx091 EEPROM");
MODULE_LICENSE("GPL v2");
