/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include <asm/mach-types.h>
#include <linux/gpio.h>
#include <mach/camera.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>
#include <mach/gpiomux.h>
#include "devices.h"
#include "board-8960.h"

#ifdef CONFIG_MSM_CAMERA

#if (defined(CONFIG_GPIO_SX150X) || defined(CONFIG_GPIO_SX150X_MODULE)) && \
	defined(CONFIG_I2C)

static struct i2c_board_info cam_expander_i2c_info[] = {
	{
		I2C_BOARD_INFO("sx1508q", 0x22),
		.platform_data = &msm8960_sx150x_data[SX150X_CAM]
	},
};

static struct msm_cam_expander_info cam_expander_info[] = {
	{
		cam_expander_i2c_info,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	},
};
#endif

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 1*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 2*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*active 3*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_5, /*active 4*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_6, /*active 5*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_2, /*active 6*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_3, /*active 7*/
		.drv = GPIOMUX_DRV_8MA,
		.pull = GPIOMUX_PULL_UP,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*i2c suspend*/
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
	//add by wxl for p896a25 
	{
	.func = GPIOMUX_FUNC_2,      
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	},
	//add by jzq for p896a11 
#if defined (CONFIG_MACH_ROUNDTOP)|| defined (CONFIG_MACH_NUNIVAK)|| defined(CONFIG_MACH_QUANTUM)    //jzq add gpio89 for cam1_avdd_en	
	{
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
	},
#endif	
};
//wt isp mbg 2013-01-21
#ifdef CONFIG_ISPCAM

static struct gpiomux_setting gsbi7_settings[] = {

	{
	.func = GPIOMUX_FUNC_2,      /*active 1*/
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	},

	{
	.func = GPIOMUX_FUNC_1,      /*active 1*/
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
	},
	{
	.func = GPIOMUX_FUNC_1, /*i2c suspend*/
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
	},
};


#endif

static struct msm_gpiomux_config msm8960_cdp_flash_configs[] = {
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[1],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
};

static struct msm_gpiomux_config msm8960_cam_common_configs[] = {
#if 0	/* Modified By jzq for Flash 12.11.06*/
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif	
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#ifdef CONFIG_ISPCAM
     {
			.gpio = 4,
			.settings = {
				[GPIOMUX_ACTIVE]	= &gsbi7_settings[0],
				[GPIOMUX_SUSPENDED] = &cam_settings[0],
			},
	},
#else
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[9],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif	
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
	{
		.gpio = 76,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if defined (CONFIG_MACH_ROUNDTOP)|| defined (CONFIG_MACH_NUNIVAK)|| defined(CONFIG_MACH_QUANTUM)    //jzq add gpio89 for cam1_avdd_en	
	{
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[10],
			[GPIOMUX_SUSPENDED] = &cam_settings[10],
		},
	},
#endif	
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#if 0	/* Modified By jzq for Flash 12.11.22*/
	{
		.gpio = 54,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	},
#endif
{
		.gpio = 75,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[2],
			[GPIOMUX_SUSPENDED] = &cam_settings[0],
		},
	}
};

static struct msm_gpiomux_config msm8960_cam_2d_configs[] = {
#if 0	/* Modified By jzq for Flash 12.11.06*/
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
#endif	
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
};

//wt for isp  fujitsu 2013-01-21
   #ifdef 	CONFIG_ISPCAM
static struct msm_gpiomux_config msm8960_ispcam_configs[] = {

	{
		.gpio	   = 32,	/* GSBI7 I2C */
		.settings = {
		    [GPIOMUX_ACTIVE] = &gsbi7_settings[1],
		    [GPIOMUX_SUSPENDED] = &gsbi7_settings[2],
			
		},
	},
	{
		.gpio	   = 33,	/* GSBI7 I2C */
		.settings = {
			[GPIOMUX_ACTIVE] = &gsbi7_settings[1],
			[GPIOMUX_SUSPENDED] = &gsbi7_settings[2],
			
		},
	},
};
    #endif
//wt for isp  fujitsu 2013-01-21
	

static struct msm_gpiomux_config msm8960_cam_2d_configs_sglte[] = {
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[8],
		},
	},
};

#define VFE_CAMIF_TIMER1_GPIO 2
#define VFE_CAMIF_TIMER2_GPIO 3
#define VFE_CAMIF_TIMER3_GPIO_INT 4
static struct msm_camera_sensor_strobe_flash_data strobe_flash_xenon = {
	.flash_trigger = VFE_CAMIF_TIMER2_GPIO,
	.flash_charge = VFE_CAMIF_TIMER1_GPIO,
	.flash_charge_done = VFE_CAMIF_TIMER3_GPIO_INT,
	.flash_recharge_duration = 50000,
	.irq = MSM_GPIO_TO_INT(VFE_CAMIF_TIMER3_GPIO_INT),
};

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = VFE_CAMIF_TIMER1_GPIO,
	._fsrc.ext_driver_src.led_flash_en = VFE_CAMIF_TIMER2_GPIO,
	._fsrc.ext_driver_src.flash_id = MAM_CAMERA_EXT_LED_FLASH_SC628A,
};
#endif

