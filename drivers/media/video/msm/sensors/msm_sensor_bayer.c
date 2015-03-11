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
 */

#include "msm_sensor_bayer.h"
#include "msm.h"
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"

extern void msm_sensorinfo_set_back_sensor_id(uint16_t id);
int msm_sensor_bayer_enable_debugfs(struct msm_sensor_ctrl_t *s_ctrl);
/*=============================================================*/

long msm_sensor_bayer_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	void __user *argp = (void __user *)arg;
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_CFG:
		return s_ctrl->func_tbl->sensor_config(s_ctrl, argp);
	case VIDIOC_MSM_SENSOR_RELEASE:
		return 0;
	case VIDIOC_MSM_SENSOR_CSID_INFO: {
		struct msm_sensor_csi_info *csi_info =
			(struct msm_sensor_csi_info *)arg;
		s_ctrl->is_csic = csi_info->is_csic;
		return 0;
	}
	default:
		return -ENOIOCTLCMD;
	}
}

int32_t msm_sensor_bayer_get_csi_params(struct msm_sensor_ctrl_t *s_ctrl,
		struct csi_lane_params_t *sensor_output_info)
{
	uint8_t index;
	struct msm_camera_csi_lane_params *csi_lane_params =
		s_ctrl->sensordata->sensor_platform_info->csi_lane_params;
	if (csi_lane_params) {
		sensor_output_info->csi_lane_assign = csi_lane_params->
			csi_lane_assign;
		sensor_output_info->csi_lane_mask = csi_lane_params->
			csi_lane_mask;
	}
	sensor_output_info->csi_if = s_ctrl->sensordata->csi_if;
	for (index = 0; index < sensor_output_info->csi_if; index++)
		sensor_output_info->csid_core[index] = s_ctrl->sensordata->
			pdata[index].csid_core;

	return 0;
}

