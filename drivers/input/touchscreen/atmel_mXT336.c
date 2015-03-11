/* drivers/input/touchscreen/atmel.c - ATMEL Touch driver
 *
 * Copyright (C) 2009 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/* ========================================================================================
when            who       what, where, why                         		comment tag
--------     ----       -------------------------------------    --------------------------
2011-08-30   hjy        add the TOUCH_LONG_SLIDE function    TOUCH_LONG_SLIDE
2011-04-25   zfj         add P732A driver code                      ZTE_TS_ZFJ_20110425
2011-02-23   hjy         ATMEL 140 用在P727A20上       ZTE_HJY_CRDB00000000
2010-12-14   wly         v9默认竖屏                                 ZTE_WLY_CRDB00586327
2010-12-13   wly         v9＋默认竖屏                               ZTE_WLY_CRDB00586327
2010-11-24   wly         解决手掌在屏上，睡眠唤醒后数据乱报问题     ZTE_WLY_CRDB00577718
2010-10-25   wly         update report data format                  ZTE_WLY_CRDB00517999
========================================================================================*/
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <mach/board.h>
#include <asm/mach-types.h>
#include <linux/input/atmel_qt602240.h>
#include <linux/jiffies.h>
#include <mach/msm_hsusb.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/fb.h>
#include <linux/bitops.h>


#define ATMEL_I2C_RETRY_TIMES 10

extern struct atmel_config_data atmel_data;



static uint8_t count=0;


#define TOUCH_LONG_SLIDE
#ifdef TOUCH_LONG_SLIDE
static int x_value;
static int y_value;
//是否取了起点的标志位: 0-no, 1-yes, 2-ok
static uint8_t temp_flag=0;
//是否滑动完成的标志位: 0-no, 1-yes
static uint8_t temp_flag2=0;
#endif

static struct atmel_ts_data *atmel_ts;

struct atmel_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *atmel_wq;
	struct work_struct work;
	//int (*power) (int on);
	struct early_suspend early_suspend;
	struct info_id_t *id;
	struct object_t *object_table;
	uint8_t finger_count;
	uint16_t abs_x_min;
	uint16_t abs_x_max;
	uint16_t abs_y_min;
	uint16_t abs_y_max;
	uint8_t abs_pressure_min;
	uint8_t abs_pressure_max;
	uint8_t abs_width_min;
	uint8_t abs_width_max;
	//uint8_t first_pressed;
	uint8_t debug_log_level;
	struct atmel_finger_data finger_data[10];
	uint8_t finger_type;
	uint8_t finger_support;
	uint16_t finger_pressed;
	//uint8_t face_suppression;
	//uint8_t grip_suppression;
	//uint8_t noise_status[2];
	uint16_t *filter_level;
	uint8_t calibration_confirm;
	uint64_t timestamp;
	
	uint64_t timestamp1;
	struct atmel_config_data_part config_setting[2];
	uint8_t status;
	uint8_t GCAF_sample;
	uint8_t *GCAF_level;
	uint8_t noisethr;
//#ifdef ATMEL_EN_SYSFS
//	struct device dev;
//#endif
#ifdef ENABLE_IME_IMPROVEMENT
	int display_width;	/* display width in pixel */
	int display_height;	/* display height in pixel */
	int ime_threshold_pixel;	/* threshold in pixel */
	int ime_threshold[2];		/* threshold X & Y in raw data */
	int ime_area_pixel[4];		/* ime area in pixel */
	int ime_area_pos[4];		/* ime area in raw data */
	int ime_finger_report[2];
	int ime_move;
#endif

	struct atmel_config_data *pConfig;
//	int gpio_irq;

	int (*gpio_init)(int on);
	void (*power)(int on);
	void (*reset)(int on);
	void (*irq)(int hl, bool flag);
	char  fwfile[64];	// firmware file name
};

//static struct atmel_ts_data *private_ts;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void atmel_ts_early_suspend(struct early_suspend *h);
static void atmel_ts_late_resume(struct early_suspend *h);
#endif

int i2c_atmel_read(struct i2c_client *client, uint16_t address, uint8_t *data, uint8_t length)
{
	int retry;
	uint8_t addr[2];
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};
	addr[0] = address & 0xFF;
	addr[1] = (address >> 8) & 0xFF;

	for (retry = 0; retry < ATMEL_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		mdelay(10);
	}
	if (retry == ATMEL_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_read_block retry over %d\n",
			ATMEL_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;
}

int i2c_atmel_write(struct i2c_client *client, uint16_t address, uint8_t *data, uint8_t length)
{
	int retry, loop_i;
	uint8_t buf[length + 2];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 2,
			.buf = buf,
		}
	};

	buf[0] = address & 0xFF;
	buf[1] = (address >> 8) & 0xFF;

	for (loop_i = 0; loop_i < length; loop_i++)
		buf[loop_i + 2] = data[loop_i];

	for (retry = 0; retry < ATMEL_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}

	if (retry == ATMEL_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_write_block retry over %d\n",
			ATMEL_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;
}

int i2c_atmel_write_byte_data(struct i2c_client *client, uint16_t address, uint8_t value)
{
	i2c_atmel_write(client, address, &value, 1);
	return 0;
}
int i2c_atmel_write_word(struct i2c_client *client, uint16_t address, int16_t *data, uint8_t length)
{
	int i,ret;
	uint8_t buf[length*2];
	for(i =0;i<length;i++)
	{
		buf[i*2] = data[i] & 0xff;
		buf[i*2+1] = data[i]>> 8;
  }
  ret = i2c_atmel_write(client, address, buf, length*2);
  return ret;
    	
}
int i2c_atmel_read_word(struct i2c_client *client, uint16_t address, int16_t *data, uint8_t length)
{
	  uint8_t i,buf[length*2],ret;
    ret = i2c_atmel_read(client, address, buf, length*2);
    for(i=0;i<length*2;i++)
    data[i]= buf[i*2]|(buf[i*2+1] << 8);
    return ret;
}
uint16_t get_object_address(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].i2c_address;
	}
	return 0;
}
uint8_t get_object_size(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].size;
	}
	return 0;
}