static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 27648000,
		.ib  = 110592000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274406400,
		.ib  = 617103360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 274423680,
		.ib  = 1097694720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 302071680,
		.ib  = 1208286720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};

static struct msm_bus_vectors cam_video_ls_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 348192000,
		.ib  = 617103360,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_dual_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 348192000,
		.ib  = 1208286720,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 206807040,
		.ib  = 488816640,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 540000000,
		.ib  = 1350000000,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_MM_IMEM,
		.ab  = 43200000,
		.ib  = 69120000,
	},
};



static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_video_ls_vectors),
		cam_video_ls_vectors,
	},
	{
		ARRAY_SIZE(cam_dual_vectors),
		cam_dual_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};

static struct msm_camera_device_platform_data msm_camera_csi_device_data[] = {
	{
		.csid_core = 0,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.csid_core = 1,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
	{
		.csid_core = 2,
		.is_vpe    = 1,
		.cam_bus_scale_table = &cam_bus_client_pdata,
	},
};

static struct camera_vreg_t msm_8960_cam_vreg[] = {
	{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
	{"cam_vio", REG_VS, 0, 0, 0},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vaf", REG_LDO, 2800000, 2800000, 300000},
};

static struct gpio msm8960_common_cam_gpio[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"},
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};

static struct gpio msm8960_front_cam_gpio[] = {
	{76, GPIOF_DIR_OUT, "CAM_RESET"},
#if 0  /* Modified By jzq for Flash 12.11.22*/
	{75, GPIOF_DIR_OUT, "CAM_PWDN"},
#endif
};

static struct gpio msm8960_back_cam_gpio[] = {
#if 0  /* Modified By jzq for Flash 12.11.22*/
        {54, GPIOF_DIR_OUT, "CAM_STANDBY"},
#endif
	{107, GPIOF_DIR_OUT, "CAM_RESET"},
};

#if defined (CONFIG_MACH_ROUNDTOP)|| defined (CONFIG_MACH_NUNIVAK)	
static struct msm_gpio_set_tbl msm8960_front_cam_gpio_set_tbl[] = {
	{76, GPIOF_OUT_INIT_HIGH, 1000},
	{76, GPIOF_OUT_INIT_LOW, 5000},
	{76, GPIOF_OUT_INIT_HIGH, 4000},
};
#else
static struct msm_gpio_set_tbl msm8960_front_cam_gpio_set_tbl[] = {
        {75, GPIOF_OUT_INIT_LOW, 1000},
	{76, GPIOF_OUT_INIT_LOW, 1000},
	{76, GPIOF_OUT_INIT_HIGH, 4000},
};
#endif

static struct msm_gpio_set_tbl msm8960_back_cam_gpio_set_tbl[] = {
        {54, GPIOF_OUT_INIT_HIGH, 1000},
	{107, GPIOF_OUT_INIT_LOW, 1000},
	{107, GPIOF_OUT_INIT_HIGH, 4000},
};

static struct msm_camera_gpio_conf msm_8960_front_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_front_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_front_cam_gpio),
	.cam_gpio_set_tbl = msm8960_front_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_front_cam_gpio_set_tbl),
};

static struct msm_camera_gpio_conf msm_8960_back_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_back_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_back_cam_gpio),
	.cam_gpio_set_tbl = msm8960_back_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_back_cam_gpio_set_tbl),
};

static struct i2c_board_info msm_act_main_cam_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x11),
};

static struct msm_actuator_info msm_act_main_cam_0_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_0,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};
#if 0
static struct i2c_board_info msm_act_main_cam1_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x18),
};

static struct msm_actuator_info msm_act_main_cam_1_info = {
	.board_info     = &msm_act_main_cam1_i2c_info,
	.cam_name       = MSM_ACTUATOR_MAIN_CAM_1,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};
#endif

static struct msm_camera_sensor_flash_data flash_imx074 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_src	= &msm_flash_src
#endif
};

static struct msm_camera_csi_lane_params imx074_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct msm_camera_sensor_platform_info sensor_board_info_imx074 = {
	.mount_angle	= 90,
	.cam_vreg = msm_8960_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_cam_vreg),
	.gpio_conf = &msm_8960_back_cam_gpio_conf,
	.csi_lane_params = &imx074_csi_lane_params,
};


static struct msm_camera_sensor_info msm_camera_sensor_imx074_data = {
	.sensor_name	= "imx074",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx074,
	.strobe_flash_data = &strobe_flash_xenon,
	.sensor_platform_info = &sensor_board_info_imx074,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_0_info,
};