int32_t msm_sensor_bayer_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensor_cfg_data cdata;
	long rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata.cfgtype);
	switch (cdata.cfgtype) {
	case CFG_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *regs = NULL;

		if (copy_from_user(&conf_array,
			(void *)cdata.cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		regs = kzalloc(conf_array.size * sizeof(
			struct msm_camera_i2c_reg_array),
			GFP_KERNEL);
		if (!regs) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(regs, (void *)conf_array.reg_setting,
			conf_array.size * sizeof(
			struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(regs);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = regs;
		rc = msm_camera_i2c_write_bayer_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(regs);
		break;
	}
	case CFG_FOR_MSLEEP: {
		uint32_t sleeptime = cdata.cfg.sleeptime;
		pr_err("%s:%d %s  sleeptime=%d", __func__, __LINE__,s_ctrl->sensordata->sensor_name,sleeptime);
		msleep(sleeptime);
		break;
	}
	case CFG_READ_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *regs;
		int index;

		if (copy_from_user(&conf_array,
				(void *)cdata.cfg.setting,
				sizeof(struct msm_camera_i2c_reg_setting))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
		}

		regs = kzalloc(conf_array.size * sizeof(
				struct msm_camera_i2c_reg_array),
				GFP_KERNEL);
		if (!regs) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			kfree(regs);
			break;
		}

		if (copy_from_user(regs, (void *)conf_array.reg_setting,
				conf_array.size * sizeof(
				struct msm_camera_i2c_reg_array))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				kfree(regs);
				rc = -EFAULT;
				break;
			}

		s_ctrl->sensor_i2c_client->addr_type = conf_array.addr_type;
		for (index = 0; index < conf_array.size; index++) {
			msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
					regs[index].reg_addr,
					&regs[index].reg_data,
				conf_array.data_type
				);
		}

		if (copy_to_user(conf_array.reg_setting,
			regs,
			conf_array.size * sizeof(
			struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(regs);
			rc = -EFAULT;
			break;
		}
		s_ctrl->sensor_i2c_client->addr_type = conf_array.addr_type;
		kfree(regs);
		break;
	}
	case CFG_PCLK_CHANGE: {
		uint32_t pclk = cdata.cfg.pclk;
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &pclk);
		break;
	}
	case CFG_GPIO_OP: {
		struct msm_cam_gpio_operation gop;
		if (copy_from_user(&gop,
			(void *)cdata.cfg.setting,
			sizeof(struct msm_cam_gpio_operation))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
		}
		switch (gop.op_type) {
		case GPIO_GET_VALUE:
			gop.value = gpio_get_value(gop.address);
			if (copy_from_user((void *)cdata.cfg.setting,
				&gop,
				sizeof(struct msm_cam_gpio_operation))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
			break;
		case GPIO_SET_VALUE:
			gpio_set_value(gop.address, gop.value);
			break;
		case GPIO_SET_DIRECTION_INPUT:
			gpio_direction_input(gop.address);
			break;
		case GPIO_SET_DIRECTION_OUTPUT:
			gpio_direction_output(gop.address, gop.value);
			break;
		case GPIO_REQUEST:
			gpio_request(gop.address, gop.tag);
			break;
		case GPIO_FREE:
			gpio_free(gop.address);
			break;
		default:
			break;
		}

		break;
	}
	case CFG_GET_CSI_PARAMS:
		if (s_ctrl->func_tbl->sensor_get_csi_params == NULL) {
			rc = -EFAULT;
			break;
		}
		rc = s_ctrl->func_tbl->sensor_get_csi_params(
			s_ctrl,
			&cdata.cfg.csi_lane_params);

		if (copy_to_user((void *)argp,
			&cdata,
			sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;

	case CFG_POWER_UP:
		pr_err("%s calling power up\n", __func__);
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

//jzq add for otp
	case CFG_OTP_PARAMS:
		pr_err("%s calling otp params\n", __func__);
		if (s_ctrl->func_tbl->sensor_otp_func)
			rc = s_ctrl->func_tbl->sensor_otp_func(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
		pr_err("%s calling power down\n", __func__);
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_CONFIG_VREG_ARRAY: {
		struct msm_camera_vreg_setting vreg_setting;
		struct camera_vreg_t *cam_vreg = NULL;

		if (copy_from_user(&vreg_setting,
			(void *)cdata.cfg.setting,
			sizeof(struct msm_camera_vreg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		cam_vreg = kzalloc(vreg_setting.num_vreg * sizeof(
			struct camera_vreg_t),
			GFP_KERNEL);
		if (!cam_vreg) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(cam_vreg, (void *)vreg_setting.cam_vreg,
			vreg_setting.num_vreg * sizeof(
			struct camera_vreg_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(cam_vreg);
			rc = -EFAULT;
			break;
		}
		rc = msm_camera_config_vreg(
			&s_ctrl->sensor_i2c_client->client->dev,
			cam_vreg,
			vreg_setting.num_vreg,
			NULL,
			0,
			s_ctrl->reg_ptr,
			vreg_setting.enable);
		if (rc < 0) {
			kfree(cam_vreg);
			pr_err("%s: regulator on failed\n", __func__);
			break;
		}

		rc = msm_camera_enable_vreg(
			&s_ctrl->sensor_i2c_client->client->dev,
			cam_vreg,
			vreg_setting.num_vreg,
			NULL,
			0,
			s_ctrl->reg_ptr,
			vreg_setting.enable);
		if (rc < 0) {
			kfree(cam_vreg);
			pr_err("%s: enable regulator failed\n", __func__);
			break;
		}
		kfree(cam_vreg);
		break;
	}
	case CFG_CONFIG_CLK_ARRAY: {
		struct msm_cam_clk_setting clk_setting;
		struct msm_cam_clk_info *clk_info = NULL;

		if (copy_from_user(&clk_setting,
			(void *)cdata.cfg.setting,
			sizeof(struct msm_camera_vreg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		clk_info = kzalloc(clk_setting.num_clk_info * sizeof(
			struct msm_cam_clk_info),
			GFP_KERNEL);
		if (!clk_info) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(clk_info, (void *)clk_setting.clk_info,
			clk_setting.num_clk_info * sizeof(
			struct msm_cam_clk_info))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(clk_info);
			rc = -EFAULT;
			break;
		}
		rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
			clk_info, s_ctrl->cam_clk,
			clk_setting.num_clk_info,
			clk_setting.enable);
		kfree(clk_info);
		break;
	}
	case CFG_GET_EEPROM_DATA: {
		if (copy_to_user((void *)cdata.cfg.eeprom_data.eeprom_data,
			&s_ctrl->eeprom_data, s_ctrl->eeprom_data.length)) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
		}
		cdata.cfg.eeprom_data.index = s_ctrl->eeprom_data.length;
		break;
	}
	default:
		rc = -EFAULT;
		break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

int32_t msm_sensor_bayer_enable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_INIT, NULL);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
		VIDIOC_MSM_I2C_MUX_CFG, (void *)&i2c_conf->i2c_mux_mode);
	return 0;
}

int32_t msm_sensor_bayer_disable_i2c_mux(struct msm_camera_i2c_conf *i2c_conf)
{
	struct v4l2_subdev *i2c_mux_sd =
		dev_get_drvdata(&i2c_conf->mux_dev->dev);
	v4l2_subdev_call(i2c_mux_sd, core, ioctl,
				VIDIOC_MSM_I2C_MUX_RELEASE, NULL);
	return 0;
}

static struct msm_camera_power_seq_t sensor_power_seq[] = {
	{REQUEST_GPIO, 0},
	{REQUEST_VREG, 0},
	{ENABLE_VREG, 0},
	{ENABLE_GPIO, 0},
	{CONFIG_CLK, 0},
	{CONFIG_EXT_POWER_CTRL, 0},
	{CONFIG_I2C_MUX, 0},
};

int32_t msm_sensor_bayer_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0, size = 0, index = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct msm_camera_power_seq_t *power_seq = NULL;
	CDBG("%s: %d\n", __func__, __LINE__);
	if (s_ctrl->power_seq) {
		power_seq = s_ctrl->power_seq;
		size = s_ctrl->num_power_seq;
	} else {
		power_seq = &sensor_power_seq[0];
		size = ARRAY_SIZE(sensor_power_seq);
	}

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}

	for (index = 0; index < size; index++) {
		switch (power_seq[index].power_config) {
		case REQUEST_GPIO:
			rc = msm_camera_request_gpio_table(data, 1);
			if (rc < 0) {
				pr_err("%s: request gpio failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case REQUEST_VREG:
			rc = msm_camera_config_vreg(
				&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 1);
			if (rc < 0) {
				pr_err("%s: regulator on failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case ENABLE_VREG:
			rc = msm_camera_enable_vreg(
				&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 1);
			if (rc < 0) {
				pr_err("%s: enable regulator failed\n",
					__func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case ENABLE_GPIO:
			rc = msm_camera_config_gpio_table(data, 1);
			if (rc < 0) {
				pr_err("%s: config gpio failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case CONFIG_CLK:
			if (s_ctrl->clk_rate != 0)
				cam_clk_info->clk_rate = s_ctrl->clk_rate;

			rc = msm_cam_clk_enable(
				&s_ctrl->sensor_i2c_client->client->dev,
				cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 1);
			if (rc < 0) {
				pr_err("%s: clk enable failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case CONFIG_EXT_POWER_CTRL:
			if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(1);
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case CONFIG_I2C_MUX:
			if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_enable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		default:
			pr_err("%s error power config %d\n", __func__,
				power_seq[index].power_config);
			rc = -EINVAL;
			break;
		}
	}

	return rc;

ERROR:
	for (index--; index >= 0; index--) {
		switch (power_seq[index].power_config) {
		case CONFIG_I2C_MUX:
			if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_disable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
			break;
		case CONFIG_EXT_POWER_CTRL:
			if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(0);
			break;
		case CONFIG_CLK:
			msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->
				dev, cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 0);
			break;
		case ENABLE_GPIO:
			msm_camera_config_gpio_table(data, 0);
			break;
		case ENABLE_VREG:
			msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);
			break;
		case REQUEST_VREG:
			msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);
			break;
		case REQUEST_GPIO:
			msm_camera_request_gpio_table(data, 0);
			break;
		default:
			pr_err("%s error power config %d\n", __func__,
				power_seq[index].power_config);
			break;
		}
	}
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t msm_sensor_bayer_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t size = 0, index = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct msm_camera_power_seq_t *power_seq = NULL;
	CDBG("%s\n", __func__);

	if (s_ctrl->power_seq) {
		power_seq = s_ctrl->power_seq;
		size = s_ctrl->num_power_seq;
	} else {
		power_seq = &sensor_power_seq[0];
		size = ARRAY_SIZE(sensor_power_seq);
	}

	for (index = (size - 1); index >= 0; index--) {
		switch (power_seq[index].power_config) {
		case CONFIG_I2C_MUX:
			if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_disable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
			break;
		case CONFIG_EXT_POWER_CTRL:
			if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(0);
			break;
		case CONFIG_CLK:
			msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->
				dev, cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 0);
			break;
		case ENABLE_GPIO:
			msm_camera_config_gpio_table(data, 0);
			break;
		case ENABLE_VREG:
			msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);
			break;
		case REQUEST_VREG:
			msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);
			break;
		case REQUEST_GPIO:
			msm_camera_request_gpio_table(data, 0);
			break;
		default:
			pr_err("%s error power config %d\n", __func__,
				power_seq[index].power_config);
			break;
		}
	}
	kfree(s_ctrl->reg_ptr);
	return 0;
}
int32_t msm_sensor_bayer_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
	rc = msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
			MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: %s: read id failed\n", __func__,
			s_ctrl->sensordata->sensor_name);
		return rc;
	}

	pr_err("%s: read id: %x expected id %x:\n", __func__, chipid,
		s_ctrl->sensor_id_info->sensor_id);
	if (chipid != s_ctrl->sensor_id_info->sensor_id) {
		pr_err("msm_sensor_match_id chip id doesnot match\n");
		return -ENODEV;
	}
	return rc;
}

//modified by zhang.yu_1 for support eeprom to imx091 sensor
#if defined(CONFIG_IMX091)
static int32_t msm_sensor_bayer_eeprom_read(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint32_t reg_addr = 0;
	uint8_t *data = s_ctrl->eeprom_data.data;
	uint32_t num_byte = 0;
	int rc = 0;
	uint32_t i2c_addr;
	struct msm_camera_sensor_info *sensor_info = s_ctrl->sensordata;
	i2c_addr = sensor_info->eeprom_info->eeprom_i2c_slave_addr;
	num_byte = s_ctrl->eeprom_data.length = sensor_info->eeprom_info->
		eeprom_read_length;
	reg_addr = sensor_info->eeprom_info->eeprom_reg_addr;

	data = kzalloc(num_byte * sizeof(uint8_t), GFP_KERNEL);
	if (!data) {
		pr_err("%s:%d failed\n", __func__, __LINE__);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensor_i2c_client->client->addr = i2c_addr;
	CDBG("eeprom read: i2c addr is %x num byte %d  reg addr %x\n",
		i2c_addr, num_byte, reg_addr);
	rc = msm_camera_i2c_read_seq(s_ctrl->sensor_i2c_client, reg_addr, data,
		num_byte);
	s_ctrl->sensor_i2c_client->client->addr = s_ctrl->sensor_i2c_addr;
	return rc;
}
#endif

#if defined (CONFIG_OV8825)
static int OV8825_IsPowered;
static struct msm_camera_i2c_reg_array ov8825_sensor_reset_settings[]={
  {0x0103, 0x01},
};

static struct msm_camera_i2c_reg_array ov8825_sensor_init_settings[] ={
  //OV8825 Camera Application Notes R1.12
  {0x3000, 0x16}, // strobe disable, frex disable, vsync disable
  {0x3001, 0x00}, 
  {0x3002, 0x6c}, // SCCB ID = 0x6c
  {0x300d, 0x00}, // PLL2
  {0x301f, 0x09}, // frex_mask_mipi, frex_mask_mipi_phy
  {0x3010, 0x00}, // strobe, sda, frex, vsync, shutter GPIO unselected
  {0x3018, 0x00}, // clear PHY HS TX power down and PHY LP RX power down
  {0x3300, 0x00}, 
  {0x3500, 0x00}, // exposure[19:16] = 0
  {0x3503, 0x07}, // Gain has no delay, VTS manual, AGC manual, AEC manual
  {0x3509, 0x00}, // use sensor gain
  {0x3602, 0x42}, 
  {0x3603, 0x5c}, // analog control
  {0x3604, 0x98}, // analog control
  {0x3605, 0xf5}, // analog control
  {0x3609, 0xb4}, // analog control
  {0x360a, 0x7c}, // analog control
  {0x360b, 0xc9}, // analog control
  {0x360c, 0x0b}, // analog control
  {0x3612, 0x00}, // pad drive 1x, analog control
  {0x3613, 0x02}, // analog control
  {0x3614, 0x0f}, // analog control
  {0x3615, 0x00}, // analog control
  {0x3616, 0x03}, // analog control
  {0x3617, 0xa1}, // analog control
  {0x3618, 0x00}, // VCM position & slew rate, slew rate = 0
  {0x3619, 0x00}, // VCM position = 0
  {0x361a, 0xB0}, // VCM clock divider, VCM clock = 24000000/0x4b0 = 20000
  {0x361b, 0x04}, // VCM clock divider
  {0x361c, 0x07}, //added by CDZ_CAM_ZTE FOR AF  electric current  
  {0x3701, 0x44}, // sensor control
  {0x370b, 0x01}, // sensor control
  {0x370c, 0x50}, // sensor control
  {0x370d, 0x00}, // sensor control
  {0x3816, 0x02}, // Hsync start H
  {0x3817, 0x40}, // Hsync start L
  {0x3818, 0x00}, // Hsync end H
  {0x3819, 0x40}, // Hsync end L
  {0x3b1f, 0x00}, // Frex conrol
  // clear OTP data buffer
  {0x3d00, 0x00},
  {0x3d01, 0x00},
  {0x3d02, 0x00},
  {0x3d03, 0x00},
  {0x3d04, 0x00},
  {0x3d05, 0x00},
  {0x3d06, 0x00},
  {0x3d07, 0x00},
  {0x3d08, 0x00},
  {0x3d09, 0x00},
  {0x3d0a, 0x00},
  {0x3d0b, 0x00},
  {0x3d0c, 0x00},
  {0x3d0d, 0x00},
  {0x3d0e, 0x00},
  {0x3d0f, 0x00},
  {0x3d10, 0x00},
  {0x3d11, 0x00},
  {0x3d12, 0x00},
  {0x3d13, 0x00},
  {0x3d14, 0x00},
  {0x3d15, 0x00},
  {0x3d16, 0x00},
  {0x3d17, 0x00},
  {0x3d18, 0x00},
  {0x3d19, 0x00},
  {0x3d1a, 0x00},
  {0x3d1b, 0x00},
  {0x3d1c, 0x00},
  {0x3d1d, 0x00},
  {0x3d1e, 0x00},
  {0x3d1f, 0x00},
  {0x3d80, 0x00},
  {0x3d81, 0x00},
  {0x3d84, 0x00},
  {0x3f06, 0x00},
  {0x3f07, 0x00},
  // BLC
  {0x4000, 0x29},
  {0x4001, 0x02}, // BLC start line
  {0x4002, 0x45}, // BLC auto, reset 5 frames
  {0x4003, 0x08}, // BLC redo at 8 frames
  {0x4004, 0x04}, // 4 black lines are used for BLC
  {0x4005, 0x18}, // no black line output, apply one channel offiset (0x400c, 0x400d) to all manual BLC channels
  {0x4300, 0xff}, // max
  {0x4303, 0x00}, // format control
  {0x4304, 0x08}, // output {data[7:0], data[9:8]}
  {0x4307, 0x00}, // embeded control
  //MIPI
  {0x4800, 0x04},
  {0x4801, 0x0f}, // ECC configure
  {0x4843, 0x02}, // manual set pclk divider
  // ISP
  {0x5000, 0x06}, // LENC off, BPC on, WPC on
  {0x5001, 0x00}, // MWB off
  {0x5002, 0x00},
  {0x501f, 0x00}, // enable ISP
  {0x5780, 0xfc},
  {0x5c05, 0x00}, // pre BLC
  {0x5c06, 0x00}, // pre BLC
  {0x5c07, 0x80}, // pre BLC
  // temperature sensor
  {0x6700, 0x05},
  {0x6701, 0x19},
  {0x6702, 0xfd},
  {0x6703, 0xd7},
  {0x6704, 0xff},
  {0x6705, 0xff},
  {0x6800, 0x10},
  {0x6801, 0x02},
  {0x6802, 0x90},
  {0x6803, 0x10},
  {0x6804, 0x59},
  {0x6901, 0x04}, // CADC control
  //Lens Control
  {0x5800, 0x0f},
  {0x5801, 0x0d},
  {0x5802, 0x09},
  {0x5803, 0x0a},
  {0x5804, 0x0d},
  {0x5805, 0x14},
  {0x5806, 0x0a},
  {0x5807, 0x04},
  {0x5808, 0x03},
  {0x5809, 0x03},
  {0x580a, 0x05},
  {0x580b, 0x0a},
  {0x580c, 0x05},
  {0x580d, 0x02},
  {0x580e, 0x00},
  {0x580f, 0x00},
  {0x5810, 0x03},
  {0x5811, 0x05},
  {0x5812, 0x09},
  {0x5813, 0x03},
  {0x5814, 0x01},
  {0x5815, 0x01},
  {0x5816, 0x04},
  {0x5817, 0x09},
  {0x5818, 0x09},
  {0x5819, 0x08},
  {0x581a, 0x06},
  {0x581b, 0x06},
  {0x581c, 0x08},
  {0x581d, 0x06},
  {0x581e, 0x33},
  {0x581f, 0x11},
  {0x5820, 0x0e},
  {0x5821, 0x0f},
  {0x5822, 0x11},
  {0x5823, 0x3f},
  {0x5824, 0x08},
  {0x5825, 0x46},
  {0x5826, 0x46},
  {0x5827, 0x46},
  {0x5828, 0x46},
  {0x5829, 0x46},
  {0x582a, 0x42},
  {0x582b, 0x42},
  {0x582c, 0x44},
  {0x582d, 0x46},
  {0x582e, 0x46},
  {0x582f, 0x60},
  {0x5830, 0x62},
  {0x5831, 0x42},
  {0x5832, 0x46},
  {0x5833, 0x46},
  {0x5834, 0x44},
  {0x5835, 0x44},
  {0x5836, 0x44},
  {0x5837, 0x48},
  {0x5838, 0x28},
  {0x5839, 0x46},
  {0x583a, 0x48},
  {0x583b, 0x68},
  {0x583c, 0x28},
  {0x583d, 0xae},
  {0x5842, 0x00},
  {0x5843, 0xef},
  {0x5844, 0x01},
  {0x5845, 0x3f},
  {0x5846, 0x01},
  {0x5847, 0x3f},
  {0x5848, 0x00},
  {0x5849, 0xd5},
  // Exposure
  {0x3503, 0x07}, // Gain has no delay, VTS manual, AGC manual, AEC manual
  {0x3500, 0x00}, // expo[19:16] = lines/16
  {0x3501, 0x27}, // expo[15:8] 0x27
  {0x3502, 0x00}, // expo[7:0] 0x00
  {0x350b, 0xff}, // gain
  // MWB
  {0x3400, 0x04}, // red h
  {0x3401, 0x00}, // red l
  {0x3402, 0x04}, // green h
  {0x3403, 0x00}, // green l
  {0x3404, 0x04}, // blue h
  {0x3405, 0x00}, // blue l
  {0x3406, 0x01}, // MWB manual
  // ISP
  {0x5001, 0x01}, // MWB on
  {0x5000, 0x06}, // LENC off, BPC on, WPC on
//  {0x361c, 0x07},//added by CDZ_CAM_ZTE FOR AF  electric current  
{0x3608, 0x40},//0x40-->external DVDD,, 0x00-->internal DVDD
 // {0x4303,0x08},//colorbar
 
 //OV8820 LENC setting
{0x5000,0x86},//enable lenc
{0x5800,0x14},
{0x5801,0xc },
{0x5802,0xa },
{0x5803,0xb },
{0x5804,0xd },
{0x5805,0x16},
{0x5806,0x9 },
{0x5807,0x5 },
{0x5808,0x3 },
{0x5809,0x4 },
{0x580a,0x6 },
{0x580b,0xa },
{0x580c,0x6 },
{0x580d,0x2 },
{0x580e,0x0 },
{0x580f,0x0 },
{0x5810,0x2 },
{0x5811,0x6 },
{0x5812,0x6 },
{0x5813,0x2 },
{0x5814,0x0 },
{0x5815,0x0 },
{0x5816,0x2 },
{0x5817,0x7 },
{0x5818,0x9 },
{0x5819,0x6 },
{0x581a,0x4 },
{0x581b,0x4 },
{0x581c,0x7 },
{0x581d,0xa },
{0x581e,0x18},
{0x581f,0xd },
{0x5820,0xb },
{0x5821,0xb },
{0x5822,0xe },
{0x5823,0x1c},
{0x5824,0x28},
{0x5825,0x2a},
{0x5826,0x2a},
{0x5827,0x2a},
{0x5828,0xe },
{0x5829,0x48},
{0x582a,0x66},
{0x582b,0x64},
{0x582c,0x44},
{0x582d,0x48},
{0x582e,0x48},
{0x582f,0x82},
{0x5830,0x80},
{0x5831,0x62},
{0x5832,0x48},
{0x5833,0x48},
{0x5834,0x66},
{0x5835,0x64},
{0x5836,0x66},
{0x5837,0x48},
{0x5838,0x4a},
{0x5839,0x4a},
{0x583a,0x2c},
{0x583b,0x4a},
{0x583c,0x2e},
{0x583d,0x8e},

  //OV8825 Camera Application Notes R1.12
  //change from 24fps to 16fps
  //change r3005 from 10 to 20, r3006 from 00 to 10 by kenxu
  //@@ Capture 3264x2448 16fps 352Mbps/Lane
  //@@3264_2448_4_lane_16fps_66.67Msysclk_352MBps/lane  
  {0x0100, 0x00}, // sleep
  {0x3003, 0xce}, ////PLL_CTRL0
  {0x3004, 0xbf}, ////PLL_CTRL1
  {0x3005, 0x10}, ////PLL_CTRL2     //jzq revise as prev
  {0x3006, 0x00}, ////PLL_CTRL3
  {0x3007, 0x3b}, ////PLL_CTRL4
  {0x3011, 0x02}, ////MIPI_Lane_4_Lane
  {0x3012, 0x80}, ////SC_PLL CTRL_S0
  {0x3013, 0x39}, ////SC_PLL CTRL_S1
  {0x3104, 0x20}, ////SCCB_PLL
  {0x3106, 0x15}, ////SRB_CTRL
  {0x3501, 0x9a}, ////AEC_HIGH  0x9a
  {0x3502, 0xa0}, ////AEC_LOW  0xa0
  {0x350b, 0x1f}, ////AGC
  {0x3600, 0x06}, //ANACTRL0
  {0x3601, 0x34}, //ANACTRL1
  {0x3700, 0x20}, //SENCTROL0 Sensor control
  {0x3702, 0x50}, //SENCTROL2 Sensor control
  {0x3703, 0xcc}, //SENCTROL3 Sensor control
  {0x3704, 0x19}, //SENCTROL4 Sensor control
  {0x3705, 0x14}, //SENCTROL5 Sensor control
  {0x3706, 0x4b}, //SENCTROL6 Sensor control
  {0x3707, 0x63}, //SENCTROL7 Sensor control
  {0x3708, 0x84}, //SENCTROL8 Sensor control
  {0x3709, 0x40}, //SENCTROL9 Sensor control
  {0x370a, 0x12}, //SENCTROLA Sensor control
  {0x370e, 0x00}, //SENCTROLE Sensor control
  {0x3711, 0x0f}, //SENCTROL11 Sensor control
  {0x3712, 0x9c}, //SENCTROL12 Sensor control
  {0x3724, 0x01}, //Reserved
  {0x3725, 0x92}, //Reserved
  {0x3726, 0x01}, //Reserved
  {0x3727, 0xa9}, //Reserved
  {0x3800, 0x00}, //HS(HREF start High)
  {0x3801, 0x00}, //HS(HREF start Low)
  {0x3802, 0x00}, //VS(Vertical start High)
  {0x3803, 0x00}, //VS(Vertical start Low)
  {0x3804, 0x0c}, //HW = 3295
  {0x3805, 0xdf}, //HW
  {0x3806, 0x09}, //VH = 2459
  {0x3807, 0x9b}, //VH
  {0x3808, 0x0c}, //ISPHO = 3264
  {0x3809, 0xc0}, //ISPHO
  {0x380a, 0x09}, //ISPVO = 2448
  {0x380b, 0x90}, //ISPVO
  {0x380c, 0x0d}, //HTS = 3360
  {0x380d, 0x20}, //HTS
  {0x380e, 0x09}, //VTS = 2480
  {0x380f, 0xb0}, //VTS
  {0x3810, 0x00}, //HOFF = 16
  {0x3811, 0x10}, //HOFF
  {0x3812, 0x00}, //VOFF = 6
  {0x3813, 0x06}, //VOFF
  {0x3814, 0x11}, //X INC
  {0x3815, 0x11}, //Y INC
#if defined (CONFIG_MACH_OKMOK)|| defined (CONFIG_MACH_NUNIVAK)|| defined (CONFIG_MACH_ROUNDTOP)
{0x3820, 0x80}, //Timing Reg20:Vflip
{0x3821, 0x16}, //Timing Reg21:Hmirror
#else
{0x3820, 0x86}, //Timing Reg20:Vflip
{0x3821, 0x10}, //Timing Reg21:Hmirror
#endif
  {0x3f00, 0x02}, //PSRAM Ctrl0
  {0x3f01, 0xfc}, //PSRAM Ctrl1
  {0x3f05, 0x10}, //PSRAM Ctrl5
  {0x4600, 0x04}, //VFIFO Ctrl0
  {0x4601, 0x00}, //VFIFO Read ST High
  {0x4602, 0x20}, //VFIFO Read ST Low
  {0x4837, 0x1e}, //MIPI PCLK PERIOD
  {0x5068, 0x00}, //HSCALE_CTRL
  {0x506a, 0x00}, //VSCALE_CTRL
  {0x5c00, 0x80}, //PBLC CTRL00
  {0x5c01, 0x00}, //PBLC CTRL01
  {0x5c02, 0x00}, //PBLC CTRL02
  {0x5c03, 0x00}, //PBLC CTRL03
  {0x5c04, 0x00}, //PBLC CTRL04
  {0x5c08, 0x10}, //PBLC CTRL08
  {0x6900, 0x61}, //CADC CTRL00
  {0x0100, 0x01}, // wake up
};

struct otp_struct current_wb_otp;
struct otp_struct current_lenc_otp;

int read_sccb16(uint32_t address, struct msm_sensor_ctrl_t *s_ctrl)
{
uint16_t temp, flag;
temp= msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			address, &flag,
			MSM_CAMERA_I2C_BYTE_DATA);
return flag;
}

int write_sccb16(uint32_t address, uint32_t value, struct msm_sensor_ctrl_t *s_ctrl)
{
int rc;


rc= msm_camera_i2c_write(
			s_ctrl->sensor_i2c_client,
			address, value,
			MSM_CAMERA_I2C_BYTE_DATA);
return rc;
}	
// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_wb(uint index, struct msm_sensor_ctrl_t *s_ctrl)
{
uint  flag,i;
uint32_t address;
// select bank 0
write_sccb16(0x3d84, 0x08,s_ctrl);
// load otp to buffer
write_sccb16(0x3d81, 0x01,s_ctrl);
msleep(3);
// read flag
address = 0x3d05 + index*9;
flag = read_sccb16(address,s_ctrl);
// disable otp read
write_sccb16(0x3d81, 0x00,s_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,s_ctrl);
}
if (!flag) {
return 0;
} else if ((!(flag &0x80)) && (flag&0x7f)) {
return 2;
} else {
return 1;
}
}

// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_lenc(uint index, struct msm_sensor_ctrl_t *s_ctrl)
{
uint  flag,i;
uint32_t address;
// select bank: index*2 + 1
write_sccb16(0x3d84, 0x09 + index*2,s_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,s_ctrl);
msleep(3);
address = 0x3d00;
flag = read_sccb16(address,s_ctrl);
flag = flag & 0xc0;
// disable otp read
write_sccb16(0x3d81, 0x00,s_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,s_ctrl);
}
if (!flag) {
return 0;
} else if (
flag == 0x40) {
return 2;
} else {
return 1;
}
}


// index: index of otp group. (0, 1, 2)
// return: 0,
int read_otp_wb(uint index, struct otp_struct *p_otp, struct msm_sensor_ctrl_t *s_ctrl)
{
int i;
uint32_t address;
// select bank 0
write_sccb16(0x3d84, 0x08,s_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,s_ctrl);
msleep(3);
address = 0x3d05 + index*9;
p_otp->module_integrator_id = (read_sccb16(address,s_ctrl) & 0x7f);
p_otp->lens_id = read_sccb16(address + 1,s_ctrl);
p_otp->rg_ratio = read_sccb16(address + 2,s_ctrl);
p_otp->bg_ratio = read_sccb16(address + 3,s_ctrl);
p_otp->user_data[0] = read_sccb16(address + 4,s_ctrl);
p_otp->user_data[1] = read_sccb16(address + 5,s_ctrl);
p_otp->user_data[2] = read_sccb16(address + 6,s_ctrl);
p_otp->user_data[3] = read_sccb16(address + 7,s_ctrl);
p_otp->user_data[4] = read_sccb16(address + 8,s_ctrl);
pr_err("otpwb %s  module_integrator_id =0x%x",__func__,p_otp->module_integrator_id);
pr_err("otpwb %s  lens_id =0x%x",__func__,p_otp->lens_id);
pr_err("otpwb %s  rg_ratio =0x%x",__func__,p_otp->rg_ratio);
pr_err("otpwb %s  bg_ratio =0x%x",__func__,p_otp->bg_ratio);
pr_err("otpwb %s  user_data[0] =0x%x",__func__,p_otp->user_data[0]);
pr_err("otpwb %s  user_data[1] =0x%x",__func__,p_otp->user_data[1]);
pr_err("otpwb %s  user_data[2] =0x%x",__func__,p_otp->user_data[2]);
pr_err("otpwb %s  user_data[3] =0x%x",__func__,p_otp->user_data[3]);
pr_err("otpwb %s  user_data[4] =0x%x",__func__,p_otp->user_data[4]);
// disable otp read
write_sccb16(0x3d81, 0x00,s_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0;i<32;i++) {
write_sccb16(address + i, 0x00,s_ctrl);
}
return 0;
}

// index: index of otp group. (0, 1, 2)
// return: 0
int read_otp_lenc(uint index, struct otp_struct *p_otp, struct msm_sensor_ctrl_t *s_ctrl)
{
uint bank, temp, i;
uint32_t address;
// select bank: index*2 + 1
bank = index*2 + 1;
temp = 0x08 | bank;
write_sccb16(0x3d84, temp,s_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,s_ctrl);
msleep(3);
address = 0x3d01;
for (i=0; i<31; i++) {
p_otp->lenc[i] = read_sccb16(address,s_ctrl);
address++;
}
// disable otp read
write_sccb16(0x3d81, 0x00,s_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,s_ctrl);
}
// select next bank
bank++;
temp = 0x08 | bank;
write_sccb16(0x3d84, temp,s_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,s_ctrl);
msleep(3);
address = 0x3d00;
for (i=31; i<62; i++) {
p_otp->lenc[i] = read_sccb16(address,s_ctrl);
address++;
}
// disable otp read
write_sccb16(0x3d81, 0x00,s_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,s_ctrl);
}
return 0;
}

//R_gain: red gain of sensor AWB, 0x400 = 1
// G_gain: green gain of sensor AWB, 0x400 = 1
// B_gain: blue gain of sensor AWB, 0x400 = 1
// return 0
int update_awb_gain(uint32_t R_gain, uint32_t G_gain, uint32_t B_gain, struct msm_sensor_ctrl_t *s_ctrl)
{
pr_err("otpwb %s  R_gain =%x  0x3400=%d",__func__,R_gain,R_gain>>8);
pr_err("otpwb %s  R_gain =%x  0x3401=%d",__func__,R_gain,R_gain & 0x00ff);
pr_err("otpwb %s  G_gain =%x  0x3402=%d",__func__,G_gain,G_gain>>8);
pr_err("otpwb %s  G_gain =%x  0x3403=%d",__func__,G_gain,G_gain & 0x00ff);
pr_err("otpwb %s  B_gain =%x  0x3404=%d",__func__,B_gain,B_gain>>8);
pr_err("otpwb %s  B_gain =%x  0x3405=%d",__func__,B_gain,B_gain & 0x00ff);

if (R_gain>0x400) {
write_sccb16(0x3400, R_gain>>8,s_ctrl);
write_sccb16(0x3401, R_gain & 0x00ff,s_ctrl);
}
if (G_gain>0x400) {
write_sccb16(0x3402, G_gain>>8,s_ctrl);
write_sccb16(0x3403, G_gain & 0x00ff,s_ctrl);
}
if (B_gain>0x400) {
write_sccb16(0x3404, B_gain>>8,s_ctrl);
write_sccb16(0x3405, B_gain & 0x00ff,s_ctrl);
}
return 0;
}



int update_lenc(struct otp_struct otp, struct msm_sensor_ctrl_t *s_ctrl)
{
		uint i, temp;
		temp = read_sccb16(0x5000,s_ctrl);
		temp |= 0x80;
		write_sccb16(0x5000, temp,s_ctrl);
		for(i=0;i<62;i++) {
		write_sccb16(0x5800+i, otp.lenc[i],s_ctrl);
		}
		 return 0;
}

// R/G and B/G of typical camera module is defined here
uint RG_Ratio_typical ;
uint BG_Ratio_typical ;

// call this function after OV8820 initialization
// return value: 0 update success
// 1, no OTP
int update_otp_wb(struct msm_sensor_ctrl_t *s_ctrl) 
{
uint i, temp, otp_index;

uint32_t R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
// R/G and B/G of current camera module is read out from sensor OTP
// check first wb OTP with valid data
for(i=0;i<3;i++) {
temp = check_otp_wb(i,s_ctrl);
pr_err("otpwb %s  temp =%d  i=%d",__func__,temp,i);
if (temp == 2) {
otp_index = i;
break;
}
}
if (i==3) {
pr_err("otpwb %s  i=%d   no valid wb OTP data ",__func__,i);
return 1;
}
read_otp_wb(otp_index, &current_wb_otp,s_ctrl);

if(current_wb_otp.module_integrator_id==0x02){
//truly
BG_Ratio_typical=0x49;
RG_Ratio_typical=0x4d;
}else if(current_wb_otp.module_integrator_id==0x31){
//mcnex
BG_Ratio_typical=0x53;
RG_Ratio_typical=0x55;
}else{
//qtech
BG_Ratio_typical=0x57;
RG_Ratio_typical=0x5a;
}


//calculate gain
//0x400 = 1x gain
if(current_wb_otp.bg_ratio < BG_Ratio_typical)
{
if (current_wb_otp.rg_ratio < RG_Ratio_typical)
{
// current_otp.bg_ratio < BG_Ratio_typical &&
// current_otp.rg_ratio < RG_Ratio_typical
G_gain = 0x400;
B_gain = 0x400 * BG_Ratio_typical / current_wb_otp.bg_ratio;
R_gain = 0x400 * RG_Ratio_typical / current_wb_otp.rg_ratio;
} else
{
// current_otp.bg_ratio < BG_Ratio_typical &&
// current_otp.rg_ratio >= RG_Ratio_typical
R_gain = 0x400;
G_gain = 0x400 * current_wb_otp.rg_ratio / RG_Ratio_typical;
B_gain = G_gain * BG_Ratio_typical / current_wb_otp.bg_ratio;
}
} else
{
if (current_wb_otp.rg_ratio < RG_Ratio_typical)
{
// current_otp.bg_ratio >= BG_Ratio_typical &&
// current_otp.rg_ratio < RG_Ratio_typical
B_gain = 0x400;
G_gain = 0x400 * current_wb_otp.bg_ratio / BG_Ratio_typical;
R_gain = G_gain * RG_Ratio_typical / current_wb_otp.rg_ratio;
} else
{
// current_otp.bg_ratio >= BG_Ratio_typical &&
// current_otp.rg_ratio >= RG_Ratio_typical
G_gain_B = 0x400 * current_wb_otp.bg_ratio / BG_Ratio_typical;
G_gain_R = 0x400 * current_wb_otp.rg_ratio / RG_Ratio_typical;
if(G_gain_B > G_gain_R )
{
B_gain = 0x400;
G_gain = G_gain_B;
R_gain = G_gain * RG_Ratio_typical / current_wb_otp.rg_ratio;
}
else
{
R_gain = 0x400;
G_gain = G_gain_R;
B_gain = G_gain * BG_Ratio_typical / current_wb_otp.bg_ratio;
}
}
}
// write sensor wb gain to registers
update_awb_gain(R_gain, G_gain, B_gain,s_ctrl);
return 0;
}

// call this function after OV8820 initialization
// return value: 0 update success
// 1, no OTP
int update_otp_lenc(struct msm_sensor_ctrl_t *s_ctrl) 
{

uint i, temp, otp_index;

// check first lens correction OTP with valid data

for(i=0;i<3;i++) {
temp = check_otp_lenc(i,s_ctrl);
if (temp == 2) {
pr_err("otplenc %s  temp =%d  i=%d",__func__,temp,i);
otp_index = i;
break;
}
}
if (i==3) {
// no lens correction data
pr_err("otplenc %s  i=%d   no valid lenc OTP data ",__func__,i);
return 1;
}
read_otp_lenc(otp_index, &current_lenc_otp,s_ctrl);
update_lenc(current_lenc_otp,s_ctrl);
//success
return 0;
}

int msm_camera_config_gpio_table_ov8825(int gpio_en)
{
	int rc = 0;

   if(gpio_en)	 
	rc=gpio_direction_output(54, 1);
   else
   	rc=gpio_direction_output(54, 0);

  if(rc<0)
   pr_err("%s pwd gpio54  direction %d  rc %d\n",__func__,gpio_en,rc);

   return rc;

}

int32_t ov8825_msm_sensor_otp_func(struct msm_sensor_ctrl_t *s_ctrl)
{
       int rc=0;
      
       pr_err("%s: %d\n", __func__, __LINE__);

	update_otp_wb(s_ctrl);//added for wb otp
	pr_err("%s  current_wb_otp id =0x%x\n",__func__,current_wb_otp.module_integrator_id);

	update_otp_lenc(s_ctrl);
	pr_err("%s  current_lenc_otp id =0x%x\n",__func__,current_lenc_otp.module_integrator_id);   

	return rc;
}

int32_t ov8825_msm_sensor_bayer_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0, size = 0, index = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	struct msm_camera_power_seq_t *power_seq = NULL;
	CDBG("%s: %d\n", __func__, __LINE__);
	if (s_ctrl->power_seq) {
		power_seq = s_ctrl->power_seq;
		size = s_ctrl->num_power_seq;
	} else {
		power_seq = &sensor_power_seq[0];
		size = ARRAY_SIZE(sensor_power_seq);
	}

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		pr_err("%s: could not allocate mem for regulators\n",
			__func__);
		return -ENOMEM;
	}

if(!OV8825_IsPowered){
	for (index = 0; index < size; index++) {
		switch (power_seq[index].power_config) {
		case REQUEST_GPIO:
			rc = msm_camera_request_gpio_table(data, 1);
			if (rc < 0) {
				pr_err("%s: request gpio failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case REQUEST_VREG:
		  	pr_err("%s: REQUEST_VREG\n", __func__);
			rc = msm_camera_config_vreg(
				&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 1);
			if (rc < 0) {
				pr_err("%s: regulator on failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case ENABLE_VREG:
		   	pr_err("%s: ENABLE_VREG\n", __func__);
			rc = msm_camera_enable_vreg(
				&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 1);
			if (rc < 0) {
				pr_err("%s: enable regulator failed\n",
					__func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);

			break;
		case ENABLE_GPIO:
			rc = msm_camera_config_gpio_table(data, 1);

			if (rc < 0) {
				pr_err("%s: config gpio failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
		   	break;
		case CONFIG_CLK:
			if (s_ctrl->clk_rate != 0)
				cam_clk_info->clk_rate = s_ctrl->clk_rate;

			rc = msm_cam_clk_enable(
				&s_ctrl->sensor_i2c_client->client->dev,
				cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 1);
			if (rc < 0) {
				pr_err("%s: clk enable failed\n", __func__);
				goto ERROR;
			}
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case CONFIG_EXT_POWER_CTRL:
			if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(1);
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		case CONFIG_I2C_MUX:
			if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_enable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
			if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);
			break;
		default:
			pr_err("%s error power config %d\n", __func__,
				power_seq[index].power_config);
			rc = -EINVAL;
			break;
		}
	}
	OV8825_IsPowered=1;
}else{
	if (data->sensor_platform_info->ext_power_ctrl != NULL)
		data->sensor_platform_info->ext_power_ctrl(1);
		if (power_seq[index].delay)
				usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);

	if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
	msm_sensor_bayer_enable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
	if (power_seq[index].delay)
		usleep_range(power_seq[index].delay * 1000,
					(power_seq[index].delay * 1000) + 1000);	


	if (s_ctrl->clk_rate != 0)
				cam_clk_info->clk_rate = s_ctrl->clk_rate;

       rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->
				dev, cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 1);


	 if (rc < 0) 
		pr_err("%s: clk enable failed\n", __func__);

	 mdelay(1);	 

        msm_camera_config_gpio_table_ov8825(1);
	 mdelay(10);

	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3018, 0x00,
		      MSM_CAMERA_I2C_BYTE_DATA);
       if (rc < 0) 
	pr_err("%s: 0x3018 rc=%d\n", __func__,rc);

}

	return rc;

ERROR:
	for (index--; index >= 0; index--) {
		switch (power_seq[index].power_config) {
		case CONFIG_I2C_MUX:
			if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_disable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
			break;
		case CONFIG_EXT_POWER_CTRL:
			if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(0);
			break;
		case CONFIG_CLK:
			msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->
				dev, cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 0);
			break;
		case ENABLE_GPIO:
			msm_camera_config_gpio_table(data, 0);
			break;
		case ENABLE_VREG:
			msm_camera_enable_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);

			if(OV8825_IsPowered==1)
				OV8825_IsPowered=0;
			
			break;
		case REQUEST_VREG:
			msm_camera_config_vreg(&s_ctrl->sensor_i2c_client->
				client->dev,
				s_ctrl->sensordata->sensor_platform_info->
				cam_vreg,
				s_ctrl->sensordata->sensor_platform_info->
				num_vreg,
				s_ctrl->vreg_seq,
				s_ctrl->num_vreg_seq,
				s_ctrl->reg_ptr, 0);
			break;
		case REQUEST_GPIO:
			msm_camera_request_gpio_table(data, 0);
			break;
		default:
			pr_err("%s error power config %d\n", __func__,
				power_seq[index].power_config);
			break;
		}
	}
	kfree(s_ctrl->reg_ptr);
	return rc;
}

int32_t ov8825_msm_sensor_bayer_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t size = 0;
	struct msm_camera_power_seq_t *power_seq = NULL;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	int rc=0;
	CDBG("%s\n", __func__);
	
	if (s_ctrl->power_seq) {
		power_seq = s_ctrl->power_seq;
		size = s_ctrl->num_power_seq;
	} else {
		power_seq = &sensor_power_seq[0];
		size = ARRAY_SIZE(sensor_power_seq);
	}

	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3018, 0x18,
		      MSM_CAMERA_I2C_BYTE_DATA);

      if (rc < 0)
	  pr_err("%s: 0x3018 rc=%d\n", __func__,rc);

       msm_camera_config_gpio_table_ov8825(0);
	usleep_range(1000, 2000);	   	

	msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->
				dev, cam_clk_info, s_ctrl->cam_clk,
				ARRAY_SIZE(cam_clk_info), 0);

	if (data->sensor_platform_info->ext_power_ctrl != NULL)
				data->sensor_platform_info->ext_power_ctrl(0);

	if (data->sensor_platform_info->i2c_conf &&
				data->sensor_platform_info->i2c_conf->
				use_i2c_mux)
				msm_sensor_bayer_disable_i2c_mux(
					data->sensor_platform_info->i2c_conf);
	 
	kfree(s_ctrl->reg_ptr);
	return 0;
}