uint8_t get_report_ids_size(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].report_ids;
	}
	return 0;
}

#if 1 //wly add
static void release_all_fingers(struct atmel_ts_data*ts)
{
	int loop_i;

	for(loop_i =0; loop_i< ts->finger_support; loop_i ++ )
	{
		if(ts->finger_data[loop_i].report_enable)
		{
			printk("release all fingers!\n");
			ts->finger_data[loop_i].z = 0;
			ts->finger_data[loop_i].report_enable = 0;
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[loop_i].z);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[loop_i].w);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[loop_i].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[loop_i].y);	
			input_mt_sync(ts->input_dev);	
		}
		
	}
	input_sync(ts->input_dev);
	ts->finger_pressed = 0;
	ts->finger_count = 0;
}
#endif


#if defined CONFIG_TS_NOTIFIER
static int usb_status=0;

static int ts_event(struct notifier_block *this, unsigned long event,
			   void *ptr)
{
	int ret;

	switch(event)
		{
		case 0:
			//offline
			if(usb_status!=0){
		 		usb_status=0;
				printk("ts config change to offline status\n");
		 		if(atmel_ts->id->version!=0x16){
		 		i2c_atmel_write_byte_data(atmel_ts->client,
		 			get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 31,
		 			atmel_ts->config_setting[0].config_T9[31]);
		 		}				

		 		i2c_atmel_write_byte_data(atmel_ts->client,
		 			get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 7,
		 			atmel_ts->config_setting[0].config_T9[7]);

		 		i2c_atmel_write_byte_data(atmel_ts->client,
		 			get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 11,
		 			atmel_ts->config_setting[0].config_T9[11]);
		 		
		 		i2c_atmel_write_byte_data(atmel_ts->client,
		 			get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 12,
		 			atmel_ts->config_setting[0].config_T9[12]);			
			}

			break;
		case 1:
			//online
			if(usb_status!=1){
		 		usb_status=1;
				printk("ts config change to online status\n");
	 			if(atmel_ts->id->version!=0x16){
	 			i2c_atmel_write_byte_data(atmel_ts->client,
	 				get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 31,
	 				atmel_ts->config_setting[0].config_T9_charge[1]);
	 			}
	 			i2c_atmel_write_byte_data(atmel_ts->client,
	 				get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 7,
	 				atmel_ts->config_setting[0].config_T9_charge[0]);
	 			
	 			i2c_atmel_write_byte_data(atmel_ts->client,
	 				get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 11,
	 				atmel_ts->config_setting[0].config_T9_charge[2]);
	 			
	 			i2c_atmel_write_byte_data(atmel_ts->client,
	 				get_object_address(atmel_ts, TOUCH_MULTITOUCHSCREEN_T9) + 12,
	 				atmel_ts->config_setting[0].config_T9_charge[3]);
			}
			break;
		default:
			break;
		}

	ret = NOTIFY_DONE;

	return ret;
}

static struct notifier_block ts_notifier = {
	.notifier_call = ts_event,
};


static BLOCKING_NOTIFIER_HEAD(ts_chain_head);

int register_ts_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&ts_chain_head, nb);
}
EXPORT_SYMBOL_GPL(register_ts_notifier);

int unregister_ts_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&ts_chain_head, nb);
}
EXPORT_SYMBOL_GPL(unregister_ts_notifier);

int ts_notifier_call_chain(unsigned long val)
{
	return (blocking_notifier_call_chain(&ts_chain_head, val, NULL)
			== NOTIFY_BAD) ? -EINVAL : 0;
}

#endif
#if 1
static int detect_device(struct i2c_client *client)
{
	int i, buf;
	bool ret = 0;

	if ( client == NULL )
		return ret;


	for (i=0; i<3; i++ )
	{
		// 0xFF: synaptics rmi page select register
		//buf = i2c_smbus_read_byte_data(client, 0xFF);
		buf = i2c_smbus_read_byte_data(client, 0x0000);
		if ( buf >= 0 ){
			ret = 1;
			break;
		}
		msleep(10);
	}

	return ret;
}

static int get_screeninfo(uint *xres, uint *yres)
{
	struct fb_info *info;

	info = registered_fb[0];
	if (!info) {
		pr_err("%s: Can not access lcd info \n",__func__);
		return -ENODEV;
	}

	*xres = info->var.xres;
	*yres = info->var.yres;
	printk("%s: xres=%d, yres=%d \n", __func__, *xres, *yres);

	return 1;
}

