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

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#define SENSOR_NAME "ov7692"
#define PLATFORM_DRIVER_NAME "msm_camera_ov7692"
#define ov7692_obj ov7692_##obj
static int IsPowered=0;
extern void msm_sensorinfo_set_front_sensor_id(uint16_t id);

DEFINE_MUTEX(ov7692_mut);
static struct msm_sensor_ctrl_t ov7692_s_ctrl;


static struct msm_camera_i2c_reg_conf ov7692_prev_and_snapshot_settings[] = {

   
    {0x12, 0x80},
    {0x0e, 0x08},
    {0x69, 0x52},
    {0x1e, 0xb3},
    {0x48, 0x42},
    {0xff, 0x01},
    {0xae, 0xa0},
    {0xa8, 0x26},
    {0xb4, 0xc0},
    {0xb5, 0x40},
    {0xff, 0x00},
    {0x0c, 0xd0}, //0XD0  by CDZ//0x00    [6] mirror enable  [7] filp enable //0x80 ZTE_20120117_YGL
    {0x62, 0x10},
    {0x12, 0x00},
    {0x17, 0x65},
    {0x18, 0xa4},
    {0x19, 0x0a},
    {0x1a, 0xf6},
    {0x3e, 0x30},
    {0x64, 0x0a},
    {0xff, 0x01},
    {0xb4, 0xc0},
    {0x86, 0x48},
    {0xff, 0x00},
    {0x67, 0x20},
    {0x81, 0x3f},
    {0xcc, 0x02},
    {0xcd, 0x80},
    {0xce, 0x01},
    {0xcf, 0xe0},
    {0xc8, 0x02},
    {0xc9, 0x80},
    {0xca, 0x01},
    {0xcb, 0xe0},
    {0xd0, 0x48},
    {0x82, 0x03},
  //  {0x0e, 0x00},
    {0x70, 0x00},
    {0x71, 0x34},
    {0x74, 0x28},
    {0x75, 0x98},
    {0x76, 0x00},
    {0x77, 0x64},
    {0x78, 0x01},
    {0x79, 0xc2},
    {0x7a, 0x4e},
    {0x7b, 0x1f},
    {0x7c, 0x00},
    {0x11, 0x00},
    {0x20, 0x00},
    {0x21, 0x23},
    {0x50, 0x9a},
    {0x51, 0x80},
    {0x4c, 0x7d},
    {0x0e, 0x00},
    {0x80, 0x7f},
    {0x85, 0x10},
    {0x86, 0x00},
    {0x87, 0x00},
    {0x88, 0x00},
    {0x89, 0x2a},
    {0x8a, 0x26},
    {0x8b, 0x22},
    {0xbb, 0x7a},
    {0xbc, 0x69},
    {0xbd, 0x11},
    {0xbe, 0x13},
    {0xbf, 0x81},
    {0xc0, 0x96},
    {0xc1, 0x1e},
    {0xb7, 0x05},
    {0xb8, 0x09},
    {0xb9, 0x00},
    {0xba, 0x18},
    {0x5a, 0x1f},
    {0x5b, 0x9f},
    {0x5c, 0x6a},
    {0x5d, 0x42},
    {0xa3, 0x0b},
    {0xa4, 0x15},
    {0xa5, 0x2a},
    {0xa6, 0x51},
    {0xa7, 0x63},
    {0xa8, 0x74},
    {0xa9, 0x83},
    {0xaa, 0x91},
    {0xab, 0x9e},
    {0xac, 0xaa},
    {0xad, 0xbe},
    {0xae, 0xce},
    {0xaf, 0xe5},
    {0xb0, 0xf3},
    {0xb1, 0xfb},
    {0xb2, 0x06},
    {0x8c, 0x5c},
    {0x8d, 0x11},
    {0x8e, 0x12},
    {0x8f, 0x19},
    {0x90, 0x50},
    {0x91, 0x20},
    {0x92, 0x96},
    {0x93, 0x80},
    {0x94, 0x13},
    {0x95, 0x1b},
    {0x96, 0xff},
    {0x97, 0x00},
    {0x98, 0x3d},
    {0x99, 0x36},
    {0x9a, 0x51},
    {0x9b, 0x43},
    {0x9c, 0xf0},
    {0x9d, 0xf0},
    {0x9e, 0xf0},
    {0x9f, 0xff},
    {0xa0, 0x68},
    {0xa1, 0x62},
    {0xa2, 0x0e},
    //===Lens Correction==;;  
    {0x80, 0x7F},  //control        
    {0x85, 0x10},  //control        
    {0x86, 0x10},  //radius         
    {0x87, 0x10},  //X              
    {0x88, 0x80},  //Y              
    {0x89, 0x2a},  //R              
    {0x8a, 0x25},  //G              
    {0x8b, 0x25},  //B              
    //;;====Color Matrix====;;  
    {0xbb, 0xac}, //D7              
    {0xbc, 0xae}, //DA              
    {0xbd, 0x02}, //03              
    {0xbe, 0x1f}, //27              
    {0xbf, 0x93}, //B8              
    {0xc0, 0xb1}, //DE              
    {0xc1, 0x1A},                  
    //===Edge + Denoise====;; 
    {0xb4, 0x06},                  
    {0xb5, 0x05}, //auto, no meaning
    {0xb6, 0x00}, //auto, no meaning
    {0xb7, 0x00},                  
    {0xb8, 0x06},                  
    {0xb9, 0x02},                  
    {0xba, 0x78},                  
     //====AEC/AGC target====;;
	/*change AE adjust speed,ZTE_CAM_YGL_20111215*/
    {0x00, 0x40},     //manual gain               
    {0x10, 0x80},     //manual exposure
    {0x13, 0xf7},
    {0x24, 0x88},  //0x94,ZTE_YGL_20120105,keep the same as default exposure setting                
    {0x25, 0x78},  //0x80                
    {0x26, 0xB5},                  
    //=====UV adjust======;;  
    {0x81, 0xff},                  
    {0x5A, 0x10},                  
    {0x5B, 0xA1},                  
    {0x5C, 0x3A},                  
    {0x5d, 0x20},                  
    //====Gamma====;;         
    {0xa3, 0x10},                  
    {0xa4, 0x1c},                  
    {0xa5, 0x30},                  
    {0xa6, 0x58},                  
    {0xa7, 0x68},                  
    {0xa8, 0x76},                  
    {0xa9, 0x81},                  
    {0xaa, 0x8a},                  
    {0xab, 0x92},                  
    {0xac, 0x98},                  
    {0xad, 0xa4},                  
    {0xae, 0xb1},                  
    {0xaf, 0xc5},                  
    {0xb0, 0xd7},                  
    {0xb1, 0xe8},                  
    {0xb2, 0x20},   
  