static struct msm_camera_sensor_flash_data flash_mt9m114 = {
	.flash_type = MSM_CAMERA_FLASH_NONE
};

static struct msm_camera_csi_lane_params mt9m114_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

#if defined (CONFIG_MACH_ROUNDTOP)|| defined (CONFIG_MACH_NUNIVAK)	|| defined (CONFIG_MACH_OKMOK)	
static struct camera_vreg_t mt9m114_cam_vreg[] = {
	//{"cam_vio", REG_VS, 0, 0, 0, 50},
       {"cam_vdig", REG_VS, 1800000, 1800000, 0, 50},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600, 50},
};
#else
static struct camera_vreg_t mt9m114_cam_vreg[] = {
	{"cam_vio", REG_VS, 0, 0, 0, 50},
       {"cam_vdig", REG_LDO, 1200000, 1200000, 105000, 50},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600, 50},
};
#endif

static struct msm_camera_sensor_platform_info sensor_board_info_mt9m114 = {
       .mount_angle = 270,  
	.cam_vreg = mt9m114_cam_vreg,
	.num_vreg = ARRAY_SIZE(mt9m114_cam_vreg),
	.gpio_conf = &msm_8960_front_cam_gpio_conf,
	.csi_lane_params = &mt9m114_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9m114_data = {
	.sensor_name = "mt9m114",
	.pdata = &msm_camera_csi_device_data[1],
	.flash_data = &flash_mt9m114,
	.sensor_platform_info = &sensor_board_info_mt9m114,
	.csi_if = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

static struct msm_camera_sensor_flash_data flash_ov2720 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params ov2720_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov2720 = {
	.mount_angle	= 0,
	.cam_vreg = msm_8960_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_cam_vreg),
	.gpio_conf = &msm_8960_front_cam_gpio_conf,
	.csi_lane_params = &ov2720_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov2720_data = {
	.sensor_name	= "ov2720",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov2720,
	.sensor_platform_info = &sensor_board_info_ov2720,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};

#ifdef CONFIG_OV9726
static struct msm_camera_sensor_flash_data flash_ov9726 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params ov9726_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov9726 = {
	.mount_angle	= 0,
	.cam_vreg = msm_8960_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_cam_vreg),
	.gpio_conf = &msm_8960_front_cam_gpio_conf,
	.csi_lane_params = &ov9726_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data = {
	.sensor_name	= "ov9726",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov9726,
	.sensor_platform_info = &sensor_board_info_ov9726,
	.csi_if	= 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
#endif

#ifdef CONFIG_ISPCAM
static struct gpio msm8960_common_ispcam_gpio[] = {
   {4, GPIOF_DIR_IN, "CAM_MCLK"},
	//{32, GPIOF_DIR_IN, "CAM_I2C_DATA"},
	//{33, GPIOF_DIR_IN, "CAM_I2C_CLK"},
};

static struct gpio msm8960_isp_cam_gpio[] = {
	//{107, GPIOF_DIR_OUT, "CAM_RESET"},
};
static struct msm_gpio_set_tbl msm8960_isp_cam_gpio_set_tbl[] = {
       // {107, GPIOF_OUT_INIT_LOW, 1000},
       //{107, GPIOF_OUT_INIT_HIGH, 10000},
};
static struct msm_camera_gpio_conf msm_8960_isp_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_ispcam_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_ispcam_configs),
	.cam_gpio_common_tbl = msm8960_common_ispcam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_common_ispcam_gpio),
	.cam_gpio_req_tbl = msm8960_isp_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_isp_cam_gpio),
	.cam_gpio_set_tbl = msm8960_isp_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_isp_cam_gpio_set_tbl),
};
static struct camera_vreg_t msm_8960_isp_vreg[] = {
	//{"mipi_csi_vdd", REG_LDO, 1200000, 1200000, 20000},
	{"cam_vdig", REG_LDO, 1800000, 1800000, 105000},
	//{"cam_vio", REG_VS, 0, 0, 0},
	//{"cam_vdig", REG_LDO, 1800000, 1800000, 105000},
	{"cam_vaf", REG_LDO, 2800000, 2800000, 300000},
};
static struct msm_camera_sensor_flash_data flash_isp = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
	
};

static struct msm_camera_csi_lane_params isp_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x03,
};

static struct msm_camera_sensor_platform_info sensor_board_info_isp = {
	.mount_angle	= 90,
	.cam_vreg = msm_8960_isp_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_isp_vreg),
	.gpio_conf = &msm_8960_isp_cam_gpio_conf,
	.csi_lane_params = &isp_csi_lane_params,
};

static struct msm_camera_sensor_info msm_camera_sensor_isp_data = {
	.sensor_name	= "ispcam",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_isp,
	.sensor_platform_info = &sensor_board_info_isp,
	.csi_if	= 1,//0:Parallel interface, 1:MIPI interface 
	.camera_type = BACK_CAMERA_2D,
};

