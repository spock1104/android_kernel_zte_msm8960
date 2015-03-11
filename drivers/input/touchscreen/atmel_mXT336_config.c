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
#include <mach/vreg.h>


struct atmel_config_data atmel_data = {
	.version = 0x16,
	.source = 1,
	.abs_x_min = 0,
	.abs_y_min = 0,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 15,
	//.gpio_irq = 55,
	//.power = atmel_platform_power,
	.config_T7[0]=0x14,
	.config_T7[1]=0x0c,
	.config_T7[2]=0x19,
	.config_T7[3]=0x03,
	
	.config_T8[0]=0x1e,
	.config_T8[1]=0,
	.config_T8[2]=0x0a,
	.config_T8[3]=0x0a,
	.config_T8[4]=0,
	.config_T8[5]=0,
	.config_T8[6]=255,
	.config_T8[7]=1,
	.config_T8[8]=0,
	.config_T8[9]=0,
	
	.config_T9[0]=0x8b,
	.config_T9[1]=0,
	.config_T9[2]=0,
	.config_T9[3]=0x18,
	.config_T9[4]=0x0e,
	.config_T9[5]=0,
	.config_T9[6]=0x70,
	.config_T9[7]=0x28,
	.config_T9[8]=2,
	.config_T9[9]=3,
	.config_T9[10]=10,
	.config_T9[11]=3,
	.config_T9[12]=3,
	.config_T9[13]=0,
	.config_T9[14]=0x0a,
	.config_T9[15]=20,
	.config_T9[16]=20,
	.config_T9[17]=15,
	/*ergate*/
	.config_T9[18]=0x4f,
	.config_T9[19]=0x5,
	.config_T9[20]=0xd0,
	.config_T9[21]=0x2,
	.config_T9[22]=6,
	.config_T9[23]=6,
	.config_T9[24]=12,
	.config_T9[25]=10,
	.config_T9[26]=224,
	.config_T9[27]=32,
	.config_T9[28]=144,
	.config_T9[29]=69,
	.config_T9[30]=20,
	.config_T9[31]=10,
	.config_T9[32]=55,
	.config_T9[33]=55,
	.config_T9[34]=1,
	.config_T9[35]=0,

	.config_T15[0]=0,
	.config_T15[1]=0,
	.config_T15[2]=0,
	.config_T15[3]=0,
	.config_T15[4]=0,
	.config_T15[5]=0,
	.config_T15[6]=0,
	.config_T15[7]=0,
	.config_T15[8]=0,
	.config_T15[9]=0,
	.config_T15[10]=0,

	.config_T18[0]=0,
	.config_T18[1]=0,
	
	.config_T19[0]=0,
	.config_T19[1]=0,
	.config_T19[2]=0,
	.config_T19[3]=0,
	.config_T19[4]=0,
	.config_T19[5]=0,


	.config_T23[0]=0,
	.config_T23[1]=0,
	.config_T23[2]=0,
	.config_T23[3]=0,
	.config_T23[4]=0,
	.config_T23[5]=0,
	.config_T23[6]=0,
	.config_T23[7]=0,
	.config_T23[8]=0,
	.config_T23[9]=0,
	.config_T23[10]=0,
	.config_T23[11]=0,
	.config_T23[12]=0,
	.config_T23[13]=0,
	.config_T23[14]=0,

	.config_T25[0]=0,
	.config_T25[1]=0,
	.config_T25[2]=0,
	.config_T25[3]=0,
	.config_T25[4]=0,
	.config_T25[5]=0,
	.config_T25[6]=0,
	.config_T25[7]=0,
	.config_T25[8]=0,
	.config_T25[9]=0,
	.config_T25[10]=0,
	.config_T25[11]=0,
	.config_T25[12]=0,
	.config_T25[13]=0,
	.config_T25[14]=0,

	.config_T40[0]=0,
	.config_T40[1]=0,
	.config_T40[2]=0,
	.config_T40[3]=0,
	.config_T40[4]=0,

	.config_T42[0]=33,
	.config_T42[1]=30,
	.config_T42[2]=40,
	.config_T42[3]=20,
	.config_T42[4]=0,
	.config_T42[5]=0,
	.config_T42[6]=5,
	.config_T42[7]=5,
	.config_T42[8]=5,
	.config_T42[9]=0,

	.config_T46[0]=0,
	.config_T46[1]=0,
	.config_T46[2]=16,
	.config_T46[3]=16,
	.config_T46[4]=0,
	.config_T46[5]=0,
	.config_T46[6]=0,
	.config_T46[7]=0,
	.config_T46[8]=0,
	.config_T46[9]=1,

	.config_T47[0]=0,
	.config_T47[1]=0,
	.config_T47[2]=0,
	.config_T47[3]=0,
	.config_T47[4]=0,
	.config_T47[5]=0,
	.config_T47[6]=0,
	.config_T47[7]=0,
	.config_T47[8]=0,
	.config_T47[9]=0,
	.config_T47[10]=0,
	.config_T47[11]=0,
	.config_T47[12]=0,
	