int32_t ov8825_msm_set_init_code_probe(struct msm_sensor_ctrl_t *s_ctrl)
{
     int32_t i=0;
     int rc=0;	 

     for(i=0;i<(sizeof(ov8825_sensor_reset_settings) / sizeof((ov8825_sensor_reset_settings)[0]));i++){	
	 	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client,ov8825_sensor_reset_settings[i].reg_addr, ov8825_sensor_reset_settings[i].reg_data,
		      MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0)
			pr_err("%s set external power err for leakage current address = 0x%x, value =0x%x\n", __func__, ov8825_sensor_reset_settings[i].reg_addr,ov8825_sensor_reset_settings[i].reg_data);
     	}	 

     msleep(50);
   
     for(i=0;i<(sizeof(ov8825_sensor_init_settings) / sizeof((ov8825_sensor_init_settings)[0]));i++){	
	 	rc=msm_camera_i2c_write(s_ctrl->sensor_i2c_client,ov8825_sensor_init_settings[i].reg_addr, ov8825_sensor_init_settings[i].reg_data,
		      MSM_CAMERA_I2C_BYTE_DATA);
		if(rc<0)
			pr_err("%s set external power err for leakage current address = 0x%x, value =0x%x\n", __func__, ov8825_sensor_init_settings[i].reg_addr,ov8825_sensor_init_settings[i].reg_data);
     	}
     return rc;
	 
}