    //==Advance==;;           
    {0x8c, 0x52},                  
    {0x8d, 0x11},                  
    {0x8e, 0x12},                  
    {0x8f, 0x19},                  
    {0x90, 0x50},                  
    {0x91, 0x20},                  
    {0x92, 0xb1},                  
    {0x93, 0x9a},                  
    {0x94, 0x0c},                   
    {0x95, 0x0c},                   
    {0x96, 0xf0},                  
    {0x97, 0x10},                  
    {0x98, 0x61},                  
    {0x99, 0x63},                  
    {0x9a, 0x71},                  
    {0x9b, 0x78},                  
    {0x9c, 0xf0},                  
    {0x9d, 0xf0},                  
    {0x9e, 0xf0},                  
    {0x9f, 0xff},                  
    {0xa0, 0xa7},                  
    {0xa1, 0xb0},                  
    {0xa2, 0x0f}, 
	/*change the fps to 15,ZTE_CAM_YGL_20111215*/
    {0x31, 0x87}, //0x87 for 15fps ; 0x83 for 30fps   
    {0xd2, 0x06},  //enable saturation       //02	         
    {0xd8, 0x38}, //saturation                 
    {0xd9, 0x38},   
    
    {0x0e, 0x08},           
};

static struct msm_camera_i2c_conf_array ov7692_confs[] = {
	{&ov7692_prev_and_snapshot_settings[0],
	ARRAY_SIZE(ov7692_prev_and_snapshot_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct v4l2_subdev_info ov7692_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_YUYV8_2X8,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};


static struct msm_camera_i2c_reg_conf ov7692_brightness[][2] = {
	{
	//Brightness -3  
	{0xd3,0x40},
	{0xdc,0x09},
},
	{
	//Brightness -2
	{0xd3,0x20},
	{0xdc,0x09}, 
},
	{
	//Brightness -1
	 {0xd3,0x00},
	 {0xdc,0x09}, 
},
	{
	//Brightness 0 (Default)
	{0xd3,0x10},
	{0xdc,0x01},
},
	{
	//Brightness +1 
	{0xd3,0x20},
	{0xdc,0x01},           
},
{
	 //Brightness +2          
	{0xd3,0x30},
	{0xdc,0x01},         
},
	{
	 //Brightness +3
	{0xd3,0x40},
	{0xdc,0x01},          
},	
};


static struct msm_camera_i2c_conf_array ov7692_brightness_confs[][1] = {
	{{ov7692_brightness[0],
		ARRAY_SIZE(ov7692_brightness[0]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[1],
		ARRAY_SIZE(ov7692_brightness[1]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[2],
		ARRAY_SIZE(ov7692_brightness[2]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[3],
		ARRAY_SIZE(ov7692_brightness[3]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[4],
		ARRAY_SIZE(ov7692_brightness[4]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[5],
		ARRAY_SIZE(ov7692_brightness[5]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	{{ov7692_brightness[6],
		ARRAY_SIZE(ov7692_brightness[6]), 0, MSM_CAMERA_I2C_BYTE_DATA},
	},
	
};

static int ov7692_brightness_enum_map[] = {
	MSM_V4L2_BRIGHTNESS_L0,
	MSM_V4L2_BRIGHTNESS_L1,
	MSM_V4L2_BRIGHTNESS_L2,
	MSM_V4L2_BRIGHTNESS_L3,
	MSM_V4L2_BRIGHTNESS_L4,
	MSM_V4L2_BRIGHTNESS_L5,
	MSM_V4L2_BRIGHTNESS_L6,
};


static struct msm_camera_i2c_enum_conf_array ov7692_brightness_enum_confs = {
	.conf = &ov7692_brightness_confs[0][0],
	.conf_enum = ov7692_brightness_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_brightness_enum_map),
	.num_index = ARRAY_SIZE(ov7692_brightness_confs),
	.num_conf = ARRAY_SIZE(ov7692_brightness_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

struct msm_sensor_v4l2_ctrl_info_t ov7692_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_BRIGHTNESS,
		.min = MSM_V4L2_BRIGHTNESS_L0,
		.max = MSM_V4L2_BRIGHTNESS_L6,
		.step = 1,
		.enum_cfg_settings = &ov7692_brightness_enum_confs,
		.s_v4l2_ctrl = msm_sensor_s_ctrl_by_enum,
	},	
};


static struct msm_sensor_output_info_t ov7692_dimensions[] = {
	{
		.x_output = 0x280,
		.y_output = 0x01E0,
		.line_length_pclk = 0x280,
		.frame_length_lines = 0x01E0,
		.vt_pixel_clk = 12000000, 
		.op_pixel_clk = 16000000, 
		.binning_factor = 1,
	},	
};

static struct msm_sensor_output_reg_addr_t ov7692_reg_addr = {
	.x_output = 0xcc,
	.y_output = 0xce,
	.line_length_pclk = 0xcc,
	.frame_length_lines = 0xce,
};

static struct msm_sensor_id_info_t ov7692_id_info = {
	.sensor_id_reg_addr = 0x0A,
	.sensor_id = 0x7692,
};

static const struct i2c_device_id ov7692_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov7692_s_ctrl},
	{ }
};
int32_t msm_ov7692_sensor_mode_init(struct msm_sensor_ctrl_t *s_ctrl,
			int mode, struct sensor_init_cfg *init_info)
{
	s_ctrl->fps_divider = Q10;
	s_ctrl->cam_mode = MSM_SENSOR_MODE_INVALID;
	if (mode != s_ctrl->cam_mode) {
		s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
		s_ctrl->cam_mode = mode;
	}
	return 0;
}
static struct msm_cam_clk_info cam_8960_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

static void ov7692_start_stream(struct msm_sensor_ctrl_t *s_ctrl) {
	int rc=0;
	pr_err("cdz:%s  E\n",__func__);
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0e, 0x00, 
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
	pr_err("%s: i2c write failed\n", __func__);
	}
	msleep(10);
};

static void ov7692_stop_stream(struct msm_sensor_ctrl_t *s_ctrl) {	
	pr_err("cdz:%s  E\n",__func__);	
	msleep(10);
};

int32_t msm_ov7692_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct device *dev = NULL;
	pr_err("%s: E\n", __func__);
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE)
		dev = &s_ctrl->pdev->dev;
	else
		dev = &s_ctrl->sensor_i2c_client->client->dev;
	
       if(!IsPowered) {
	   	pr_err("%s: no IsPowered\n", __func__);
	   	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}
		rc = msm_camera_config_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
		if (rc < 0) {
			pr_err("%s: regulator on failed\n", __func__);
			goto config_vreg_failed;
		}

		rc = msm_camera_enable_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 1);
		if (rc < 0) {
			pr_err("%s: enable regulator failed\n", __func__);
			goto enable_vreg_failed;
		}
				