	.config_T55[0]=0,
	.config_T55[1]=0,
	.config_T55[2]=0,
	.config_T55[3]=0,
	.config_T55[4]=0,
	.config_T55[5]=0,

	/*ergate*/
	.config_T56[0]=1,
	.config_T56[1]=0,
	.config_T56[2]=0,
	.config_T56[3]=0x18,
	.config_T56[4]=0x05,
	.config_T56[5]=0x05,
	.config_T56[6]=0x05,
	.config_T56[7]=0x05,
	.config_T56[8]=0x05,
	.config_T56[9]=0x05,
	.config_T56[10]=0x05,
	.config_T56[11]=0x05,
	.config_T56[12]=0x05,
	.config_T56[13]=0x05,
	.config_T56[14]=0x05,
	.config_T56[15]=0x05,
	.config_T56[16]=0x05,
	.config_T56[17]=0x05,
	.config_T56[18]=0x05,
	.config_T56[19]=0x05,
	.config_T56[20]=0x05,
	.config_T56[21]=0x05,
	.config_T56[22]=0x05,
	.config_T56[23]=0x05,
	.config_T56[24]=0x05,
	.config_T56[25]=0x05,
	.config_T56[26]=0x05,
	.config_T56[27]=0x05,
	.config_T56[28]=0,
	.config_T56[29]=0,
	/*ergate*/
	.config_T56[30]=1,
	.config_T56[31]=2,
	.config_T56[32]=2,
	.config_T56[33]=2,
	/*ergate end*/
	.config_T56[34]=0,
	.config_T56[35]=0,
	.config_T56[36]=0,
	.config_T56[37]=0,
	.config_T56[38]=0,
	.config_T56[39]=0,
	.config_T56[40]=0,
	.config_T56[41]=0,

	.config_T57[0]=0x63,
	.config_T57[1]=0,
	.config_T57[2]=0,

	.config_T61[0]=0,
	.config_T61[1]=0,
	.config_T61[2]=0,
	.config_T61[3]=0,
	.config_T61[4]=0,
	.config_T61[5]=0,
	.config_T61[6]=0,
	.config_T61[7]=0,
	.config_T61[8]=0,
	.config_T61[9]=0,

	.config_T62[0]=0x01,
	.config_T62[1]=0x03,
	.config_T62[2]=0,
	.config_T62[3]=0x06,
	.config_T62[4]=0,
	.config_T62[5]=0,
	.config_T62[6]=0,
	.config_T62[7]=0,
	.config_T62[8]=0x64,
	.config_T62[9]=0,
	.config_T62[10]=0x03,
	.config_T62[11]=0x09,
	.config_T62[12]=0x0f,
	.config_T62[13]=0x17,
	.config_T62[14]=0x05,
	.config_T62[15]=0,
	.config_T62[16]=0x0a,
	.config_T62[17]=0x05,
	.config_T62[18]=0x05,
	.config_T62[19]=0x60,
	.config_T62[20]=0x10,
	.config_T62[21]=0x10,
	.config_T62[22]=0x34,
	.config_T62[23]=0x3e,
	.config_T62[24]=0x3f,
	.config_T62[25]=0,
	.config_T62[26]=0,
	.config_T62[27]=0,
	.config_T62[28]=0,
	.config_T62[29]=0,
	.config_T62[30]=0,
	.config_T62[31]=0,
	.config_T62[32]=0,
	.config_T62[33]=0,
	.config_T62[34]=0x40,
	.config_T62[35]=0x46,
	.config_T62[36]=0x02,
	.config_T62[37]=0,
	.config_T62[38]=0,
	.config_T62[39]=0,
	.config_T62[40]=0x0a,
	.config_T62[41]=0,
	.config_T62[42]=0,
	.config_T62[43]=0,
	.config_T62[44]=0,
	.config_T62[45]=0,
	.config_T62[46]=0,
	.config_T62[47]=0,
	.config_T62[48]=0,
	.config_T62[49]=0,
	.config_T62[50]=0,
	.config_T62[51]=0,
	.config_T62[52]=0xf,
	.config_T62[53]=0,
	
	.object_crc[0]=1,
	.object_crc[1]=1,
	.object_crc[2]=1,

	.cable_config[0] = 30,
	.cable_config[1] = 20,
	.cable_config[2] = 4,
	.cable_config[3] = 8,
	.GCAF_level[0] = 4,
	.GCAF_level[1] = 16,
	.GCAF_level[2] = 0,
	.GCAF_level[3] = 0,
	.filter_level[0] = 100,
	.filter_level[1] = 100,
	.filter_level[2] = 100,
	.filter_level[3] = 100,
	.display_width = 1024,  /* display width in pixel */
	.display_height = 1024, /* display height in pixel */
};