int32_t ov8825_msm_sensor_bayer_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	CDBG("%s %s_i2c_probe called\n", __func__, client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s %s i2c_check_functionality failed\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
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

	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s %s power up failed\n", __func__, client->name);
		return rc;
	}

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_bayer_match_id(s_ctrl);
	if (rc < 0)
		goto probe_fail;

	rc=ov8825_msm_set_init_code_probe(s_ctrl);
	if (rc < 0)
		pr_err("%s %s set external power err for leakage current\n", __func__, client->name);
	
	//add by wxl for opt issue
    msleep(10);
	rc=ov8825_msm_sensor_otp_func(s_ctrl);

#ifdef CONFIG_SENSOR_INFO 
    	msm_sensorinfo_set_back_sensor_id(s_ctrl->sensor_id_info->sensor_id);
#else
  //do nothing here
#endif	

	if (!s_ctrl->wait_num_frames)
		s_ctrl->wait_num_frames = 1;

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
	return rc;
}

#endif

int32_t msm_sensor_bayer_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	CDBG("%s %s_i2c_probe called\n", __func__, client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s %s i2c_check_functionality failed\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
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

#if defined (CONFIG_MACH_ROUNDTOP) || defined (CONFIG_MACH_OKMOK)
	s_ctrl->reg_ptr_bug = kzalloc(sizeof(struct regulator *)
			* s_ctrl->sensordata->sensor_platform_info->num_bug_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr_bug) {
		pr_err("%s: could not allocate mem for reg_ptr_bug\n",
			__func__);
		return -ENOMEM;
	}

	rc = msm_camera_config_bug_vreg(&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->cam_bug_vreg,
				s_ctrl->sensordata->sensor_platform_info->num_bug_vreg,
				s_ctrl->reg_ptr_bug, 1);
		if (rc < 0) {
			pr_err("%s: regulator on failed\n", __func__);
			return rc;
		}
        rc = msm_camera_enable_bug_vreg(&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->cam_bug_vreg,
				s_ctrl->sensordata->sensor_platform_info->num_bug_vreg,
				s_ctrl->reg_ptr_bug, 1);
		if (rc < 0) {
			pr_err("%s: enable regulator failed\n", __func__);
			return rc;
		}
	
#endif //jzq add for fix can not probe 8825 bug

	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s %s power up failed\n", __func__, client->name);
		return rc;
	}

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_bayer_match_id(s_ctrl);
	if (rc < 0)
		goto probe_fail;