		if (s_ctrl->sensor_device_type == MSM_SENSOR_I2C_DEVICE) {
		
		if (s_ctrl->clk_rate != 0)
			cam_8960_clk_info->clk_rate = s_ctrl->clk_rate;

		rc = msm_cam_clk_enable(dev, cam_8960_clk_info,
			s_ctrl->cam_clk, ARRAY_SIZE(cam_8960_clk_info), 1);
		if (rc < 0) {
			pr_err("%s: clk enable failed\n", __func__);
			goto enable_clk_failed;
		}
	}
	mdelay(1);
	
	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: request gpio failed\n", __func__);
		goto request_gpio_failed;
	}
	rc = msm_camera_config_gpio_table(data, 1);
	if (rc < 0) {
		pr_err("%s: config gpio failed\n", __func__);
		goto config_gpio_failed;
	}
	IsPowered = 1;
       }else{
		pr_err("%s: has IsPowered\n", __func__);
		if (s_ctrl->sensor_device_type == MSM_SENSOR_I2C_DEVICE) {
		
		if (s_ctrl->clk_rate != 0)
			cam_8960_clk_info->clk_rate = s_ctrl->clk_rate;

		rc = msm_cam_clk_enable(dev, cam_8960_clk_info,
			s_ctrl->cam_clk, ARRAY_SIZE(cam_8960_clk_info), 1);
		if (rc < 0) {
			pr_err("%s: clk enable failed\n", __func__);
			goto enable_clk_failed;
		}
	}

	mdelay(1);
	
	rc=gpio_direction_output(53, 0);
	if (rc < 0)  
		pr_err("%s pwd gpio53  direction 0   failed\n",__func__);
	usleep_range(1000, 1000);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,  0xFF, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,  0xb4, 0xC0, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,   0xb5, 0x40, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,   0xFF, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	   }
	   
