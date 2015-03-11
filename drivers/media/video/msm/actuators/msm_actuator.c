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

#include <linux/module.h>
#include "msm_actuator.h"

#include "../sensors/msm_sensor_common.h"

static struct msm_actuator_ctrl_t msm_actuator_t;
static struct msm_actuator msm_vcm_actuator_table;
static struct msm_actuator msm_piezo_actuator_table;

static struct msm_actuator *actuators[] = {
	&msm_vcm_actuator_table,
	&msm_piezo_actuator_table,
};

//add by wxl for p896a25
#if defined CONFIG_OV8825
#define OV8825_AF_MSB       0x3619//0x30EC
#define OV8825_AF_LSB       0x3618//0x30ED
extern struct otp_struct current_wb_otp;
#endif

static int32_t msm_actuator_piezo_set_default_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_move_params_t *move_params)
{
	int32_t rc = 0;

	if (a_ctrl->curr_step_pos != 0) {
		a_ctrl->i2c_tbl_index = 0;
		rc = a_ctrl->func_tbl->actuator_parse_i2c_params(a_ctrl,
			a_ctrl->initial_code, 0, 0);
		rc = a_ctrl->func_tbl->actuator_parse_i2c_params(a_ctrl,
			a_ctrl->initial_code, 0, 0);
		rc = msm_camera_i2c_write_table_w_microdelay(
			&a_ctrl->i2c_client, a_ctrl->i2c_reg_tbl,
			a_ctrl->i2c_tbl_index, a_ctrl->i2c_data_type);
		if (rc < 0) {
			pr_err("%s: i2c write error:%d\n",
				__func__, rc);
			return rc;
		}
		a_ctrl->i2c_tbl_index = 0;
		a_ctrl->curr_step_pos = 0;
	}
	return rc;
}

static int32_t msm_actuator_parse_i2c_params(struct msm_actuator_ctrl_t *a_ctrl,
	int16_t next_lens_position, uint32_t hw_params, uint16_t delay)
{
	struct msm_actuator_reg_params_t *write_arr = a_ctrl->reg_tbl;
	uint32_t hw_dword = hw_params;
	uint16_t i2c_byte1 = 0, i2c_byte2 = 0;
	uint16_t value = 0;
	uint32_t size = a_ctrl->reg_tbl_size, i = 0;
	int32_t rc = 0;
	struct msm_camera_i2c_reg_tbl *i2c_tbl = a_ctrl->i2c_reg_tbl;
	uint8_t hw_reg_write = 1;
	CDBG("%s: IN\n", __func__);
	if (a_ctrl->curr_hwparams == hw_params)
		hw_reg_write = 0;
	for (i = 0; i < size; i++) {
		if (write_arr[i].reg_write_type == MSM_ACTUATOR_WRITE_DAC) {
			value = (next_lens_position <<
				write_arr[i].data_shift) |
				((hw_dword & write_arr[i].hw_mask) >>
				write_arr[i].hw_shift);

			if (write_arr[i].reg_addr != 0xFFFF) {
				i2c_byte1 = write_arr[i].reg_addr;
				i2c_byte2 = value;
				if (size != (i+1)) {
					i2c_byte2 = value & 0xFF;
					CDBG("%s: byte1:0x%x, byte2:0x%x\n",
					__func__, i2c_byte1, i2c_byte2);
					i2c_tbl[a_ctrl->i2c_tbl_index].
						reg_addr = i2c_byte1;
					i2c_tbl[a_ctrl->i2c_tbl_index].
						reg_data = i2c_byte2;
					i2c_tbl[a_ctrl->i2c_tbl_index].
						delay = 0;
					a_ctrl->i2c_tbl_index++;
					i++;
					i2c_byte1 = write_arr[i].reg_addr;
					i2c_byte2 = (value & 0xFF00) >> 8;
				}
			} else {
				i2c_byte1 = (value & 0xFF00) >> 8;
				i2c_byte2 = value & 0xFF;
			}
			CDBG("%s: i2c_byte1:0x%x, i2c_byte2:0x%x\n", __func__,
				i2c_byte1, i2c_byte2);
			i2c_tbl[a_ctrl->i2c_tbl_index].reg_addr = i2c_byte1;
			i2c_tbl[a_ctrl->i2c_tbl_index].reg_data = i2c_byte2;
			i2c_tbl[a_ctrl->i2c_tbl_index].delay = delay;
			a_ctrl->i2c_tbl_index++;
		} else {
			if (hw_reg_write) {
				i2c_byte1 = write_arr[i].reg_addr;
				i2c_byte2 = (hw_dword & write_arr[i].hw_mask) >>
					write_arr[i].hw_shift;
				CDBG("%s: i2c_byte1:0x%x, i2c_byte2:0x%x\n", __func__,
					i2c_byte1, i2c_byte2);
				i2c_tbl[a_ctrl->i2c_tbl_index].reg_addr = i2c_byte1;
				i2c_tbl[a_ctrl->i2c_tbl_index].reg_data = i2c_byte2;
				i2c_tbl[a_ctrl->i2c_tbl_index].delay = delay;
				a_ctrl->i2c_tbl_index++;
			}
		}
	}
	CDBG("%s: OUT\n", __func__);
	if (rc == 0)
		a_ctrl->curr_hwparams = hw_params;
	return rc;
}

