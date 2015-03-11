/*
 * drivers/media/video/msm/msm_sensorinfo.c
 *
 * For sensor cfg test
 *
 * Copyright (C) 2009-2010 ZTE Corporation.
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
 * Created by li.jing19@zte.com.cn
 */
/*-----------------------------------------------------------------------------------------
  when         who      what, where, why                        comment tag
  --------     ----     -------------------------------------   ---------------------------
  2010-06-13   lijing   modify file permission                  LIJING_CAM_20100613
  2010-06-10   lijing   create file                             
------------------------------------------------------------------------------------------*/
#include <linux/sysdev.h>
#include <linux/i2c.h>
#include <linux/semaphore.h>
/*-----------------------------------------------------------------------------------------
 *
 * MACRO DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
#define MAX_NAME_LENGTH     32

#define SENSOR_INFO_MT9M114_MODEL_ID    0x2481
#define SENSOR_INFO_OV5640_MODEL_ID     0x5640
#define SENSOR_INFO_OV8825_MODEL_ID     0x8825
#define SENSOR_INFO_OV7692_MODEL_ID    0x7692
#define SENSOR_INFO_OV9740_MODEL_ID    0x9740
#define SENSOR_INFO_IMX091_MODEL_ID    0x0091

/*-----------------------------------------------------------------------------------------
 *
 * TYPE DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
static struct sysdev_class camera_sysdev_class = {
    .name = "camera",
};

static struct sys_device back_camera_sys_device = {
	.id = 0,
	.cls = &camera_sysdev_class,
};

static struct sys_device front_camera_sys_device = {
 	.id = 1,
	.cls = &camera_sysdev_class,
};


/*-----------------------------------------------------------------------------------------
 *
 * GLOBAL VARIABLE DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static uint16_t g_back_sensor_id = 0;
static uint16_t g_front_sensor_id = 0;
/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DECLARATION
 *
 *----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static ssize_t back_sensorinfo_show_id(struct sys_device *dev,
                                        struct sysdev_attribute *attr,
                                        char *buf)
{
    return snprintf(buf, PAGE_SIZE, "0x%x\n", g_back_sensor_id);
}

static ssize_t front_sensorinfo_show_id(struct sys_device *dev,
                                        struct sysdev_attribute *attr,
                                        char *buf)
{
    return snprintf(buf, PAGE_SIZE, "0x%x\n", g_front_sensor_id);

}

//added by zhang.yu_1 for display module vendor
#if defined(CONFIG_IMX091)
extern uint16_t  imx091_get_module_vendor(void);
#endif

static ssize_t back_sensorinfo_show_name(struct sys_device *dev,
                                            struct sysdev_attribute *attr,
                                            char *buf)
{
    char sensor_name[MAX_NAME_LENGTH] = {0x00};
    #if defined(CONFIG_IMX091)	
    uint16_t vendor = 0 ;  // modified by zhang.yu_1 for display module vendor 
    #endif

    switch(g_back_sensor_id)
    {
        case SENSOR_INFO_MT9M114_MODEL_ID:
            sprintf(sensor_name, "MT9M114-1.3Mp-FF");
            break;
       case SENSOR_INFO_OV5640_MODEL_ID:
            sprintf(sensor_name, "OV5640-5.0Mp-AF");
            break;
        case SENSOR_INFO_OV8825_MODEL_ID:
            sprintf(sensor_name, "OV8825-8.0Mp-AF");
            break;
	 case SENSOR_INFO_OV7692_MODEL_ID:
            sprintf(sensor_name, "OV7692-0.3Mp-FF");
            break;	
	 case SENSOR_INFO_OV9740_MODEL_ID:
            sprintf(sensor_name, "OV9740-1.3Mp-FF");
            break;
	case SENSOR_INFO_IMX091_MODEL_ID:
	  /* modified by zhang.yu_1 for display module vendor */
           #if defined(CONFIG_IMX091)
           vendor = imx091_get_module_vendor();
	  if(vendor == 0x01)
	           sprintf(sensor_name, "IMX091-%s-13Mp-AF", "SUNNY");
	  else if(vendor == 0x31 || vendor == 0xA0)
	           sprintf(sensor_name, "IMX091-%s-13Mp-AF", "MCNEX");	
	  else
            sprintf(sensor_name, "IMX091-13Mp-AF");
	  #else
            sprintf(sensor_name, "IMX091-13Mp-AF");	  
	  #endif
            break;		
        default:
            sprintf(sensor_name, "No sensor or error ID!");
            break;
    }

    return snprintf(buf, PAGE_SIZE, "%s\n", sensor_name);
}