#endif


static struct msm_camera_sensor_flash_data flash_s5k3l1yx = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params s5k3l1yx_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k3l1yx = {
	.mount_angle  = 0,
	.cam_vreg = msm_8960_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_cam_vreg),
	.gpio_conf = &msm_8960_back_cam_gpio_conf,
	.csi_lane_params = &s5k3l1yx_csi_lane_params,
};

static struct msm_actuator_info msm_act_main_cam_2_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_2,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3l1yx_data = {
	.sensor_name          = "s5k3l1yx",
	.pdata                = &msm_camera_csi_device_data[0],
	.flash_data           = &flash_s5k3l1yx,
	.sensor_platform_info = &sensor_board_info_s5k3l1yx,
	.csi_if               = 1,
	.camera_type          = BACK_CAMERA_2D,
	.sensor_type          = BAYER_SENSOR,
	.actuator_info    = &msm_act_main_cam_2_info,
};

static struct pm8xxx_mpp_config_data privacy_light_on_config = {
	.type		= PM8XXX_MPP_TYPE_SINK,
	.level		= PM8XXX_MPP_CS_OUT_5MA,
	.control	= PM8XXX_MPP_CS_CTRL_MPP_LOW_EN,
};

static struct pm8xxx_mpp_config_data privacy_light_off_config = {
	.type		= PM8XXX_MPP_TYPE_SINK,
	.level		= PM8XXX_MPP_CS_OUT_5MA,
	.control	= PM8XXX_MPP_CS_CTRL_DISABLE,
};

#ifdef CONFIG_OV9740

static struct msm_camera_csi_lane_params ov9740_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};
static struct gpio msm8960_ov9740_common_cam_gpio[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"}, 
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};
#if 0
static struct gpio msm8960_ov9740_cam_gpio[] = {
	{53, GPIOF_DIR_OUT, "CAM_PWD"},
      {76, GPIOF_DIR_OUT, "CAM_RESET"},
};
static struct msm_gpio_set_tbl msm8960_ov9740_cam_gpio_set_tbl[] = {
	{53, GPIOF_OUT_INIT_HIGH, 1000},
	{53, GPIOF_OUT_INIT_LOW, 1000},
      {76, GPIOF_OUT_INIT_HIGH, 5000},
      {76, GPIOF_OUT_INIT_LOW, 5000},
	{76, GPIOF_OUT_INIT_HIGH, 1000},
};
#endif
static struct msm_camera_gpio_conf msm_8960_ov9740_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_ov9740_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_ov9740_common_cam_gpio),
	//.cam_gpio_req_tbl = msm8960_ov9740_cam_gpio,
	//.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_ov9740_cam_gpio),
	//.cam_gpio_set_tbl = msm8960_ov9740_cam_gpio_set_tbl,
	//.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_ov9740_cam_gpio_set_tbl),
};
static struct camera_vreg_t msm_8960_ov9740_vreg[] = {
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
#if defined(CONFIG_MACH_PRINDLE) // //add by lijing for P896F10 20121114  
	{"cam_vdig", REG_VS, 1800000, 1800000, 0, 50},
#endif
};
static struct msm_camera_sensor_flash_data flash_ov9740 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,//type of flash
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov9740 = {
#if defined(CONFIG_MACH_PRINDLE) ||defined (CONFIG_MACH_MAZAMA) || defined(CONFIG_MACH_QUANTUM)||defined(CONFIG_MACH_HAYES)
	.mount_angle	= 270,  //modfiy by wxl for p896f10 and p896d01
#else
	.mount_angle	= 90,  
#endif
	.cam_vreg = msm_8960_ov9740_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_ov9740_vreg),
	.gpio_conf = &msm_8960_ov9740_cam_gpio_conf,
	.csi_lane_params = &ov9740_csi_lane_params
};

static struct msm_camera_sensor_info msm_camera_sensor_ov9740_data = {
	.sensor_name	= "ov9740",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov9740,
	.sensor_platform_info = &sensor_board_info_ov9740,
	.csi_if	= 1,//0:Parallel interface, 1:MIPI interface 
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

#endif

#ifdef CONFIG_OV5640

static struct msm_camera_csi_lane_params ov5640_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x3,
};
static struct gpio msm8960_ov5640_common_cam_gpio[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"}, 
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};