static int32_t msm_actuator_init_focus(struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t size, enum msm_actuator_data_type type,
	struct reg_settings_t *settings)
{
	int32_t rc = -EFAULT;
	int32_t i = 0;
	CDBG("%s called\n", __func__);

	for (i = 0; i < size; i++) {
		switch (type) {
		case MSM_ACTUATOR_BYTE_DATA:
			rc = msm_camera_i2c_write(
				&a_ctrl->i2c_client,
				settings[i].reg_addr,
				settings[i].reg_data, MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case MSM_ACTUATOR_WORD_DATA:
			rc = msm_camera_i2c_write(
				&a_ctrl->i2c_client,
				settings[i].reg_addr,
				settings[i].reg_data, MSM_CAMERA_I2C_WORD_DATA);
			break;
		default:
			pr_err("%s: Unsupport data type: %d\n",
				__func__, type);
			break;
		}
		if (rc < 0)
			break;
	}

	a_ctrl->curr_step_pos = 0;
	CDBG("%s Exit:%d\n", __func__, rc);
	return rc;
}


//add by wxl for p896a25
#if defined CONFIG_OV8825
static int32_t msm_actuator_write_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t curr_lens_pos,
	struct damping_params_t *damping_params,
	int8_t sign_direction,
	int16_t code_boundary)
{
   	uint8_t msb = 0, lsb = 0;
	uint8_t S3_to_0 = 0x1; 
	int32_t rc = 0;
	
	msb = code_boundary >> 4;
	lsb =((code_boundary & 0x000F) << 4) | S3_to_0;
	pr_err("cdznew::%s: Actuator next_lens_position =%d \n", __func__, code_boundary);
	pr_err("cdznew::%s: Actuator MSB:0x%x, LSB:0x%x\n", __func__, msb, lsb);
	

	rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
	           OV8825_AF_LSB, 
			   lsb, 
			   MSM_CAMERA_I2C_BYTE_DATA);
	
	rc = msm_camera_i2c_write(&a_ctrl->i2c_client,
	           OV8825_AF_MSB, 
			   msb, 
			   MSM_CAMERA_I2C_BYTE_DATA);
	
	return rc;
}


#else

static int32_t msm_actuator_write_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	uint16_t curr_lens_pos,
	struct damping_params_t *damping_params,
	int8_t sign_direction,
	int16_t code_boundary)
{
	int32_t rc = 0;
	int16_t next_lens_pos = 0;
	uint16_t damping_code_step = 0;
	uint16_t wait_time = 0;

	damping_code_step = damping_params->damping_step;
	wait_time = damping_params->damping_delay;

	/* Write code based on damping_code_step in a loop */
	for (next_lens_pos =
		curr_lens_pos + (sign_direction * damping_code_step);
		(sign_direction * next_lens_pos) <=
			(sign_direction * code_boundary);
		next_lens_pos =
			(next_lens_pos +
				(sign_direction * damping_code_step))) {
		rc = a_ctrl->func_tbl->
			actuator_parse_i2c_params(a_ctrl, next_lens_pos,
				damping_params->hw_params, wait_time);
		if (rc < 0) {
			pr_err("%s: error:%d\n",
				__func__, rc);
			return rc;
		}
		curr_lens_pos = next_lens_pos;
	}

	if (curr_lens_pos != code_boundary) {
		rc = a_ctrl->func_tbl->
			actuator_parse_i2c_params(a_ctrl, code_boundary,
				damping_params->hw_params, wait_time);
	}
	return rc;
}

#endif

static int32_t msm_actuator_piezo_move_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_move_params_t *move_params)
{
	int32_t dest_step_position = move_params->dest_step_pos;
	int32_t rc = 0;
	int32_t num_steps = move_params->num_steps;

	if (num_steps == 0)
		return rc;

	a_ctrl->i2c_tbl_index = 0;
	rc = a_ctrl->func_tbl->
		actuator_parse_i2c_params(a_ctrl,
		(num_steps *
		a_ctrl->region_params[0].code_per_step),
		move_params->ringing_params[0].hw_params, 0);

	rc = msm_camera_i2c_write_table_w_microdelay(&a_ctrl->i2c_client,
		a_ctrl->i2c_reg_tbl, a_ctrl->i2c_tbl_index,
		a_ctrl->i2c_data_type);
	if (rc < 0) {
		pr_err("%s: i2c write error:%d\n",
			__func__, rc);
		return rc;
	}
	a_ctrl->i2c_tbl_index = 0;
	a_ctrl->curr_step_pos = dest_step_position;
	return rc;
}