static ssize_t front_sensorinfo_show_name(struct sys_device *dev,
                                            struct sysdev_attribute *attr,
                                            char *buf)
{
    char sensor_name[MAX_NAME_LENGTH] = {0x00};

    switch(g_front_sensor_id)
    {
        case SENSOR_INFO_MT9M114_MODEL_ID:
            sprintf(sensor_name, "MT9M114-1.3Mp-FF");
            break;
       case SENSOR_INFO_OV5640_MODEL_ID:
            sprintf(sensor_name, "OV5640-5.0Mp-AF");
            break;
        case SENSOR_INFO_OV8825_MODEL_ID:
            sprintf(sensor_name, "OV8825-8.0Mp-AF");
            break;
	 case SENSOR_INFO_OV7692_MODEL_ID:
            sprintf(sensor_name, "OV7692-0.3Mp-FF");
            break;	
	 case SENSOR_INFO_OV9740_MODEL_ID:
            sprintf(sensor_name, "OV9740-1.3Mp-FF");
            break;
        default:
            sprintf(sensor_name, "No sensor or error ID!");
            break;
    }

    return snprintf(buf, PAGE_SIZE, "%s\n", sensor_name);
}
/*
 * LIJING_CAM_20100613
 * modify file permission from 0400->0404
 */
static struct sysdev_attribute back_sensorinfo_files[] = {
    _SYSDEV_ATTR(id, 0404, back_sensorinfo_show_id, NULL),
    _SYSDEV_ATTR(name, 0404, back_sensorinfo_show_name, NULL),
};

static struct sysdev_attribute front_sensorinfo_files[] = {
    _SYSDEV_ATTR(id, 0404, front_sensorinfo_show_id, NULL),
    _SYSDEV_ATTR(name, 0404, front_sensorinfo_show_name, NULL),
};

static void sensorinfo_create_files(struct sys_device *dev,
                                        struct sysdev_attribute files[],
                                        int size)
{
    int i;

    for (i = 0; i < size; i++) {
        int err = sysdev_create_file(dev, &files[i]);
        if (err) {
            pr_err("%s: sysdev_create_file(%s)=%d\n",
                   __func__, files[i].attr.name, err);
            return;
        }
    }
}


DEFINE_SEMAPHORE(set_back_sensor_id_sem);
void msm_sensorinfo_set_back_sensor_id(uint16_t id)
{
    down(&set_back_sensor_id_sem);
    g_back_sensor_id = id;
    up(&set_back_sensor_id_sem);
}
EXPORT_SYMBOL(msm_sensorinfo_set_back_sensor_id);


DEFINE_SEMAPHORE(set_front_sensor_id_sem);
void msm_sensorinfo_set_front_sensor_id(uint16_t id)
{
    down(&set_front_sensor_id_sem);
    g_front_sensor_id = id;
    up(&set_front_sensor_id_sem);
    
    
}
EXPORT_SYMBOL(msm_sensorinfo_set_front_sensor_id);
/*
 * Attention:
 *
 * Path of camera's sysdev: /sys/devices/system/camera/camera0
 */
static int __init sensorinfo_init(void)
{
    int err;

    err = sysdev_class_register(&camera_sysdev_class);
    if (err) {
        pr_err("%s: sysdev_class_register fail (%d)\n",
               __func__, err);
        return -EFAULT;
    }

    err = sysdev_register(&back_camera_sys_device);
    if (err) {
        pr_err("%s: sysdev_register fail (%d)\n",
               __func__, err);
        return -EFAULT;
    }
	
    sensorinfo_create_files(&back_camera_sys_device, back_sensorinfo_files,
    ARRAY_SIZE(back_sensorinfo_files));
     
     err = sysdev_register(&front_camera_sys_device);
    if (err) {
        pr_err("%s: sysdev_register fail (%d)\n",
               __func__, err);
        return -EFAULT;
    }

    sensorinfo_create_files(&front_camera_sys_device, front_sensorinfo_files,
    ARRAY_SIZE(front_sensorinfo_files));

    return 0;
}
module_init(sensorinfo_init);