static struct gpio msm8960_ov5640_cam_gpio[] = {
	{54, GPIOF_DIR_OUT, "CAM_PWD"},
	{107, GPIOF_DIR_OUT, "CAM_RESET"},
};
static struct msm_gpio_set_tbl msm8960_ov5640_cam_gpio_set_tbl[] = {
	{54, GPIOF_OUT_INIT_HIGH, 1000},
	{54, GPIOF_OUT_INIT_LOW, 1000},
	{107, GPIOF_OUT_INIT_HIGH, 5000},
	{107, GPIOF_OUT_INIT_LOW, 5000},
	{107, GPIOF_OUT_INIT_HIGH, 1000},
};
static struct msm_camera_gpio_conf msm_8960_ov5640_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_ov5640_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_ov5640_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_ov5640_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_ov5640_cam_gpio),
	.cam_gpio_set_tbl = msm8960_ov5640_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_ov5640_cam_gpio_set_tbl),
};
static struct camera_vreg_t msm_8960_ov5640_vreg[] = {
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
#if defined (CONFIG_MACH_ELDEN)
       {"cam_vio", REG_VS, 0, 0, 0},
#endif
	{"cam_vaf", REG_LDO, 2800000, 2800000, 300000},
};
static struct msm_camera_sensor_flash_data flash_ov5640 = {
	.flash_type	= MSM_CAMERA_FLASH_LED,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5640 = {
	.mount_angle	= 90,  
	.cam_vreg = msm_8960_ov5640_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_ov5640_vreg),
	.gpio_conf = &msm_8960_ov5640_cam_gpio_conf,
	.csi_lane_params = &ov5640_csi_lane_params
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5640_data = {
	.sensor_name	= "ov5640",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov5640,
	.sensor_platform_info = &sensor_board_info_ov5640,
	.csi_if	= 1,//0:Parallel interface, 1:MIPI interface 
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

#endif
#ifdef CONFIG_OV7692

static struct msm_camera_csi_lane_params ov7692_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0x1,
};
static struct gpio msm8960_ov7692_common_cam_gpio[] = {
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"}, 
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};

static struct gpio msm8960_ov7692_cam_gpio[] = {
	{53, GPIOF_DIR_OUT, "CAM_PWD"},
};
static struct msm_gpio_set_tbl msm8960_ov7692_cam_gpio_set_tbl[] = {
	{53, GPIOF_OUT_INIT_HIGH, 1000},
	{53, GPIOF_OUT_INIT_LOW, 1000},
};
static struct msm_camera_gpio_conf msm_8960_ov7692_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_ov7692_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_ov7692_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_ov7692_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_ov7692_cam_gpio),
	.cam_gpio_set_tbl = msm8960_ov7692_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_ov7692_cam_gpio_set_tbl),
};
static struct camera_vreg_t msm_8960_ov7692_vreg[] = {
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
	{"cam_vio", REG_VS, 0, 0, 0},
};
static struct msm_camera_sensor_flash_data flash_ov7692 = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov7692 = {
	.mount_angle	= 270,  
	.cam_vreg = msm_8960_ov7692_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_ov7692_vreg),
	.gpio_conf = &msm_8960_ov7692_cam_gpio_conf,
	.csi_lane_params = &ov7692_csi_lane_params
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7692_data = {
	.sensor_name	= "ov7692",
	.pdata	= &msm_camera_csi_device_data[1],
	.flash_data	= &flash_ov7692,
	.sensor_platform_info = &sensor_board_info_ov7692,
	.csi_if	= 1,//0:Parallel interface, 1:MIPI interface 
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

#endif


//add by wxl for p896a25 
#ifdef CONFIG_OV8825

static struct i2c_board_info ov8825_actuator_i2c_info = {
 I2C_BOARD_INFO("msm_actuator", 0x6C>>2),
 };

static struct msm_actuator_info ov8825_actuator_info = {
  .board_info     = &ov8825_actuator_i2c_info,
  .cam_name       = MSM_ACTUATOR_MAIN_CAM_3,
  .bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
  .vcm_pwd        = 0,
  .vcm_enable    = 0,
  };

static struct msm_camera_csi_lane_params ov8825_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};

static struct gpio msm8960_ov8825_common_cam_gpio[] = {
// by lijing for P896D01
#if defined (CONFIG_MACH_MAZAMA)
    {4, GPIOF_DIR_IN, "CAMIF_MCLK"}, 
#else
	{5, GPIOF_DIR_IN, "CAMIF_MCLK"}, 
#endif
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};

static struct camera_vreg_t msm_8960_ov8825_vreg[] = {
#if defined (CONFIG_MACH_ROUNDTOP) || defined (CONFIG_MACH_NUNIVAK)  //add by jzq for p896a11
	{"cam_vio", REG_LDO, 1800000, 1800000, 5},
	{"cam_vana", REG_GPIO, 0, 0, 89,5},
#endif	
#if defined (CONFIG_MACH_OKMOK) ||defined(CONFIG_MACH_PRINDLE)	  //by lijing for P896A25
    {"cam_vana", REG_LDO, 2800000, 2850000, 85600,0},
#endif	
#if defined (CONFIG_MACH_MAZAMA)
	{"cam_vio", REG_VS, 1800000, 1800000, 0, 50},
	{"cam_vana", REG_LDO, 2800000, 2850000, 85600,0},
#endif
	{"cam_vdig", REG_GPIO, 0, 0, 9,5}, 
	{"cam_vaf", REG_LDO, 2800000, 2800000, 0},
	