static int32_t msm_actuator_move_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_move_params_t *move_params)
{
	int32_t rc = 0;
	int8_t sign_dir = move_params->sign_dir;
	uint16_t step_boundary = 0;
	uint16_t target_step_pos = 0;
	uint16_t target_lens_pos = 0;
	int16_t dest_step_pos = move_params->dest_step_pos;
	uint16_t curr_lens_pos = 0;
	int dir = move_params->dir;
	int32_t num_steps = move_params->num_steps;

	CDBG("%s called, dir %d, num_steps %d\n",
		__func__,
		dir,
		num_steps);

	if (dest_step_pos == a_ctrl->curr_step_pos)
		return rc;

	curr_lens_pos = a_ctrl->step_position_table[a_ctrl->curr_step_pos];
	a_ctrl->i2c_tbl_index = 0;
	CDBG("curr_step_pos =%d dest_step_pos =%d curr_lens_pos=%d\n",
		a_ctrl->curr_step_pos, dest_step_pos, curr_lens_pos);

	while (a_ctrl->curr_step_pos != dest_step_pos) {
		step_boundary =
			a_ctrl->region_params[a_ctrl->curr_region_index].
			step_bound[dir];
		if ((dest_step_pos * sign_dir) <=
			(step_boundary * sign_dir)) {

			target_step_pos = dest_step_pos;
			target_lens_pos =
				a_ctrl->step_position_table[target_step_pos];
			rc = a_ctrl->func_tbl->
				actuator_write_focus(
					a_ctrl,
					curr_lens_pos,
					&(move_params->
						ringing_params[a_ctrl->
						curr_region_index]),
					sign_dir,
					target_lens_pos);
			if (rc < 0) {
				pr_err("%s: error:%d\n",
					__func__, rc);
				return rc;
			}
			curr_lens_pos = target_lens_pos;

		} else {
			target_step_pos = step_boundary;
			target_lens_pos =
				a_ctrl->step_position_table[target_step_pos];
			rc = a_ctrl->func_tbl->
				actuator_write_focus(
					a_ctrl,
					curr_lens_pos,
					&(move_params->
						ringing_params[a_ctrl->
						curr_region_index]),
					sign_dir,
					target_lens_pos);
			if (rc < 0) {
				pr_err("%s: error:%d\n",
					__func__, rc);
				return rc;
			}
			curr_lens_pos = target_lens_pos;

			a_ctrl->curr_region_index += sign_dir;
		}
		a_ctrl->curr_step_pos = target_step_pos;
	}

//add by wxl for p896a25
#ifndef CONFIG_OV8825
	rc = msm_camera_i2c_write_table_w_microdelay(&a_ctrl->i2c_client,
		a_ctrl->i2c_reg_tbl, a_ctrl->i2c_tbl_index,
		a_ctrl->i2c_data_type);
	if (rc < 0) {
		pr_err("%s: i2c write error:%d\n",
			__func__, rc);
		return rc;
	}
	a_ctrl->i2c_tbl_index = 0;
#endif

	return rc;
}