	if (!s_ctrl->power_seq_delay)
		usleep_range(1000, 2000);
	else if (s_ctrl->power_seq_delay < 20)
		usleep_range((s_ctrl->power_seq_delay * 1000),
			((s_ctrl->power_seq_delay * 1000) + 1000));
	else
		msleep(s_ctrl->power_seq_delay);
	
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE) {
		rc = msm_sensor_cci_util(s_ctrl->sensor_i2c_client,
			MSM_CCI_INIT);
		if (rc < 0) {
			pr_err("%s cci_init failed\n", __func__);
			goto cci_init_failed;
		}
	}
	pr_err("%s: X\n", __func__);
	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	return rc;

cci_init_failed:
	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

config_gpio_failed:
	msm_camera_enable_vreg(dev,
			s_ctrl->sensordata->sensor_platform_info->cam_vreg,
			s_ctrl->sensordata->sensor_platform_info->num_vreg,
			s_ctrl->vreg_seq,
			s_ctrl->num_vreg_seq,
			s_ctrl->reg_ptr, 0);
request_gpio_failed:
	kfree(s_ctrl->reg_ptr);
enable_clk_failed:
		msm_camera_config_gpio_table(data, 0);
enable_vreg_failed:
	msm_camera_config_vreg(dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
	msm_camera_request_gpio_table(data, 0);

	return rc;
}

int32_t msm_ov7692_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct device *dev = NULL;
	pr_err("%s  E\n", __func__);
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE)
		dev = &s_ctrl->pdev->dev;
	else
		dev = &s_ctrl->sensor_i2c_client->client->dev;
	if (s_ctrl->sensor_device_type == MSM_SENSOR_PLATFORM_DEVICE) {
		msm_sensor_cci_util(s_ctrl->sensor_i2c_client,
			MSM_CCI_RELEASE);
	}

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(0);

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xFF, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,  0xb4, 0x40, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,  0xb5, 0x30, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,  0xFF, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
		pr_err("%s: i2c write failed\n", __func__);		
		return rc;
		}
	
	rc=gpio_direction_output(53, 1);
	if (rc < 0)  
		pr_err("%s pwd gpio53  direction 1   failed\n",__func__);
	usleep_range(1000, 1000);
	
	msm_cam_clk_enable(dev, cam_8960_clk_info, s_ctrl->cam_clk,
			ARRAY_SIZE(cam_8960_clk_info), 0);
	s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
	return 0;
}
static enum msm_camera_vreg_name_t ov7692_veg_seq[] = {
	CAM_VANA,
	CAM_VIO,
};