	//{"cam_vaf", REG_LDO, 2800000, 2800000, 300000},
};

#if defined (CONFIG_MACH_ROUNDTOP)|| defined (CONFIG_MACH_OKMOK)
static struct camera_vreg_t msm_8960_ov8825_bug_vreg[] = {
       {"cam_vdigbug", REG_VS, 1800000, 1800000, 0, 50},
	{"cam_vanabug", REG_LDO, 2800000, 2850000, 85600, 50},
};
#endif //jzq add for fix can not probe 8825 bug

static struct msm_camera_sensor_flash_data flash_ov8825 = {
#ifdef CONFIG_FLSH_ADP1650 /* Modified By jzq for Flash 12.11.06*/	
	.flash_type	= MSM_CAMERA_FLASH_LED,
#else
       .flash_type	= MSM_CAMERA_FLASH_NONE,
#endif
#ifdef CONFIG_MSM_CAMERA_FLASH
	//.flash_src	= &msm_flash_src
#endif
};

static struct gpio msm8960_ov8825_cam_gpio[] = {
	{54, GPIOF_DIR_OUT, "CAM_PWD"},
	{107, GPIOF_DIR_OUT, "CAM_RESET"},
};
static struct msm_gpio_set_tbl msm8960_ov8825_cam_gpio_set_tbl[] = {
	//{54, GPIOF_OUT_INIT_LOW, 1000},
	{54, GPIOF_OUT_INIT_HIGH, 1000},
	{107, GPIOF_OUT_INIT_LOW, 1000},
	{107, GPIOF_OUT_INIT_HIGH, 4000},
};
static struct msm_camera_gpio_conf msm_8960_ov8825_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_ov8825_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_ov8825_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_ov8825_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_ov8825_cam_gpio),
	.cam_gpio_set_tbl = msm8960_ov8825_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_ov8825_cam_gpio_set_tbl),
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov8825 = {
	.mount_angle	= 90,
	.cam_vreg = msm_8960_ov8825_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_ov8825_vreg),
	.gpio_conf = &msm_8960_ov8825_gpio_conf,
	.csi_lane_params = &ov8825_csi_lane_params,
#if defined (CONFIG_MACH_ROUNDTOP) || defined (CONFIG_MACH_OKMOK)
	.cam_bug_vreg = msm_8960_ov8825_bug_vreg,
	.num_bug_vreg = ARRAY_SIZE(msm_8960_ov8825_bug_vreg),
#endif //jzq add for fix can not probe 8825 bug
};


static struct msm_camera_sensor_info msm_camera_sensor_ov8825_data = {
	.sensor_name	= "ov8825",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_ov8825,
	.sensor_platform_info = &sensor_board_info_ov8825,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &ov8825_actuator_info,  
};
#endif

#ifdef CONFIG_IMX091
static struct i2c_board_info imx091_actuator_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x18>>1),
};
static struct msm_actuator_info imx091_actuator_info = {
	.board_info     = &imx091_actuator_i2c_info,
	.cam_name       = MSM_ACTUATOR_MAIN_CAM_1,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.vcm_pwd        = 0,
	.vcm_enable    = 0,
};
static struct msm_camera_csi_lane_params imx091_csi_lane_params = {
	.csi_lane_assign = 0xE4,
	.csi_lane_mask = 0xF,
};
static struct gpio msm8960_imx091_common_cam_gpio[] = {
	{4, GPIOF_DIR_IN, "CAMIF_MCLK"},
	{20, GPIOF_DIR_IN, "CAMIF_I2C_DATA"},
	{21, GPIOF_DIR_IN, "CAMIF_I2C_CLK"},
};
static struct gpio msm8960_imx091_cam_gpio[] = {
	{107, GPIOF_DIR_OUT, "CAM_RESET"},
};
static struct msm_gpio_set_tbl msm8960_imx091_cam_gpio_set_tbl[] = {
	//{107, GPIOF_OUT_INIT_LOW, 1000},
	{107, GPIOF_OUT_INIT_HIGH, 20000},
};
static struct msm_camera_gpio_conf msm_8960_imx091_cam_gpio_conf = {
	.cam_gpiomux_conf_tbl = msm8960_cam_2d_configs,
	.cam_gpiomux_conf_tbl_size = ARRAY_SIZE(msm8960_cam_2d_configs),
	.cam_gpio_common_tbl = msm8960_imx091_common_cam_gpio,
	.cam_gpio_common_tbl_size = ARRAY_SIZE(msm8960_imx091_common_cam_gpio),
	.cam_gpio_req_tbl = msm8960_imx091_cam_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(msm8960_imx091_cam_gpio),
	.cam_gpio_set_tbl = msm8960_imx091_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(msm8960_imx091_cam_gpio_set_tbl),
};
static struct camera_vreg_t msm_8960_imx091_vreg[] = {
	//{"cam_vana", REG_LDO, 2800000, 2850000, 85600},
     	//{"cam_vdig", REG_LDO, 1200000, 1200000, 105000},
     	{"cam_vio_lvs5", REG_VS, 0, 0, 0},
     	{"cam_vio_l29", REG_LDO, 1800000, 1800000, 100000},
       {"cam_vaf", REG_LDO, 2800000, 2800000, 300000},
     	{"cam_vana", REG_GPIO, 0, 0, 89,0},
	{"cam_vdig", REG_GPIO, 0, 0, 9,0}, 

};
static struct msm_camera_sensor_flash_data flash_imx091 = {
#ifdef CONFIG_FLSH_ADP1650
	.flash_type	= MSM_CAMERA_FLASH_LED,
#else
	.flash_type	= MSM_CAMERA_FLASH_NONE,		
#endif
};
static struct msm_camera_sensor_platform_info sensor_board_info_imx091 = {
	.mount_angle	= 90,
	.cam_vreg = msm_8960_imx091_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_imx091_vreg),
	.gpio_conf = &msm_8960_imx091_cam_gpio_conf, 
	.csi_lane_params = &imx091_csi_lane_params,
};