#ifdef CONFIG_SENSOR_INFO 
    #if defined (CONFIG_OV8825) || defined (CONFIG_IMX091)
    	msm_sensorinfo_set_back_sensor_id(s_ctrl->sensor_id_info->sensor_id);
    #endif
#else
  //do nothing here
#endif	

	if (!s_ctrl->wait_num_frames)
		s_ctrl->wait_num_frames = 1;

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
	
//modified by zhang.yu_1 for support eeprom to imx091 sensor
#if defined(CONFIG_IMX091)
	msm_sensor_bayer_eeprom_read(s_ctrl);
#endif
	msm_sensor_enable_debugfs(s_ctrl);
	goto power_down;
probe_fail:
	pr_err("%s %s_i2c_probe failed\n", __func__, client->name);
power_down:
	if (rc > 0)
		rc = 0;
	 s_ctrl->func_tbl->sensor_power_down(s_ctrl);

#if defined (CONFIG_MACH_ROUNDTOP) || defined (CONFIG_MACH_OKMOK)
        rc = msm_camera_enable_bug_vreg(&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->cam_bug_vreg,
				s_ctrl->sensordata->sensor_platform_info->num_bug_vreg,
				s_ctrl->reg_ptr_bug, 0);
		if (rc < 0) {
			pr_err("%s: disable regulator failed\n", __func__);
			return rc;
		}

	rc = msm_camera_config_bug_vreg(&s_ctrl->sensor_i2c_client->client->dev,
				s_ctrl->sensordata->sensor_platform_info->cam_bug_vreg,
				s_ctrl->sensordata->sensor_platform_info->num_bug_vreg,
				s_ctrl->reg_ptr_bug, 0);
		if (rc < 0) {
			pr_err("%s: regulator on failed\n", __func__);
			return rc;
		}	

	kfree(s_ctrl->reg_ptr_bug);	