#ifdef CONFIG_OV8825
// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_af(uint index, struct msm_actuator_ctrl_t *a_ctrl)
{
		uint  i;
		uint32_t address;
		uint16_t flag;

		// select bank 7
		msm_camera_i2c_write(&a_ctrl->i2c_client,
			           0x3d84, 
				    0x0f, 
				    MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(&a_ctrl->i2c_client,
			           0x3d81, 
				    0x01, 
				    MSM_CAMERA_I2C_BYTE_DATA);
		msleep(3);

		// read flag
		address = 0x3d00 + index*10;

		msm_camera_i2c_read(&a_ctrl->i2c_client,
			           address, 
				    &flag, 
				    MSM_CAMERA_I2C_BYTE_DATA);

		// disable otp read
		msm_camera_i2c_write(&a_ctrl->i2c_client,
			           0x3d81, 
				    0x00, 
				    MSM_CAMERA_I2C_BYTE_DATA);

		// clear otp buffer
		address = 0x3d00;
		for (i=0; i<32;i++) {
		msm_camera_i2c_write(&a_ctrl->i2c_client,
			           address + i, 
				    0x00, 
				    MSM_CAMERA_I2C_BYTE_DATA);	
		}
		pr_err("otpaf %s  flag =%x  ",__func__,flag);
		if (!flag) {
		return 0;
		} else if ((!(flag &0x80)) && (flag&0x7f)) {
		return 2;
		} else {
		return 1;
		}
}

uint16_t inf_value=0,mac_value=0;
// index: index of otp group. (0, 1, 2)
// return: 0,
int read_otp_af(uint index,struct msm_actuator_ctrl_t *a_ctrl)
{
uint32_t address1,address2;
// select bank 7
msm_camera_i2c_write(&a_ctrl->i2c_client,
			           0x3d84, 
				    0x0f, 
				    MSM_CAMERA_I2C_BYTE_DATA);
msm_camera_i2c_write(&a_ctrl->i2c_client,
			           0x3d81, 
				    0x01, 
				    MSM_CAMERA_I2C_BYTE_DATA);
msleep(3);
#if 0
address1 = 0x3d02 + index*10;
address2 = 0x3d06 + index*10;
#endif
pr_err("otpaf %s  module_integrator_id 0x%x",__func__,current_wb_otp.module_integrator_id);
if(current_wb_otp.module_integrator_id==0x31){
	//mcnex
	address1 = 0x3d02 + index*10;
	address2 = 0x3d06 + index*10;
	pr_err("%s af mcnex",__func__);
	}else{
	//qtech
	address1 = 0x3d04 + index*10;
	address2 = 0x3d08 + index*10;
	pr_err("%s af qtech",__func__);
}
	
msm_camera_i2c_read(&a_ctrl->i2c_client,
			           address1, 
				    &inf_value, 
				    MSM_CAMERA_I2C_BYTE_DATA);
msm_camera_i2c_read(&a_ctrl->i2c_client,
			           address2, 
				    &mac_value, 
				    MSM_CAMERA_I2C_BYTE_DATA);
return 0;
}

int update_otp_af(struct msm_actuator_ctrl_t *a_ctrl) 
{
		uint i, temp, otp_index;

		// check first af OTP with valid data
		for(i=0;i<3;i++) {
		temp = check_otp_af(i,a_ctrl);
		pr_err("otpaf %s  temp =%d  i=%d",__func__,temp,i);
		if (temp == 2) {
		otp_index = i;
		break;
		}
		}
		if (i==3) {
		pr_err("otpaf %s  i=%d   no valid af OTP data ",__func__,i);
		return 1;
		}

		read_otp_af(otp_index, a_ctrl);

		return 0;
}

#if 0
int read_sccb16(uint32_t address, struct msm_actuator_ctrl_t *a_ctrl)
{
uint16_t temp, flag;
temp= msm_camera_i2c_read(
			&a_ctrl->i2c_client,
			address, &flag,
			MSM_CAMERA_I2C_BYTE_DATA);
return flag;
}

int write_sccb16(uint32_t address, uint32_t value, struct msm_actuator_ctrl_t *a_ctrl)
{
int rc;


rc= msm_camera_i2c_write(
			&a_ctrl->i2c_client,
			address, value,
			MSM_CAMERA_I2C_BYTE_DATA);
return rc;
}	
// index: index of otp group. (0, 1, 2)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
int check_otp_wb(uint index, struct msm_actuator_ctrl_t *a_ctrl)
{
uint  flag,i;
uint32_t address;
// select bank 0
write_sccb16(0x3d84, 0x08,a_ctrl);
// load otp to buffer
write_sccb16(0x3d81, 0x01,a_ctrl);
msleep(3);
// read flag
address = 0x3d05 + index*9;
flag = read_sccb16(address,a_ctrl);
// disable otp read
write_sccb16(0x3d81, 0x00,a_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,a_ctrl);
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
int check_otp_lenc(uint index, struct msm_actuator_ctrl_t *a_ctrl)
{
uint  flag,i;
uint32_t address;
// select bank: index*2 + 1
write_sccb16(0x3d84, 0x09 + index*2,a_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,a_ctrl);
msleep(3);
address = 0x3d00;
flag = read_sccb16(address,a_ctrl);
flag = flag & 0xc0;
// disable otp read
write_sccb16(0x3d81, 0x00,a_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,a_ctrl);
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
int read_otp_wb(uint index, struct otp_struct *p_otp, struct msm_actuator_ctrl_t *a_ctrl)
{
int i;
uint32_t address;
// select bank 0
write_sccb16(0x3d84, 0x08,a_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,a_ctrl);
msleep(3);
address = 0x3d05 + index*9;
p_otp->module_integrator_id = (read_sccb16(address,a_ctrl) & 0x7f);
p_otp->lens_id = read_sccb16(address + 1,a_ctrl);
p_otp->rg_ratio = read_sccb16(address + 2,a_ctrl);
p_otp->bg_ratio = read_sccb16(address + 3,a_ctrl);
p_otp->user_data[0] = read_sccb16(address + 4,a_ctrl);
p_otp->user_data[1] = read_sccb16(address + 5,a_ctrl);
p_otp->user_data[2] = read_sccb16(address + 6,a_ctrl);
p_otp->user_data[3] = read_sccb16(address + 7,a_ctrl);
p_otp->user_data[4] = read_sccb16(address + 8,a_ctrl);
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
write_sccb16(0x3d81, 0x00,a_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0;i<32;i++) {
write_sccb16(address + i, 0x00,a_ctrl);
}
return 0;
}

// index: index of otp group. (0, 1, 2)
// return: 0
int read_otp_lenc(uint index, struct otp_struct *p_otp, struct msm_actuator_ctrl_t *a_ctrl)
{
uint bank, temp, i;
uint32_t address;
// select bank: index*2 + 1
bank = index*2 + 1;
temp = 0x08 | bank;
write_sccb16(0x3d84, temp,a_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,a_ctrl);
msleep(3);
address = 0x3d01;
for (i=0; i<31; i++) {
p_otp->lenc[i] = read_sccb16(address,a_ctrl);
address++;
}
// disable otp read
write_sccb16(0x3d81, 0x00,a_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,a_ctrl);
}
// select next bank
bank++;
temp = 0x08 | bank;
write_sccb16(0x3d84, temp,a_ctrl);
// read otp
write_sccb16(0x3d81, 0x01,a_ctrl);
msleep(3);
address = 0x3d00;
for (i=31; i<62; i++) {
p_otp->lenc[i] = read_sccb16(address,a_ctrl);
address++;
}
// disable otp read
write_sccb16(0x3d81, 0x00,a_ctrl);
// clear otp buffer
address = 0x3d00;
for (i=0; i<32;i++) {
write_sccb16(address + i, 0x00,a_ctrl);
}
return 0;
}

//R_gain: red gain of sensor AWB, 0x400 = 1
// G_gain: green gain of sensor AWB, 0x400 = 1
// B_gain: blue gain of sensor AWB, 0x400 = 1
// return 0
int update_awb_gain(uint32_t R_gain, uint32_t G_gain, uint32_t B_gain, struct msm_actuator_ctrl_t *a_ctrl)
{
pr_err("otpwb %s  R_gain =%x  0x3400=%d",__func__,R_gain,R_gain>>8);
pr_err("otpwb %s  R_gain =%x  0x3401=%d",__func__,R_gain,R_gain & 0x00ff);
pr_err("otpwb %s  G_gain =%x  0x3402=%d",__func__,G_gain,G_gain>>8);
pr_err("otpwb %s  G_gain =%x  0x3403=%d",__func__,G_gain,G_gain & 0x00ff);
pr_err("otpwb %s  B_gain =%x  0x3404=%d",__func__,B_gain,B_gain>>8);
pr_err("otpwb %s  B_gain =%x  0x3405=%d",__func__,B_gain,B_gain & 0x00ff);

if (R_gain>0x400) {
write_sccb16(0x3400, R_gain>>8,a_ctrl);
write_sccb16(0x3401, R_gain & 0x00ff,a_ctrl);
}
if (G_gain>0x400) {
write_sccb16(0x3402, G_gain>>8,a_ctrl);
write_sccb16(0x3403, G_gain & 0x00ff,a_ctrl);
}
if (B_gain>0x400) {
write_sccb16(0x3404, B_gain>>8,a_ctrl);
write_sccb16(0x3405, B_gain & 0x00ff,a_ctrl);
}
return 0;
}



int update_lenc(struct otp_struct otp, struct msm_actuator_ctrl_t *a_ctrl)
{
		uint i, temp;
		temp = read_sccb16(0x5000,a_ctrl);
		temp |= 0x80;
		write_sccb16(0x5000, temp,a_ctrl);
		for(i=0;i<62;i++) {
		write_sccb16(0x5800+i, otp.lenc[i],a_ctrl);
		}
		 return 0;
}

// R/G and B/G of typical camera module is defined here
uint RG_Ratio_typical ;
uint BG_Ratio_typical ;

// call this function after OV8820 initialization
// return value: 0 update success
// 1, no OTP
int update_otp_wb(struct msm_actuator_ctrl_t *a_ctrl) 
{
uint i, temp, otp_index;

uint32_t R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
// R/G and B/G of current camera module is read out from sensor OTP
// check first wb OTP with valid data
for(i=0;i<3;i++) {
temp = check_otp_wb(i,a_ctrl);
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
read_otp_wb(otp_index, &current_wb_otp,a_ctrl);

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
update_awb_gain(R_gain, G_gain, B_gain,a_ctrl);
return 0;
}

// call this function after OV8820 initialization
// return value: 0 update success
// 1, no OTP
int update_otp_lenc(struct msm_actuator_ctrl_t *a_ctrl) 
{

uint i, temp, otp_index;

// check first lens correction OTP with valid data

for(i=0;i<3;i++) {
temp = check_otp_lenc(i,a_ctrl);
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
read_otp_lenc(otp_index, &current_lenc_otp,a_ctrl);
update_lenc(current_lenc_otp,a_ctrl);
//success
return 0;
}
#endif

static int32_t ov8825_msm_actuator_init_step_table(struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_set_info_t *set_info)
{
	int16_t code_per_step = 0;
	int32_t rc = 0;
	int16_t cur_code = 0;
	int16_t step_index = 0, region_index = 0;
	uint16_t step_boundary = 0;
	uint32_t max_code_size = 1;
	uint16_t data_size = set_info->actuator_params.data_size;
	CDBG("%s called\n", __func__);

#if 1
	update_otp_af(a_ctrl) ;
	pr_err("cdz::%s   inf_value=%d  mac_value=%d \n", __func__,inf_value,mac_value);

#if 0
	update_otp_wb(a_ctrl);//added for wb otp
	pr_err("%s  current_wb_otp id =0x%x\n",__func__,current_wb_otp.module_integrator_id);

	update_otp_lenc(a_ctrl);
	pr_err("%s  current_lenc_otp id =0x%x\n",__func__,current_lenc_otp.module_integrator_id);
#endif	

if(inf_value!=0&&mac_value!=0){
	set_info->af_tuning_params.initial_code=inf_value*16;

	pr_err("cdz::%s   before code_per_step=%d  \n", __func__,a_ctrl->region_params[0].code_per_step);
	
	a_ctrl->region_params[0].code_per_step=((mac_value-inf_value)*16+(set_info->af_tuning_params.total_steps-2)/2)/(set_info->af_tuning_params.total_steps-2);

	pr_err("cdz::%s   code_per_step=%d  \n", __func__,a_ctrl->region_params[0].code_per_step);
}
#endif	

	for (; data_size > 0; data_size--)
		max_code_size *= 2;

	kfree(a_ctrl->step_position_table);
	a_ctrl->step_position_table = NULL;

	/* Fill step position table */
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) *
		(set_info->af_tuning_params.total_steps + 1), GFP_KERNEL);

	if (a_ctrl->step_position_table == NULL)
		return -EFAULT;

	cur_code = set_info->af_tuning_params.initial_code;

       pr_err("cdz::%s   region_size=%d  \n", __func__,a_ctrl->region_size);
	
	a_ctrl->step_position_table[step_index++] = cur_code;
	for (region_index = 0;
		region_index < a_ctrl->region_size;
		region_index++) {
		code_per_step =
			a_ctrl->region_params[region_index].code_per_step;
		step_boundary =
			a_ctrl->region_params[region_index].
			step_bound[MOVE_NEAR];
		for (; step_index <= step_boundary;
			step_index++) {
			cur_code += code_per_step;
			if (cur_code < max_code_size)
				a_ctrl->step_position_table[step_index] =
					cur_code;
			else {
				for (; step_index <
					set_info->af_tuning_params.total_steps;
					step_index++)
					a_ctrl->
						step_position_table[
						step_index] =
						max_code_size;

				return rc;
			}
		}
	}

	return rc;
}

#else
static int32_t msm_actuator_init_step_table(struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_set_info_t *set_info)
{
	int16_t code_per_step = 0;
	int32_t rc = 0;
	int16_t cur_code = 0;
	int16_t step_index = 0, region_index = 0;
	uint16_t step_boundary = 0;
	uint32_t max_code_size = 1;
	uint16_t data_size = set_info->actuator_params.data_size;
	uint16_t i=0;
	CDBG("%s called\n", __func__);

	for (; data_size > 0; data_size--)
		max_code_size *= 2;

	kfree(a_ctrl->step_position_table);
	a_ctrl->step_position_table = NULL;

	/* Fill step position table */
	a_ctrl->step_position_table =
		kmalloc(sizeof(uint16_t) *
		(set_info->af_tuning_params.total_steps + 1), GFP_KERNEL);

	if (a_ctrl->step_position_table == NULL)
		return -EFAULT;

	cur_code = set_info->af_tuning_params.initial_code;
	a_ctrl->step_position_table[step_index++] = cur_code;
	for (region_index = 0;
		region_index < a_ctrl->region_size;
		region_index++) {
		code_per_step =
			a_ctrl->region_params[region_index].code_per_step;
		step_boundary =
			a_ctrl->region_params[region_index].
			step_bound[MOVE_NEAR];
		for (; step_index <= step_boundary;
			step_index++) {
			cur_code += code_per_step;
			if (cur_code < max_code_size)
				a_ctrl->step_position_table[step_index] =
					cur_code;
			else {
				for (; step_index <
					set_info->af_tuning_params.total_steps;
					step_index++)
					a_ctrl->
						step_position_table[
						step_index] =
						max_code_size;

				return rc;
			}
		}
	}

	for (i=0; i<set_info->af_tuning_params.total_steps; i++) {
		printk("%s: Step_Pos_Table[%d]:%d\n", __func__, i,
			a_ctrl->step_position_table[i]);
	}
	return rc;
}
#endif

static int32_t msm_actuator_set_default_focus(
	struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_move_params_t *move_params)
{
	int32_t rc = 0;
	CDBG("%s called\n", __func__);

	if (a_ctrl->curr_step_pos != 0)
		rc = a_ctrl->func_tbl->actuator_move_focus(a_ctrl, move_params);
	return rc;
}

static int32_t msm_actuator_power_down(struct msm_actuator_ctrl_t *a_ctrl)
{
	int32_t rc = 0;
	if (a_ctrl->vcm_enable) {
		rc = gpio_direction_output(a_ctrl->vcm_pwd, 0);
		if (!rc)
			gpio_free(a_ctrl->vcm_pwd);
	}

	kfree(a_ctrl->step_position_table);
	a_ctrl->step_position_table = NULL;
	kfree(a_ctrl->i2c_reg_tbl);
	a_ctrl->i2c_reg_tbl = NULL;
	a_ctrl->i2c_tbl_index = 0;
	return rc;
}

static int32_t msm_actuator_init(struct msm_actuator_ctrl_t *a_ctrl,
	struct msm_actuator_set_info_t *set_info) {
	struct reg_settings_t *init_settings = NULL;
	int32_t rc = -EFAULT;
	uint16_t i = 0;
	CDBG("%s: IN\n", __func__);

	for (i = 0; i < ARRAY_SIZE(actuators); i++) {
		if (set_info->actuator_params.act_type ==
			actuators[i]->act_type) {
			a_ctrl->func_tbl = &actuators[i]->func_tbl;
			rc = 0;
		}
	}

	if (rc < 0) {
		pr_err("%s: Actuator function table not found\n", __func__);
		return rc;
	}

	a_ctrl->region_size = set_info->af_tuning_params.region_size;
	if (a_ctrl->region_size > MAX_ACTUATOR_REGION) {
		pr_err("%s: MAX_ACTUATOR_REGION is exceeded.\n", __func__);
		return -EFAULT;
	}
	a_ctrl->pwd_step = set_info->af_tuning_params.pwd_step;
	a_ctrl->total_steps = set_info->af_tuning_params.total_steps;

	if (copy_from_user(&a_ctrl->region_params,
		(void *)set_info->af_tuning_params.region_params,
		a_ctrl->region_size * sizeof(struct region_params_t)))
		return -EFAULT;

	a_ctrl->i2c_data_type = set_info->actuator_params.i2c_data_type;
	a_ctrl->i2c_client.client->addr = set_info->actuator_params.i2c_addr;
	a_ctrl->i2c_client.addr_type = set_info->actuator_params.i2c_addr_type;
	a_ctrl->reg_tbl_size = set_info->actuator_params.reg_tbl_size;
	if (a_ctrl->reg_tbl_size > MAX_ACTUATOR_REG_TBL_SIZE) {
		pr_err("%s: MAX_ACTUATOR_REG_TBL_SIZE is exceeded.\n",
			__func__);
		return -EFAULT;
	}

	a_ctrl->i2c_reg_tbl =
		kmalloc(sizeof(struct msm_camera_i2c_reg_tbl) *
		(set_info->af_tuning_params.total_steps + 1), GFP_KERNEL);
	if (!a_ctrl->i2c_reg_tbl) {
		pr_err("%s kmalloc fail\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(&a_ctrl->reg_tbl,
		(void *)set_info->actuator_params.reg_tbl_params,
		a_ctrl->reg_tbl_size *
		sizeof(struct msm_actuator_reg_params_t))) {
		kfree(a_ctrl->i2c_reg_tbl);
		return -EFAULT;
	}

	if (set_info->actuator_params.init_setting_size) {
		if (a_ctrl->func_tbl->actuator_init_focus) {
			init_settings = kmalloc(sizeof(struct reg_settings_t) *
				(set_info->actuator_params.init_setting_size),
				GFP_KERNEL);
			if (init_settings == NULL) {
				kfree(a_ctrl->i2c_reg_tbl);
				pr_err("%s Error allocating memory for init_settings\n",
					__func__);
				return -EFAULT;
			}
			if (copy_from_user(init_settings,
				(void *)set_info->actuator_params.init_settings,
				set_info->actuator_params.init_setting_size *
				sizeof(struct reg_settings_t))) {
				kfree(init_settings);
				kfree(a_ctrl->i2c_reg_tbl);
				pr_err("%s Error copying init_settings\n",
					__func__);
				return -EFAULT;
			}
			rc = a_ctrl->func_tbl->actuator_init_focus(a_ctrl,
				set_info->actuator_params.init_setting_size,
				a_ctrl->i2c_data_type,
				init_settings);
			kfree(init_settings);
			if (rc < 0) {
				kfree(a_ctrl->i2c_reg_tbl);
				pr_err("%s Error actuator_init_focus\n",
					__func__);
				return -EFAULT;
			}
		}
	}

	a_ctrl->initial_code = set_info->af_tuning_params.initial_code;
	if (a_ctrl->func_tbl->actuator_init_step_table)
		rc = a_ctrl->func_tbl->
			actuator_init_step_table(a_ctrl, set_info);

	a_ctrl->curr_step_pos = 0;
	a_ctrl->curr_region_index = 0;

	return rc;
}


static int32_t msm_actuator_config(struct msm_actuator_ctrl_t *a_ctrl,
							void __user *argp)
{
	struct msm_actuator_cfg_data cdata;
	int32_t rc = 0;
	if (copy_from_user(&cdata,
		(void *)argp,
		sizeof(struct msm_actuator_cfg_data)))
		return -EFAULT;
	mutex_lock(a_ctrl->actuator_mutex);
	CDBG("%s called, type %d\n", __func__, cdata.cfgtype);
	switch (cdata.cfgtype) {
	case CFG_SET_ACTUATOR_INFO:
		rc = msm_actuator_init(a_ctrl, &cdata.cfg.set_info);
		if (rc < 0)
			pr_err("%s init table failed %d\n", __func__, rc);
		break;

	case CFG_SET_DEFAULT_FOCUS:
		rc = a_ctrl->func_tbl->actuator_set_default_focus(a_ctrl,
			&cdata.cfg.move);
		if (rc < 0)
			pr_err("%s move focus failed %d\n", __func__, rc);
		break;

	case CFG_MOVE_FOCUS:
		rc = a_ctrl->func_tbl->actuator_move_focus(a_ctrl,
			&cdata.cfg.move);
		if (rc < 0)
			pr_err("%s move focus failed %d\n", __func__, rc);
		break;

	default:
		break;
	}
	mutex_unlock(a_ctrl->actuator_mutex);
	return rc;
}

static int32_t msm_actuator_i2c_probe(
	struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	struct msm_actuator_ctrl_t *act_ctrl_t = NULL;
	CDBG("%s called\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	act_ctrl_t = (struct msm_actuator_ctrl_t *)(id->driver_data);
	CDBG("%s client = %x\n",
		__func__, (unsigned int) client);
	act_ctrl_t->i2c_client.client = client;

	/* Assign name for sub device */
	snprintf(act_ctrl_t->sdev.name, sizeof(act_ctrl_t->sdev.name),
			 "%s", act_ctrl_t->i2c_driver->driver.name);

	/* Initialize sub device */
	v4l2_i2c_subdev_init(&act_ctrl_t->sdev,
		act_ctrl_t->i2c_client.client,
		act_ctrl_t->act_v4l2_subdev_ops);

	CDBG("%s succeeded\n", __func__);
	return rc;

probe_failure:
	pr_err("%s failed! rc = %d\n", __func__, rc);
	return rc;
}

static int32_t msm_actuator_power_up(struct msm_actuator_ctrl_t *a_ctrl)
{
	int rc = 0;
	CDBG("%s called\n", __func__);

	CDBG("vcm info: %x %x\n", a_ctrl->vcm_pwd,
		a_ctrl->vcm_enable);
	if (a_ctrl->vcm_enable) {
		rc = gpio_request(a_ctrl->vcm_pwd, "msm_actuator");
		if (!rc) {
			CDBG("Enable VCM PWD\n");
			gpio_direction_output(a_ctrl->vcm_pwd, 1);
		}
	}
	return rc;
}

DEFINE_MUTEX(msm_actuator_mutex);

static const struct i2c_device_id msm_actuator_i2c_id[] = {
	{"msm_actuator", (kernel_ulong_t)&msm_actuator_t},
	{ }
};

static struct i2c_driver msm_actuator_i2c_driver = {
	.id_table = msm_actuator_i2c_id,
	.probe  = msm_actuator_i2c_probe,
	.remove = __exit_p(msm_actuator_i2c_remove),
	.driver = {
		.name = "msm_actuator",
	},
};

static int __init msm_actuator_i2c_add_driver(
	void)
{
	CDBG("%s called\n", __func__);
	return i2c_add_driver(msm_actuator_t.i2c_driver);
}

static long msm_actuator_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	struct msm_actuator_ctrl_t *a_ctrl = get_actrl(sd);
	void __user *argp = (void __user *)arg;
	switch (cmd) {
	case VIDIOC_MSM_ACTUATOR_CFG:
		return msm_actuator_config(a_ctrl, argp);
	default:
		return -ENOIOCTLCMD;
	}
}

static int32_t msm_actuator_power(struct v4l2_subdev *sd, int on)
{
	int rc = 0;
	struct msm_actuator_ctrl_t *a_ctrl = get_actrl(sd);
	mutex_lock(a_ctrl->actuator_mutex);
	if (on)
		rc = msm_actuator_power_up(a_ctrl);
	else
		rc = msm_actuator_power_down(a_ctrl);
	mutex_unlock(a_ctrl->actuator_mutex);
	return rc;
}

struct msm_actuator_ctrl_t *get_actrl(struct v4l2_subdev *sd)
{
	return container_of(sd, struct msm_actuator_ctrl_t, sdev);
}

static struct v4l2_subdev_core_ops msm_actuator_subdev_core_ops = {
	.ioctl = msm_actuator_subdev_ioctl,
	.s_power = msm_actuator_power,
};

static struct v4l2_subdev_ops msm_actuator_subdev_ops = {
	.core = &msm_actuator_subdev_core_ops,
};

static struct msm_actuator_ctrl_t msm_actuator_t = {
	.i2c_driver = &msm_actuator_i2c_driver,
	.act_v4l2_subdev_ops = &msm_actuator_subdev_ops,

	.curr_step_pos = 0,
	.curr_region_index = 0,
	.actuator_mutex = &msm_actuator_mutex,

};

#ifdef CONFIG_OV8825
static struct msm_actuator msm_vcm_actuator_table = {
	.act_type = ACTUATOR_VCM,
	.func_tbl = {
		.actuator_init_step_table = ov8825_msm_actuator_init_step_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = msm_actuator_init_focus,
		.actuator_parse_i2c_params = msm_actuator_parse_i2c_params,
	},
};
#else
static struct msm_actuator msm_vcm_actuator_table = {
	.act_type = ACTUATOR_VCM,
	.func_tbl = {
		.actuator_init_step_table = msm_actuator_init_step_table,
		.actuator_move_focus = msm_actuator_move_focus,
		.actuator_write_focus = msm_actuator_write_focus,
		.actuator_set_default_focus = msm_actuator_set_default_focus,
		.actuator_init_focus = msm_actuator_init_focus,
		.actuator_parse_i2c_params = msm_actuator_parse_i2c_params,
	},
};
#endif

static struct msm_actuator msm_piezo_actuator_table = {
	.act_type = ACTUATOR_PIEZO,
	.func_tbl = {
		.actuator_init_step_table = NULL,
		.actuator_move_focus = msm_actuator_piezo_move_focus,
		.actuator_write_focus = NULL,
		.actuator_set_default_focus =
			msm_actuator_piezo_set_default_focus,
		.actuator_init_focus = msm_actuator_init_focus,
		.actuator_parse_i2c_params = msm_actuator_parse_i2c_params,
	},
};

subsys_initcall(msm_actuator_i2c_add_driver);
MODULE_DESCRIPTION("MSM ACTUATOR");
MODULE_LICENSE("GPL v2");