int32_t msm_ov7692_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	pr_err("%s E update_type=%d res=%d\n", __func__, update_type,res);
	if (update_type == MSM_SENSOR_REG_INIT) {	

	rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0e, 0x08, 
		MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
	pr_err("%s: i2c write failed\n", __func__);
	}
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		
		rc = msm_sensor_write_conf_array(
		s_ctrl->sensor_i2c_client,
		s_ctrl->msm_sensor_reg->mode_settings, res);
		
		msleep(30);
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
		NOTIFY_PCLK_CHANGE, &s_ctrl->msm_sensor_reg->
		output_settings[res].op_pixel_clk);
	
	}
	return rc;
}
int32_t msm_ov7692_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	pr_err("%s %s_i2c_probe called\n", __func__, client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s %s i2c_check_functionality failed\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}
	
	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_device_type = MSM_SENSOR_I2C_DEVICE;
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		pr_err("%s %s sensor_i2c_client NULL\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}
	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		pr_err("%s %s NULL sensor data\n", __func__, client->name);
		return -EFAULT;
	}
	
	#if defined (CONFIG_MACH_ELDEN)
	rc=gpio_request(54,"ov5640");
	if (rc < 0)  
		pr_err("%s  gpio  requst 54 failed\n",__func__);
	rc=gpio_direction_output(54, 1);
	if (rc < 0)  
		pr_err("%s  gpio  direction 54 failed\n",__func__);
	#endif

	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s %s power up failed\n", __func__, client->name);
		return rc;
	}
	
	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0)
		goto probe_fail;

#ifdef CONFIG_SENSOR_INFO 
    	msm_sensorinfo_set_front_sensor_id(s_ctrl->sensor_id_info->sensor_id);
#else
  //do nothing here
#endif

	if (!s_ctrl->wait_num_frames)
		s_ctrl->wait_num_frames = 1 * Q10;

	pr_err("%s %s probe succeeded\n", __func__, client->name);
	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);
	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		s_ctrl->sensor_v4l2_subdev_ops);
	s_ctrl->sensor_v4l2_subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&s_ctrl->sensor_v4l2_subdev.entity, 0, NULL, 0);
	s_ctrl->sensor_v4l2_subdev.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->sensor_v4l2_subdev.entity.group_id = SENSOR_DEV;
	s_ctrl->sensor_v4l2_subdev.entity.name =
		s_ctrl->sensor_v4l2_subdev.name;
	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);
	s_ctrl->sensor_v4l2_subdev.entity.revision =
		s_ctrl->sensor_v4l2_subdev.devnode->num;

	msm_sensor_enable_debugfs(s_ctrl);
	goto power_down;

probe_fail:
	pr_err("%s %s_i2c_probe failed\n", __func__, client->name);
power_down:
	if (rc > 0)
		rc = 0;
	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
	
	return rc;
}

static struct i2c_driver ov7692_i2c_driver = {
	.id_table = ov7692_i2c_id,
	.probe  = msm_ov7692_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov7692_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	rc = i2c_add_driver(&ov7692_i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops ov7692_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov7692_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov7692_subdev_ops = {
	.core = &ov7692_subdev_core_ops,
	.video  = &ov7692_subdev_video_ops,
};

static struct msm_sensor_fn_t ov7692_func_tbl = {
	.sensor_start_stream = ov7692_start_stream,
	.sensor_stop_stream = ov7692_stop_stream,
	.sensor_setting = msm_ov7692_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_ov7692_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_ov7692_sensor_power_up,
	.sensor_power_down = msm_ov7692_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t ov7692_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.mode_settings = &ov7692_confs[0],
	.output_settings = &ov7692_dimensions[0],
	.num_conf = ARRAY_SIZE(ov7692_confs),
};

static struct msm_sensor_ctrl_t ov7692_s_ctrl = {
	.msm_sensor_reg = &ov7692_regs,
	.msm_sensor_v4l2_ctrl_info = ov7692_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(ov7692_v4l2_ctrl_info),
	.sensor_i2c_client = &ov7692_sensor_i2c_client,
	.sensor_i2c_addr = 0x78,
	.vreg_seq = ov7692_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(ov7692_veg_seq),
	.sensor_output_reg_addr = &ov7692_reg_addr,
	.sensor_id_info = &ov7692_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.min_delay = 30,
	.power_seq_delay = 0,	
	.msm_sensor_mutex = &ov7692_mut,
	.sensor_i2c_driver = &ov7692_i2c_driver,
	.sensor_v4l2_subdev_info = ov7692_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov7692_subdev_info),
	.sensor_v4l2_subdev_ops = &ov7692_subdev_ops,
	.func_tbl = &ov7692_func_tbl,
	.msm_sensor_reg_default_data_type=MSM_CAMERA_I2C_BYTE_DATA,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