#endif
static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "atmel");
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x4a);

	len += sprintf(page + len, "IC type           : %s\n", "atmel 336");

	len += sprintf(page + len, "family id         : 0x%x\n", atmel_ts->id->family_id);
	len += sprintf(page + len, "variant id        : 0x%x\n", atmel_ts->id->variant_id);
	len += sprintf(page + len, "firmware version  : 0x%x\n", atmel_ts->id->version);

	len += sprintf(page + len, "module            : %s\n", "atmel + laibao");

	if (off + count >= len)
		*eof = 1;
	if (len < off)
		return 0;
	*start = page + off;

	return ((count < len - off) ? count : len - off);
}
static int proc_write_val(struct file *file, const char *buffer,
				   unsigned long count, void *data)
{
	unsigned long val;
	sscanf(buffer, "%lu", &val);

	return -EINVAL;
}
static int atmel_ts_get_build_info(struct atmel_ts_data*ts)
{
	uint8_t data[7];
	int ret;
	printk("i2c address:0x%x",ts->client->addr);
	ret = i2c_atmel_read(ts->client, 0x0000, data, 7);

	ts->id->family_id = data[0];
	ts->id->variant_id = data[1];
	ts->id->version = data[2];
	ts->id->build = data[3];
	ts->id->matrix_x_size = data[4];
	ts->id->matrix_y_size = data[5];
	ts->id->num_declared_objects = data[6];

	printk(KERN_INFO "info block: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
	ts->id->family_id, ts->id->variant_id,
	ts->id->version, ts->id->build,
	ts->id->matrix_x_size, ts->id->matrix_y_size,
	ts->id->num_declared_objects);
	
	return 0;
}
static int atmel_ts_get_table_info(struct atmel_ts_data*ts)
{
	uint8_t data[6],type_count = 0;
	int i,ret;
	if (ts->object_table==NULL){
		printk("ts->object_table==NULL\n");
		return 0;
		}
	for (i = 0; i < ts->id->num_declared_objects; i++) {
		ret = i2c_atmel_read(ts->client, i * 6 + 0x07, data, 6);
		ts->object_table[i].object_type = data[0];
		ts->object_table[i].i2c_address = data[1] | data[2] << 8;
		ts->object_table[i].size = data[3] + 1;
		ts->object_table[i].instances = data[4];
		ts->object_table[i].num_report_ids = data[5];
		if (data[5]) {
			ts->object_table[i].report_ids = type_count + 1;
			type_count += data[5];
		}
		if (data[0] == 9)
			ts->finger_type = ts->object_table[i].report_ids;
		printk(KERN_INFO "Type: %2d, Start: 0x%4.4X, Size: %2d, Instance: %2X, RD#: 0x%2X, %2d\n",
			ts->object_table[i].object_type , ts->object_table[i].i2c_address,
			ts->object_table[i].size, ts->object_table[i].instances,
			ts->object_table[i].num_report_ids, ts->object_table[i].report_ids);
	}
	return 0;
}

static int atmel_ts_load_config(struct atmel_ts_data*ts)
{
	int ret,loop_i;
	uint8_t data[9];
	struct atmel_config_data *pdata;
	pdata = ts->pConfig;

	printk(KERN_INFO "Touch: Config load\n");

	i2c_atmel_write(ts->client, get_object_address(ts, GEN_POWERCONFIG_T7),
		pdata->config_T7, get_object_size(ts, GEN_POWERCONFIG_T7));
	i2c_atmel_write(ts->client, get_object_address(ts, GEN_ACQUISITIONCONFIG_T8),
		pdata->config_T8, get_object_size(ts, GEN_ACQUISITIONCONFIG_T8));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_MULTITOUCHSCREEN_T9),
		pdata->config_T9, get_object_size(ts, TOUCH_MULTITOUCHSCREEN_T9));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_KEYARRAY_T15),
		pdata->config_T15, get_object_size(ts, TOUCH_KEYARRAY_T15));
	i2c_atmel_write(ts->client, get_object_address(ts, SPT_GPIOPWM_T19),
		pdata->config_T19, get_object_size(ts, SPT_GPIOPWM_T19));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_PROXIMITY_T23),
		pdata->config_T23, get_object_size(ts, TOUCH_PROXIMITY_T23));
	i2c_atmel_write(ts->client, get_object_address(ts, SPT_SELFTEST_T25),
		pdata->config_T25, get_object_size(ts, SPT_SELFTEST_T25));

	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_GRIP),
		pdata->config_T40, get_object_size(ts, MXT336_SPT_GRIP));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_PALM),
		pdata->config_T42, get_object_size(ts, MXT336_SPT_PALM));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_STYLUS),
		pdata->config_T47, get_object_size(ts, MXT336_SPT_STYLUS));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_ADAPTIVE_THRESHOLD),
		pdata->config_T55, get_object_size(ts, MXT336_ADAPTIVE_THRESHOLD));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_SLIMSENSOR),
		pdata->config_T56, get_object_size(ts, MXT336_SPT_SLIMSENSOR));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_EXTEA_DATA),
		pdata->config_T57, get_object_size(ts, MXT336_EXTEA_DATA));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_timer),
		pdata->config_T61, get_object_size(ts, MXT336_SPT_timer));
	i2c_atmel_write(ts->client, get_object_address(ts, MXT336_SPT_MAXCHARGER),
		pdata->config_T62, get_object_size(ts, MXT336_SPT_MAXCHARGER));
								

	ret = i2c_atmel_write_byte_data(ts->client, get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 1, 0x55);
	ret = i2c_atmel_write_byte_data(ts->client, get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x11);
	msleep(64);
	for (loop_i = 0; loop_i < 10; loop_i++) {
		if (!gpio_get_value(INT_TO_MSM_GPIO(ts->client->irq)))
			break;
		printk(KERN_INFO "Touch: wait for Message(%d)\n", loop_i + 1);
		msleep(10);
	}

	i2c_atmel_read(ts->client, get_object_address(ts, GEN_MESSAGEPROCESSOR_T5), data, 9);
	printk(KERN_INFO "Touch: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
	return 0;
}