//modified by zhang.yu_1 for support eeprom to imx091 sensor
static struct i2c_board_info imx091_eeprom_i2c_info = {
	I2C_BOARD_INFO("imx091_eeprom", 0xa0>>1),
};

static struct msm_eeprom_info imx091_eeprom_info = {
	.board_info     = &imx091_eeprom_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
	.eeprom_i2c_slave_addr = 0xA0,
	.eeprom_reg_addr = 0x0,
	.eeprom_read_length = 20,
};


static struct msm_camera_sensor_info msm_camera_sensor_imx091_data = {
	.sensor_name	= "imx091",
	.pdata	= &msm_camera_csi_device_data[0],
	.flash_data	= &flash_imx091,
	.sensor_platform_info = &sensor_board_info_imx091,
	.csi_if	= 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &imx091_actuator_info,
	.eeprom_info = &imx091_eeprom_info,
};
#endif

#if 0
static struct msm_camera_sensor_flash_data flash_ov8830 = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_csi_lane_params ov8830_csi_lane_params = {
	.csi_lane_assign = 0x9C,
	.csi_lane_mask = 0x9,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov8830 = {
	.mount_angle = 90,
	.cam_vreg = msm_8960_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_8960_cam_vreg),
	.gpio_conf = &msm_8960_back_cam_gpio_conf,
	.csi_lane_params = &ov8830_csi_lane_params,
};


static struct i2c_board_info ov8830_eeprom_i2c_info = {
	I2C_BOARD_INFO("ov8830_eeprom", 0x6C >> 3),
};

static struct msm_eeprom_info ov8830_eeprom_info = {
	.board_info     = &ov8830_eeprom_i2c_info,
	.bus_id         = MSM_8960_GSBI4_QUP_I2C_BUS_ID,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov8830_data = {
	.sensor_name = "ov8830",
	.pdata = &msm_camera_csi_device_data[0],
	.flash_data = &flash_ov8830,
	.sensor_platform_info = &sensor_board_info_ov8830,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	//.actuator_info = &msm_act_main_cam_1_info,
	.eeprom_info = &ov8830_eeprom_info,
};
#endif

static int32_t msm_camera_8960_ext_power_ctrl(int enable)
{
	int rc = 0;
	if (enable) {
		rc = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(12),
			&privacy_light_on_config);
	} else {
		rc = pm8xxx_mpp_config(PM8921_MPP_PM_TO_SYS(12),
			&privacy_light_off_config);
	}
	return rc;
}

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

void __init msm8960_init_cam(void)
{
	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE) {
		msm_8960_front_cam_gpio_conf.cam_gpiomux_conf_tbl =
			msm8960_cam_2d_configs_sglte;
		msm_8960_front_cam_gpio_conf.cam_gpiomux_conf_tbl_size =
			ARRAY_SIZE(msm8960_cam_2d_configs_sglte);
		msm_8960_back_cam_gpio_conf.cam_gpiomux_conf_tbl =
			msm8960_cam_2d_configs_sglte;
		msm_8960_back_cam_gpio_conf.cam_gpiomux_conf_tbl_size =
			ARRAY_SIZE(msm8960_cam_2d_configs_sglte);
	}
	msm_gpiomux_install(msm8960_cam_common_configs,
			ARRAY_SIZE(msm8960_cam_common_configs));

	if (machine_is_msm8960_cdp()) {
		msm_gpiomux_install(msm8960_cdp_flash_configs,
			ARRAY_SIZE(msm8960_cdp_flash_configs));
		msm_flash_src._fsrc.ext_driver_src.led_en =
			GPIO_CAM_GP_LED_EN1;
		msm_flash_src._fsrc.ext_driver_src.led_flash_en =
			GPIO_CAM_GP_LED_EN2;
		#if defined(CONFIG_I2C) && (defined(CONFIG_GPIO_SX150X) || \
		defined(CONFIG_GPIO_SX150X_MODULE))
		msm_flash_src._fsrc.ext_driver_src.expander_info =
			cam_expander_info;
		#endif
	}

	if (machine_is_msm8960_liquid()) {
		struct msm_camera_sensor_info *s_info;
		s_info = &msm_camera_sensor_imx074_data;
		s_info->sensor_platform_info->mount_angle = 180;
		s_info = &msm_camera_sensor_ov2720_data;
		s_info->sensor_platform_info->ext_power_ctrl =
			msm_camera_8960_ext_power_ctrl;
	}