#endif //jzq add for fix can not probe 8825 bug
	 
	return rc;
}

int32_t msm_sensor_delay_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;
	CDBG("%s %s_delay_i2c_probe called\n", __func__, client->name);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s %s i2c_check_functionality failed\n",
			__func__, client->name);
		rc = -EFAULT;
		return rc;
	}

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
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

	if (!s_ctrl->wait_num_frames)
		s_ctrl->wait_num_frames = 1;

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
	if (rc > 0)
		rc = 0;
	return rc;
}

int32_t msm_sensor_bayer_power(struct v4l2_subdev *sd, int on)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	mutex_lock(s_ctrl->msm_sensor_mutex);
	if (!on)
		rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	mutex_unlock(s_ctrl->msm_sensor_mutex);
	return rc;
}

int32_t msm_sensor_bayer_v4l2_enum_fmt(struct v4l2_subdev *sd,
	unsigned int index, enum v4l2_mbus_pixelcode *code)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);

	if ((unsigned int)index >= s_ctrl->sensor_v4l2_subdev_info_size)
		return -EINVAL;

	*code = s_ctrl->sensor_v4l2_subdev_info[index].code;
	return 0;
}

int32_t msm_sensor_bayer_v4l2_s_ctrl(struct v4l2_subdev *sd,
	struct v4l2_control *ctrl)
{
	int rc = -1, i = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	struct msm_sensor_v4l2_ctrl_info_t *v4l2_ctrl =
		s_ctrl->msm_sensor_v4l2_ctrl_info;