#if 0
static void check_atch_and_tch(struct atmel_ts_data*ts,uint8_t*atch_and_tch)
{
	
	uint8_t data[98];
	uint8_t x_limit,check_mask,loop_i,loop_j, tch_ch = 0, atch_ch = 0;
	
	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0xF3);

	for (loop_i = 0; !(data[0] == 0xF3 && data[1] == 0x00) && loop_i < 100; loop_i++) {
		msleep(5);
		i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 2);
	}

	if (loop_i == 100){
		printk(KERN_ERR "%s: Diag data not ready\n", __func__);
		
		ts->calibration_confirm = 0;
		//reset
		i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
		return ;
	}	   
	i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 98);
	if (data[0] == 0xF3 && data[1] == 0x00) {	

		x_limit = ts->config_setting[0].config_T9[3]+ts->config_setting[0].config_T9[1];
		x_limit = x_limit << 1;
		#if 0
		if(ATMEL_CHIP_TYPE==ATMEL_140)
		{
	 		for (loop_i = ts->config_setting[0].config_T9[1]; loop_i < x_limit; loop_i += 2) {
	 			for (loop_j = ts->config_setting[0].config_T9[2]; loop_j < 8; loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[2 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[32 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
	 			for (loop_j = 0; 
				loop_j < ts->config_setting[0].config_T9[4]+ts->config_setting[0].config_T9[2]-8; 
				loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[3 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[33 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
			}
		}else if(ATMEL_CHIP_TYPE==ATMEL_224){
	 		for (loop_i = ts->config_setting[0].config_T9[1]; loop_i < x_limit; loop_i += 2) {
	 			for (loop_j = ts->config_setting[0].config_T9[2]; loop_j < 8; loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[2 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[42 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
	 			for (loop_j = 0; 
				loop_j < ts->config_setting[0].config_T9[4]+ts->config_setting[0].config_T9[2]-8; 
				loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[3 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[43 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
			}
		}
		#else
		
	 		for (loop_i = ts->config_setting[0].config_T9[1]; loop_i < x_limit; loop_i += 2) {
	 			for (loop_j = ts->config_setting[0].config_T9[2]; loop_j < 8; loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[2 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[50 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
	 			for (loop_j = 0; 
				loop_j < ts->config_setting[0].config_T9[4]+ts->config_setting[0].config_T9[2]-8; 
				loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[3 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[51 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
			}		
		#endif
		atch_and_tch[0]=atch_ch;
		atch_and_tch[1]=tch_ch;		
	}
}
#endif
static void check_calibration(struct atmel_ts_data*ts,uint8_t *data)
  	{
	//uint8_t data[82];
	//uint8_t atch_and_tch[2];
	uint16_t tch_ch = 0, atch_ch = 0;
#if 0
	memset(data, 0xFF, sizeof(data));
	
	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0xF3);

	for (loop_i = 0; !(data[0] == 0xF3 && data[1] == 0x00) && loop_i < 100; loop_i++) {
		msleep(5);
		i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 2);
	}

	if (loop_i == 100){
		printk(KERN_ERR "%s: Diag data not ready\n", __func__);
		
		ts->calibration_confirm = 0;
		//reset
		i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
		return ;
	}
	
	check_atch_and_tch(ts,atch_and_tch);
	atch_ch=atch_and_tch[0];
	tch_ch=atch_and_tch[1];	

	printk("***********: check_calibration, tch_ch=%d, atch_ch=%d\n", tch_ch, atch_ch);

	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0x01);


#endif
	tch_ch = data[3]|(data[4]<<8);
	atch_ch = data[5]|(data[6]<<8);	
	printk("***********: check_calibration, tch_ch=%d, atch_ch=%d\n", tch_ch, atch_ch);
	//20110531
	//对于这个tch_ch=0的条件，还是有疑问的
	//henk说不要这个条件，但没有这个条件的话，不能在第一时间对计时进行设置
	//影响了校准OK的时间，从而影响了RELEASE ALL FINGERS的时间，导致错误的释放
	//所以需要知道这个计时到底从什么时候开始
	//if ((tch_ch>=0) && (atch_ch == 0)&&(tch_ch <10)) 
	if ((tch_ch>=0) && (atch_ch == 0)&&(tch_ch <40)) 
	{
		//if((tch_ch==0) && (atch_ch == 0)&&(ts->calibration_confirm == 1))
			//release_all_fingers(ts);
#ifdef TOUCH_LONG_SLIDE
		if ((jiffies > (ts->timestamp + HZ/2)||(jiffies<ts->timestamp && jiffies>HZ/2)) 
			&& (ts->calibration_confirm == 1)&&(temp_flag2==1)) 			

		{
			ts->calibration_confirm = 2;
			

   			i2c_atmel_write_byte_data(ts->client,
   			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6,
   				255/*ts->config_setting[0].config_T8[6]*/);
   
   			i2c_atmel_write_byte_data(ts->client,
   				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7,
   				1);
			
			if((ts->id->version!=0x16))
			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, ts->config_setting[0].config_T8[8]);

			temp_flag2=0;

			printk("***********: calibration is ok\n");
		}

#else
		if (jiffies > (ts->timestamp + HZ/2) && (ts->calibration_confirm == 1)) 			
		{
			ts->calibration_confirm = 2;
			
			//should added huangjinyu
			//but if the calibration is slow,it will wrong release one time 
			//#if !defined(CONFIG_MACH_SAILBOAT)
			//release_all_fingers(ts);
			//#endif

			i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6,
				255/*ts->config_setting[0].config_T8[6]*/);

			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7,
				1);
			
			if((ts->id->version!=0x16))
			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, ts->config_setting[0].config_T8[8]);

				printk("***********: calibration is ok\n");
		}
#endif
		if (ts->calibration_confirm == 0)
		{   	
			ts->calibration_confirm = 1;
			ts->timestamp = jiffies;
		}
		//printk("***********: timer reset, jiffies=%lu, Hz=%d", jiffies, HZ);
	} else 
			{
				if((atch_ch>0)||(tch_ch>10)){
					printk("huangjinyu : check_calibration, tch_ch=%d, atch_ch=%d\n", tch_ch, atch_ch);
					ts->calibration_confirm = 0;
					//release_all_fingers(ts);
					i2c_atmel_write_byte_data(ts->client,
						get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
#ifdef TOUCH_LONG_SLIDE
					temp_flag=0;
					temp_flag2=0;
#endif
					ts->timestamp = jiffies;
					count = 0;
				}else {
					count++;
				}
				
				if (count >=50)
				{
					count = 0;
					ts->calibration_confirm = 0;

					//reset
					i2c_atmel_write_byte_data(ts->client,
					get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
					msleep(100);
					i2c_atmel_write_byte_data(ts->client,
					get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
#ifdef TOUCH_LONG_SLIDE
					temp_flag=0;
					temp_flag2=0;
#endif
				}

	}

}

void get_finger_position(struct atmel_ts_data *ts, uint8_t report_type,uint8_t *data){
	//int ret;
			report_type = data[0] - ts->finger_type;
			if (report_type >= 0 && report_type < ts->finger_support) {
			/* for issue debug only */
			if ((data[ 1] & 0x60) == 0x60)
		{
			printk(KERN_INFO"x60 ISSUE happened: %x, %x, %x, %x, %x, %x, %x, %x\n",
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		return;
	}
		if ((data[1] & 0x22) && (((ts->finger_pressed >> report_type) & 1) == 1)) {
				ts->finger_count--;
				ts->finger_pressed &= ~(1 << report_type);
			} 
			else if ((data[1] & 0xC0) && (((ts->finger_pressed >> report_type) & 1) == 0)) {
				ts->finger_count++;
				ts->finger_pressed |= 1 << report_type;
			}
		#ifdef ENABLE_IME_IMPROVEMENT
		if (ts->ime_threshold_pixel > 0)
			ime_report_filter(ts, data);
		#else
			ts->finger_data[report_type].x = data[2] << 2 | data[4] >> 6;
			//ts->finger_data[report_type].y = data[3] << 2 | (data[4] & 0x0C) >> 2;
			/*ergate*/
			ts->finger_data[report_type].y = data[3] << 4 | ((data[4]) & 0x0F);
			ts->finger_data[report_type].w = data[5];
			ts->finger_data[report_type].z = data[6];
		#endif
		ts->finger_data[report_type].report_enable=2;
		if((data[1]&0x80)==0)
			{
			ts->finger_data[report_type].report_enable=1;/*The finger is up.*/
			ts->finger_data[report_type].z = 0;
			}
			}
}

void command_process(struct atmel_ts_data *ts, uint8_t *data)
{
	printk(KERN_INFO"command_process: %x, %x, %x, %x, %x, %x, %x, %x\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

			if ((data[1] & 0x08) == 0x08)
			{
				atmel_ts_load_config(ts);
			}
			else if (data[1] & 0x10)
			{
#ifdef TOUCH_LONG_SLIDE
				temp_flag=0;
				temp_flag2=0;
#endif
				ts->calibration_confirm = 0;
				printk("wly: command calibration set\n");
			}
			else if (!(data[1]&0x10))
			{
				if (ts->calibration_confirm == 0)
				{
					release_all_fingers(ts);
					data[1]=0;//fake check data
					data[2]=0;//fake check data
					data[3]=0;//fake check data
					data[4]=0;//fake check data
					data[5]=0;//fake check data
					data[6]=0;//fake check data
					check_calibration(ts,data);
				}
			}

	
} 

void virtual_keys(struct atmel_ts_data *ts, uint8_t key_code, uint8_t key_state,uint8_t *data)
{
  //printk("huangjinyu v-key coming data[2] = %d \n",data[2]);
  if(!ts->finger_count)
  {
    key_state = !!(data[1]& 0x80);
    
#if defined (CONFIG_MACH_ATLAS32)
    switch(data[2])
    {
      case 1:
        key_code = KEY_MENU;
        ts->finger_data[0].x = 30;
        break;
      case 2:
        key_code = KEY_HOME;
        ts->finger_data[0].x = 110;
        break;
      case 4:
        key_code = KEY_BACK;
        ts->finger_data[0].x = 200;	
        break;
      case 8:
        key_code = KEY_SEARCH;
        ts->finger_data[0].x = 300;			
        break;
      default:
      break;
    }
#else
    switch(data[2])
    {
      case 1:
        key_code = KEY_MENU;
        ts->finger_data[0].x = 30;
        break;
      case 2:
        key_code = KEY_HOME;
        ts->finger_data[0].x = 110;
        break;
      case 4:
        key_code = KEY_BACK;
        ts->finger_data[0].x = 200;	
        break;
      case 8:
        key_code = KEY_SEARCH;
        ts->finger_data[0].x = 300;			
        break;
      default:
      break;
    }
#endif
    if (!key_state)
    {
      ts->finger_data[0].w = 0;
      ts->finger_data[0].z = 0;
    }
    else
    {
      ts->finger_data[0].w = 10;
      ts->finger_data[0].z = 255;
    }
#if defined (CONFIG_MACH_ATLAS32)
    ts->finger_data[0].y = 520;
#else
    ts->finger_data[0].y = 520;
#endif
    input_report_key(ts->input_dev, BTN_TOUCH, !!key_state);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[0].z);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[0].w);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[0].x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[0].y);
    
    //pr_err("huangjinyu report touch v-key major= %d ,width major = %d ,x = %d ,y = %d \n",ts->finger_data[0].z,ts->finger_data[0].w,ts->finger_data[0].x,ts->finger_data[0].y);
    
    input_mt_sync(ts->input_dev);	
    input_sync(ts->input_dev);
  }
}

static void atmel_ts_work_func(struct work_struct *work)
{
	int ret;
	
	struct atmel_ts_data *ts = container_of(work, struct atmel_ts_data, work);
	uint8_t data[9];
	int i;
	uint8_t loop_i =0, report_type =0, key_state = 0,max_finger_report_type=0,finger_detect=0; 
	static uint8_t key_code = 0;
	
	ret = i2c_atmel_read(ts->client, get_object_address(ts,GEN_MESSAGEPROCESSOR_T5), data, 9);	
	report_type = data[0];
	finger_detect=data[1]&0x80;
	/*printk(KERN_INFO"atmel_ts_work_func: %x, %x, %x, %x, %x, %x, %x, %x,ts->finger_count:%d\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],ts->finger_count);*/
	//printk("huangjinyu report_type = %d \n",report_type);  	
	switch (report_type)
	{
				case 2:				
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
				case 10:
				case 11:{
					get_finger_position(ts, report_type,data);
					for(i=0;i<ts->finger_support+1;i++)
					{
						ret = i2c_atmel_read(ts->client, get_object_address(ts,GEN_MESSAGEPROCESSOR_T5), data, 9);	
						if((data[0]>=2)&&(data[0]<=11))
						{
							
							report_type = data[0];
							get_finger_position(ts, data[0],data);
						}
						else
						{
							if(data[0]==19)//T57
							{
								if ((ts->calibration_confirm < 2)&&(finger_detect==0x80))
								{
									check_calibration(ts,data);
								}	
							}

							break;
						}
					}
					}break;
				case 1:{command_process(ts, data);}break;
				case 12:{virtual_keys(ts, key_code, key_state,data);}break;
	 			case 19:
					printk("ATMEL miss the T57 message!\n");
					break;
				case 0xFF:
				default:break;	
	}


		//printk("x: %d\ny: %d\n",ts->finger_data[report_type].x,ts->finger_data[report_type].y);

	max_finger_report_type=9;

	if( report_type>=2 && report_type <= max_finger_report_type)
	{	
		for(loop_i =0; loop_i< ts->finger_support; loop_i ++ )
		{
			switch(ts->finger_data[loop_i].report_enable)
			{
				case 1:
				{
					ts->finger_data[loop_i].report_enable=0;
				};
				case 2:
				{
					if((!ts->finger_data[loop_i].z)==0){
					input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID,loop_i);
					input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[loop_i].z);
					input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[loop_i].w);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[loop_i].x);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[loop_i].y);	
					input_report_abs(ts->input_dev, ABS_MT_PRESSURE, ts->finger_data[loop_i].z);	
					}
					//pr_err("huangjinyu report touch major= %d ,width major = %d ,x = %d ,y = %d \n",ts->finger_data[loop_i].z,ts->finger_data[loop_i].w,ts->finger_data[loop_i].x,ts->finger_data[loop_i].y);
					input_mt_sync(ts->input_dev);	
				}break;
				default:break;
				}
			
		}

		input_sync(ts->input_dev);	
		//input_report_key(ts->input_dev, BTN_TOUCH, !!ts->finger_count);
		//input_sync(ts->input_dev);	
	}
#ifdef TOUCH_LONG_SLIDE
	if(ts->calibration_confirm !=2){
	if((temp_flag==0)&&((ts->finger_data[0].report_enable==2))){
		x_value=ts->finger_data[0].x;
		y_value=ts->finger_data[0].y;
		temp_flag=1;
	}
	if(ts->finger_data[0].report_enable==0)
		temp_flag=0;
	
	for(loop_i =1; loop_i< ts->finger_support; loop_i ++ )
	{
		if (ts->finger_data[loop_i].report_enable!=0){
			temp_flag=0;
			//temp_flag2=0;
			break;
			}
	}	
	
	if((temp_flag==1)&&((ts->finger_data[0].report_enable==2))){
		if(((x_value-ts->finger_data[0].x)>(ts->abs_x_max/4))||
			((y_value-ts->finger_data[0].y)>(ts->abs_y_max/4))||
			((ts->finger_data[0].x-x_value)>(ts->abs_x_max/4))||
			((ts->finger_data[0].y-y_value)>(ts->abs_y_max/4))){
				temp_flag2=1;
				temp_flag=2;
				printk("huangjinyu long slide ok!***************************************\n");
			}
	}

	}
#endif
	enable_irq(ts->client->irq);
}

static irqreturn_t atmel_ts_irq_handler(int irq, void *dev_id)
{
	struct atmel_ts_data *ts = dev_id;
	//printk("wly: atmel_ts_irq_handler\n");
	disable_irq_nosync(ts->client->irq);
	queue_work(ts->atmel_wq, &ts->work);
	return IRQ_HANDLED;
}
//ZTE_WLY_CRDB00509514,BEGIN


static int atmel_ts_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct atmel_ts_data *ts;
	struct atmel_platform_data *pdata1 = NULL;
	struct atmel_config_data *pconfig = NULL ;
	int ret = 0;//, intr = 0;
	uint8_t loop_i;
	uint8_t data[16];
	uint8_t CRC_check = 0;
	struct proc_dir_entry *dir, *refresh;
	int xres=0, yres=0;

	printk("%s: enter\n",__FUNCTION__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR"%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct atmel_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		printk(KERN_ERR"%s: allocate atmel_ts_data failed\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	pdata1 = client->dev.platform_data;
	if (pdata1) {
		//ts->gpio_irq = pdata1->gpio_irq;
		ts->gpio_init = pdata1->gpio_init;
		ts->power = pdata1->power;
		ts->reset	= pdata1->reset;
		ts->irq		= NULL;
		memcpy(ts->fwfile,pdata1->fwfile,sizeof(pdata1->fwfile));
		//intr = pdata1->gpio_irq;
	}else{
		pr_err("%s: error!\n",__func__);
		goto err_power_failed; 
	}

	if ( ts->gpio_init ) {
		ret = ts->gpio_init(1);
		if ( ret < 0 ){
			pr_err("%s, gpio init failed! %d\n", __func__, ret);
			goto err_power_failed;
		}
		if (ts->power) ts->power(1);
		msleep(250);
	}

	if ( !detect_device( client )){
		pr_info("%s, device is not exsit.\n", __func__);
		goto err_detect_failed;
	}

	get_screeninfo(&xres, &yres);
	
	/* read message*/
	ret = i2c_atmel_read(client, 0x0000, data, 7);
	if (ret < 0) {
		printk(KERN_INFO "No Atmel chip inside\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "Touch: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);


	ts->atmel_wq = create_singlethread_workqueue("atmel_wq");
	if (!ts->atmel_wq) {
		printk(KERN_ERR"%s: create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_cread_wq_failed;
	}
	INIT_WORK(&ts->work, atmel_ts_work_func);
	ts->client = client;
	ts->calibration_confirm = 0;
	i2c_set_clientdata(client, ts);
  	pr_info("wly: %s, ts->client->addr=0x%x\n", __FUNCTION__,ts->client->addr);


	ts->id = kzalloc(sizeof(struct info_id_t), GFP_KERNEL);
	if (ts->id == NULL) {
		printk(KERN_ERR"%s: allocate info_id_t failed\n", __func__);
		goto err_alloc_failed;
	}

	atmel_ts_get_build_info(ts);

	/* Read object table. */
	ts->object_table = kzalloc(sizeof(struct object_t)*ts->id->num_declared_objects, GFP_KERNEL);
	if (ts->object_table == NULL) {
		printk(KERN_ERR"%s: allocate object_table failed\n", __func__);
		goto err_alloc_failed;
	}
	
	atmel_ts_get_table_info(ts);
	

	ts->pConfig = &atmel_data;
	pconfig = ts->pConfig;
	if (pconfig) {
		ts->finger_support = pconfig->config_T9[14];
		printk(KERN_INFO"finger_type: %d, max finger: %d\n", ts->finger_type, ts->finger_support);

		/*infoamtion block CRC check */
		if (pconfig->object_crc[0]) {
			ret = i2c_atmel_write_byte_data(client,
				get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 1, 0x55);
			msleep(64);
			for (loop_i = 0; loop_i < 10; loop_i++) {
				ret = i2c_atmel_read(ts->client, get_object_address(ts,
					GEN_MESSAGEPROCESSOR_T5), data, 9);
					printk(KERN_INFO "crc checksum: %2d, %2d,  %2d\n",data[2],data[3],data[4]);
				  if (data[0] == get_report_ids_size(ts, GEN_COMMANDPROCESSOR_T6))
					break;
				msleep(5);
			}
			for (loop_i = 0; loop_i < 3; loop_i++) {
				if (pconfig->object_crc[loop_i] != data[loop_i + 2]) {
					printk(KERN_ERR"CRC Error: %x, %x\n", pconfig->object_crc[loop_i], data[loop_i + 2]);
					break;
				}
			}
			if (loop_i == 3) {
				printk(KERN_INFO "CRC passed: ");
				for (loop_i = 0; loop_i < 3; loop_i++)
					printk("0x%2.2X ", pconfig->object_crc[loop_i]);
				printk("\n");
				CRC_check = 1;
			}
		}
		ts->abs_x_min = pconfig->abs_x_min;
		ts->abs_x_max = xres;
		ts->abs_y_min = pconfig->abs_y_min;
		ts->abs_y_max = yres;
		ts->abs_pressure_min = pconfig->abs_pressure_min;
		ts->abs_pressure_max = pconfig->abs_pressure_max;
		ts->abs_width_min = pconfig->abs_width_min;
		ts->abs_width_max = pconfig->abs_width_max;
		ts->GCAF_level = pconfig->GCAF_level;
		printk(KERN_INFO "GCAF_level: %d, %d, %d, %d\n",
			ts->GCAF_level[0], ts->GCAF_level[1],ts->GCAF_level[2], ts->GCAF_level[3]);
		ts->filter_level = pconfig->filter_level;
		printk(KERN_INFO "filter_level: %d, %d, %d, %d\n",
			ts->filter_level[0], ts->filter_level[1], ts->filter_level[2], ts->filter_level[3]);
#ifdef ENABLE_IME_IMPROVEMENT
		ts->display_width = pconfig->display_width;
		ts->display_height = pconfig->display_height;
		if (!ts->display_width || !ts->display_height)
			ts->display_width = ts->display_height = 1;
#endif
		ts->config_setting[0].config_T7
			= ts->config_setting[1].config_T7
			= pconfig->config_T7;
		ts->config_setting[0].config_T8
			= ts->config_setting[1].config_T8
			= pconfig->config_T8;
		ts->config_setting[0].config_T9 = pconfig->config_T9;

#if defined(CONFIG_TS_NOTIFIER)		
		ts->config_setting[0].config_T9_charge = pconfig->config_T9_charge;
#endif
		if (pconfig->cable_config[0]) {
			ts->config_setting[0].config[0] = pconfig->config_T9[7];

			for (loop_i = 0; loop_i < 4; loop_i++)
				ts->config_setting[1].config[loop_i] = pconfig->cable_config[loop_i];
			ts->GCAF_sample = ts->config_setting[1].config[3];
			ts->noisethr = pconfig->cable_config[1];
		} else {
			if (pconfig->cable_config_T7[0])
				ts->config_setting[1].config_T7 = pconfig->cable_config_T7;
			if (pconfig->cable_config_T8[0])
				ts->config_setting[1].config_T8 = pconfig->cable_config_T8;
			if (pconfig->cable_config_T9[0]) {
				ts->config_setting[1].config_T9 = pconfig->cable_config_T9;
			}
		}
		if (!CRC_check) 
		{
			atmel_ts_load_config(ts);
		}
		else
		{
     		ret = i2c_atmel_write_byte_data(client, get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x11);
     		msleep(64);
     		for (loop_i = 0; loop_i < 10; loop_i++) {
     			//if (!gpio_get_value(ts->gpio_irq))
     			if (!gpio_get_value(INT_TO_MSM_GPIO(client->irq)))
     				break;
     			printk(KERN_INFO "Touch: wait for Message(%d)\n", loop_i + 1);
     			msleep(10);
     		}
		}
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_err(&client->dev, "Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "atmel-touchscreen";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	//set_bit(BTN_9, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	set_bit(ABS_MT_TRACKING_ID, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_ORIENTATION, ts->input_dev->absbit);
	set_bit(ABS_MT_PRESSURE, ts->input_dev->absbit);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
				0, 10, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				ts->abs_x_min, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				ts->abs_y_min, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
				ts->abs_pressure_min, ts->abs_pressure_max,
				0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,
				ts->abs_width_min, ts->abs_width_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 
                                ts->abs_pressure_min, ts->abs_pressure_max, 0, 0);

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev,
			"atmel_ts_probe: Unable to register %s input device\n",
			ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	ret = request_irq(client->irq, atmel_ts_irq_handler, IRQF_TRIGGER_LOW,//IRQF_TRIGGER_FALLING,
			client->name, ts);
	if (ret)
		dev_err(&client->dev, "request_irq failed\n");


	//enable_irq(client->irq);

		//ADD BY HUANGJINYU
		printk("atmel probe set the calibration\n");
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6, 0x0);
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7, 0x0);
		
		if((ts->id->version!=0x16))
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, 10);
		//ADD BY HUANGJINYU END
		
#ifdef ENABLE_IME_IMPROVEMENT
	ts->ime_threshold_pixel = 1;
	ts->ime_finger_report[0] = -1;
	ts->ime_finger_report[1] = -1;
	ret = device_create_file(&ts->input_dev->dev, &dev_attr_ime_threshold);
	if (ret) {
		printk(KERN_ERR "ENABLE_IME_IMPROVEMENT: "
					"Error to create ime_threshold\n");
		goto err_input_register_device_failed;
	}
	ret = device_create_file(&ts->input_dev->dev, &dev_attr_ime_work_area);
	if (ret) {
		printk(KERN_ERR "ENABLE_IME_IMPROVEMENT: "
					"Error to create ime_work_area\n");
		device_remove_file(&ts->input_dev->dev,
					   &dev_attr_ime_threshold);
		goto err_input_register_device_failed;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = atmel_ts_early_suspend;
	ts->early_suspend.resume = atmel_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	atmel_ts = ts;

  dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0664, dir);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}


#ifdef CONFIG_TS_NOTIFIER
		register_ts_notifier(&ts_notifier);
#endif

	dev_info(&client->dev, "Start touchscreen %s in interrupt mode\n",
			ts->input_dev->name);
	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed: 
err_alloc_failed:
	destroy_workqueue(ts->atmel_wq);
err_cread_wq_failed:
err_detect_failed:
	if (ts->gpio_init) 
		ts->gpio_init(0);
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	pr_info("%s exit\n",__func__);

	return ret;
}

static int atmel_ts_remove(struct i2c_client *client)
{
	struct atmel_ts_data *ts = i2c_get_clientdata(client);

	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);

	destroy_workqueue(ts->atmel_wq);
	input_unregister_device(ts->input_dev);
	kfree(ts);

	if (ts->power) ts->power(0);
	if (ts->gpio_init) ts->gpio_init(0);

	return 0;
}

static int atmel_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct atmel_ts_data *ts = i2c_get_clientdata(client);

	printk(KERN_INFO "%s: enter\n", __func__);

	disable_irq(ts->client->irq);

	ret = cancel_work_sync(&ts->work);
	if (ret)
		enable_irq(ts->client->irq);
	count=0;

	ts->finger_pressed = 0;
	ts->finger_count = 0;

	i2c_atmel_write_byte_data(client,
		get_object_address(ts, GEN_POWERCONFIG_T7), 0x0);
	i2c_atmel_write_byte_data(client,
		get_object_address(ts, GEN_POWERCONFIG_T7) + 1, 0x0);
	//ts->power(0);
	//release_all_fingers(ts);
	return 0;
}

static int atmel_ts_resume(struct i2c_client *client)
{
	struct atmel_ts_data *ts = i2c_get_clientdata(client);
	//ts->power(1);
	//msleep(200);

	i2c_atmel_write(ts->client, get_object_address(ts, GEN_POWERCONFIG_T7),
		ts->config_setting[0].config_T7, get_object_size(ts, GEN_POWERCONFIG_T7));
	printk("***********: go to check calibration\n");
		ts->calibration_confirm = 0;
		release_all_fingers(ts);

		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
#ifdef TOUCH_LONG_SLIDE
		temp_flag=0;
		temp_flag2=0;
#endif

		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6, 0x0);
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7, 0x0);
		
		if((ts->id->version!=0x16))
		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, 10);

	enable_irq(client->irq);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void atmel_ts_early_suspend(struct early_suspend *h)
{
	struct atmel_ts_data *ts;
	ts = container_of(h, struct atmel_ts_data, early_suspend);
	atmel_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void atmel_ts_late_resume(struct early_suspend *h)
{
	struct atmel_ts_data *ts;
	ts = container_of(h, struct atmel_ts_data, early_suspend);
	atmel_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id atml_ts_i2c_id[] = {
	{ ATMEL_QT602240_NAME, 0 },
	{ }
};

static struct i2c_driver atmel_ts_driver = {
	.id_table = atml_ts_i2c_id,
	.probe = atmel_ts_probe,
	.remove = atmel_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = atmel_ts_suspend,
	.resume = atmel_ts_resume,
#endif
	.driver = {
			.name = ATMEL_QT602240_NAME,
	},
};

static int __devinit atmel_ts_init(void)
{
       #if 1  //ZTE_XJB_20130216 for power_off charging
	extern int  offcharging_flag;
	if(offcharging_flag)
       {
	printk("%s boot is in offcharging_flag mode! return 0 derect\n", __func__);//ZTE
	return  0;
	}
	#endif
	printk(KERN_INFO "atmel_ts_init():\n");
	return i2c_add_driver(&atmel_ts_driver);
}

static void __exit atmel_ts_exit(void)
{
	i2c_del_driver(&atmel_ts_driver);
}

module_init(atmel_ts_init);
module_exit(atmel_ts_exit);

MODULE_DESCRIPTION("ATMEL Touch driver");
MODULE_LICENSE("GPL");