#if 0
	if (machine_is_msm8960_fluid()) {
		msm_camera_sensor_imx091_data.sensor_platform_info->
			mount_angle = 270;
	}
#endif

	platform_device_register(&msm_camera_server);
	platform_device_register(&msm8960_device_csiphy0);
	platform_device_register(&msm8960_device_csiphy1);
	platform_device_register(&msm8960_device_csiphy2);
	platform_device_register(&msm8960_device_csid0);
	platform_device_register(&msm8960_device_csid1);
	platform_device_register(&msm8960_device_csid2);
	platform_device_register(&msm8960_device_ispif);
	platform_device_register(&msm8960_device_vfe);
	platform_device_register(&msm8960_device_vpe);
}

#ifdef CONFIG_I2C
static struct i2c_board_info msm8960_camera_i2c_boardinfo[] = {
	{
	I2C_BOARD_INFO("imx074", 0x1A),
	.platform_data = &msm_camera_sensor_imx074_data,
	},
	{
	I2C_BOARD_INFO("ov2720", 0x6C),
	.platform_data = &msm_camera_sensor_ov2720_data,
	},
#ifdef CONFIG_OV8830	
	{
	I2C_BOARD_INFO("ov8830", 0x6C >> 1),
	.platform_data = &msm_camera_sensor_ov8830_data,
	},
#endif
#ifdef CONFIG_OV9726
	{
		I2C_BOARD_INFO("ov9726", 0x10),
		.platform_data = &msm_camera_sensor_ov9726_data,
	},
#endif
	{
	I2C_BOARD_INFO("mt9m114", 0x48),
	.platform_data = &msm_camera_sensor_mt9m114_data,
	},
	{
	I2C_BOARD_INFO("s5k3l1yx", 0x20),
	.platform_data = &msm_camera_sensor_s5k3l1yx_data,
	},
#ifdef CONFIG_MSM_CAMERA_FLASH_SC628A
	{
	I2C_BOARD_INFO("sc628a", 0x6E),
	},
#endif
#ifdef CONFIG_IMX091
	{
	I2C_BOARD_INFO("imx091", 0x6C>>1),
	.platform_data = &msm_camera_sensor_imx091_data,
	},
#endif	
//add by wxl for p896a25 20121014
#ifdef CONFIG_OV8825
	{
	I2C_BOARD_INFO("ov8825", 0x6C>>1),
	.platform_data = &msm_camera_sensor_ov8825_data,
	},
#endif
#ifdef CONFIG_OV9740
	{
	I2C_BOARD_INFO("ov9740", 0x20>>1),
	.platform_data = &msm_camera_sensor_ov9740_data,
	},
#endif

//Added by ZTE_CAM_CDZ for OV5640
#ifdef CONFIG_OV5640
	{
	I2C_BOARD_INFO("ov5640", 0x78>>1),
	.platform_data = &msm_camera_sensor_ov5640_data,
	},
#endif
//Added by ZTE_CAM_CDZ for OV7692
#ifdef CONFIG_OV7692
	{
	I2C_BOARD_INFO("ov7692", 0x78>>2),
	.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif
};

#ifdef CONFIG_ISPCAM //wt for isp fujitsu 2013-01-21
static struct i2c_board_info msm8960_camera_isp_i2c_boardinfo[] = {
	{
	I2C_BOARD_INFO("ispcam", 0x3E >> 1),
	.platform_data = &msm_camera_sensor_isp_data,
	},
};


struct msm_camera_board_info msm8960_isp_camera_board_info = {
	.board_info = msm8960_camera_isp_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8960_camera_isp_i2c_boardinfo),
};
#endif //wt for isp fujitsu 2013-01-21

struct msm_camera_board_info msm8960_camera_board_info = {
	.board_info = msm8960_camera_i2c_boardinfo,
	.num_i2c_board_info = ARRAY_SIZE(msm8960_camera_i2c_boardinfo),
};
#endif
#endif