	CDBG("%s\n", __func__);
	CDBG("%d\n", ctrl->id);
	if (v4l2_ctrl == NULL)
		return rc;
	for (i = 0; i < s_ctrl->num_v4l2_ctrl; i++) {
		if (v4l2_ctrl[i].ctrl_id == ctrl->id) {
			if (v4l2_ctrl[i].s_v4l2_ctrl != NULL) {
				CDBG("\n calling msm_sensor_s_ctrl_by_enum\n");
				rc = v4l2_ctrl[i].s_v4l2_ctrl(
					s_ctrl,
					&s_ctrl->msm_sensor_v4l2_ctrl_info[i],
					ctrl->value);
			}
			break;
		}
	}

	return rc;
}

int32_t msm_sensor_bayer_v4l2_query_ctrl(
	struct v4l2_subdev *sd, struct v4l2_queryctrl *qctrl)
{
	int rc = -1, i = 0;
	struct msm_sensor_ctrl_t *s_ctrl =
		(struct msm_sensor_ctrl_t *) sd->dev_priv;

	CDBG("%s\n", __func__);
	CDBG("%s id: %d\n", __func__, qctrl->id);

	if (s_ctrl->msm_sensor_v4l2_ctrl_info == NULL)
		return rc;

	for (i = 0; i < s_ctrl->num_v4l2_ctrl; i++) {
		if (s_ctrl->msm_sensor_v4l2_ctrl_info[i].ctrl_id == qctrl->id) {
			qctrl->minimum =
				s_ctrl->msm_sensor_v4l2_ctrl_info[i].min;
			qctrl->maximum =
				s_ctrl->msm_sensor_v4l2_ctrl_info[i].max;
			qctrl->flags = 1;
			rc = 0;
			break;
		}
	}

	return rc;
}

int msm_sensor_bayer_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	CDBG("%s enter\n", __func__);
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	return rc;
}



