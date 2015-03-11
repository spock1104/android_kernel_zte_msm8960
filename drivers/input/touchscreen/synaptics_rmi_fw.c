//#include <linux/input/SynaUpgrade.h>
#include <linux/input/synaptics_rmi.h>
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#include <linux/file.h>
#include <asm/uaccess.h>

#include <linux/fs.h>
#include <linux/fb.h>

extern int syna_update_flag ;

static struct i2c_client *syna_i2c_client;

#define FW_PATH_SDCARD	"/sdcard/"
#define FW_PATH_ETC		"/system/etc/firmware/"


static bool fw_update_mode;

//unsigned char Firmware_Image[16000];  // make smaller and dynamic
//unsigned char Config_Image[16000];  // make smaller and dynamic

unsigned char page_data[200];
unsigned char status;
unsigned long firmware_imgsize;
unsigned char firmware_imgver;
unsigned long config_imgsize;
//unsigned long filesize;
unsigned int synaptics_bootload_id;

unsigned char *firmware_imgdata = NULL;
unsigned char *config_imgdata = NULL;
unsigned short firmware_blocksize;
unsigned short firmware_blockcount;
unsigned short config_blocksize;
unsigned short config_blockcount;

unsigned long firmware_imgchecksum;

static const unsigned char f34_reflash_cmd_firmware_crc   = 0x01;
static const unsigned char f34_reflash_cmd_firmware_write = 0x02;
static const unsigned char f34_reflash_cmd_erase_all      = 0x03;
static const unsigned char f34_reflash_cmd_config_read    = 0x05;
static const unsigned char f34_reflash_cmd_config_write   = 0x06;
static const unsigned char f34_reflash_cmd_config_erase   = 0x07;
static const unsigned char f34_reflash_cmd_enable        = 0x0f;
static const unsigned char f34_reflash_cmd_normal_result  = 0x80; 

unsigned short f01_RMI_CommandBase;
unsigned short f01_RMI_DataBase;
unsigned short f01_RMI_QueryBase;
unsigned short f01_RMI_IntStatus;


unsigned int synaptics_bootload_imgid;
unsigned short f34_reflash_datareg;
unsigned short f34_reflash_blocknum;
unsigned short f34_reflash_blockdata;
unsigned short f34_reflash_query_bootid;

unsigned short f34_reflash_query_flashpropertyquery;//!!!!Important!!!!determine how to calculate F34 register map layout	
unsigned short f34_reflash_query_firmwareblocksize;
unsigned short f34_reflash_query_firmwareblockcount;
unsigned short f34_reflash_query_configblocksize;
unsigned short f34_reflash_query_configblockcount;

unsigned short f34_reflash_flashcontrol;
unsigned short f34_reflash_blocknum;
unsigned short f34_reflash_blockdata;

bool  flash_prog_on_startup;
bool  unconfigured;

//following functions defined in kernel/drivers/input/touchscreen/synaptics_i2c_rmi.c
//extern int RMI4ReadBootloadID(struct i2c_client *client);
//extern int synaptics_get_pgt_f34(struct i2c_client *client);

struct rmi_function_descriptor f34_func_des;
struct rmi_function_descriptor f01_func_des;


static int syna_clear_intr(struct i2c_client * client )
{
	char status;

	if ( !client )
		return 0;

	status = i2c_smbus_read_byte_data(client, f01_RMI_IntStatus );
	printk("%s, status = 0x%x\n",__func__,status);

	return status;	
}

static void RMI4WaitATTN(int msec)
{
	int ret =0;
	int uErrorCount = 0;
	int errorCount = 300;
	
	msleep(msec);
	
	do{
		ret = i2c_smbus_read_i2c_block_data(syna_i2c_client, f34_reflash_flashcontrol, 1, &page_data[0]);
//		printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, page_data[0]);

	    if((ret < 0) && uErrorCount < errorCount) {
			//mdelay(100);
			msleep(10);
			uErrorCount++;
			page_data[0] = 0;
			continue;
	    } else {
			uErrorCount++;
	    }

		// clear intr
		ret = i2c_smbus_read_i2c_block_data(syna_i2c_client, f01_RMI_IntStatus, 1, &status);

		if ( page_data[0] != 0x80) {
	        printk("%s  page_data[0]:0x%x\n", __func__, page_data[0]);
	        //mdelay(2);
	        ret = 2;
	        break;
		}
	} while((page_data[0]!=0x80) && (uErrorCount < errorCount) );

	return;
}

static bool synaptics_read_flash_property(struct i2c_client *client)
{
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_flashpropertyquery, 1, &page_data[0]);
	if(0 > ret){
		pr_err("%s: i2c read flash property error!\n", __func__);
		return -1;
	}

	pr_info("Flash Property Query = 0x%x\n", page_data[0]);
	return ((page_data[0] & 0x01) == 0x01);
}

//Func34 has 2 models, we should make sure which model the TP used
static void synaptics_set_flash_addr(struct i2c_client *client)
{
	if (synaptics_read_flash_property(client)){
		pr_info("Flash Property  1\n");
		f34_reflash_flashcontrol = f34_func_des.data_base + firmware_blocksize + 2;
		f34_reflash_blocknum = f34_func_des.data_base;
		f34_reflash_blockdata = f34_func_des.data_base + 2;
	}else{
		pr_info("Flash Property  2\n");
		f34_reflash_flashcontrol = f34_func_des.data_base;
		f34_reflash_blocknum = f34_func_des.data_base + 1;
		f34_reflash_blockdata = f34_func_des.data_base + 3;
	}
}

static int synaptics_get_pgt_f34(struct i2c_client *client)
{
	int ret=0;
	struct rmi_function_descriptor Buffer;
	unsigned short uAddress;

	f01_func_des.function_number= 0;
	f34_func_des.function_number = 0;
	//m_BaseAddresses.m_ID = 0xff;

	for(uAddress = 0xe9; uAddress > 10; uAddress -= sizeof(struct rmi_function_descriptor))
	{
	  ret = i2c_smbus_read_i2c_block_data(client, uAddress, sizeof(Buffer),(uint8_t *)&Buffer);
	  if(0 > ret){
		  pr_err("%s: i2c read F34 pgt error!\n", __func__);
		  return -1;
	  	}else
	  	{
	  		pr_info("%s: data: query base address:%d data base address:%d .\n", __func__, Buffer.query_base,Buffer.data_base);	
	  	}

	  switch(Buffer.function_number)
	  {
		case 0x34:
		  f34_func_des= Buffer;
		  break;
		case 0x01:
		  f01_func_des = Buffer;
		  break;
	  }

	  if(Buffer.function_number == 0)
	  {
		break;
	  }
	  else
	  {
		printk("Function $%02x found.\n", Buffer.function_number);
	  }
	}


    f01_RMI_DataBase= f01_func_des.data_base;
    f01_RMI_IntStatus = f01_func_des.data_base + 1;
    f01_RMI_CommandBase = f01_func_des.cmd_base;
    f01_RMI_QueryBase = f01_func_des.query_base;	
  
	f34_reflash_datareg = f34_func_des.data_base;
	f34_reflash_blocknum = f34_func_des.data_base;
	f34_reflash_blockdata = f34_func_des.data_base + 2;
	
	f34_reflash_query_bootid = f34_func_des.query_base;
	f34_reflash_query_flashpropertyquery = f34_func_des.query_base + 2;	
	f34_reflash_query_firmwareblocksize = f34_func_des.query_base + 3;
	f34_reflash_query_firmwareblockcount = f34_func_des.query_base + 5;
	f34_reflash_query_configblocksize = f34_func_des.query_base + 3;
	f34_reflash_query_configblockcount = f34_func_des.query_base + 7;
	
  	synaptics_set_flash_addr(client);
	return 0;
}

static int synaptics_read_bootload_id(struct i2c_client *client)
{
	int ret = 0;
	char data[2];

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_bootid, 2, data);
	if(0 > ret){
		pr_err("%s: i2c read bootload id error!\n", __func__);
		return -1;
	}
	
	synaptics_bootload_id = (unsigned int)data[0] + (unsigned int)data[1]*0x100;
	printk("%s bootload id: 0x%x, DATA[0]:0x%x, DATA[1]:0x%x", __func__, synaptics_bootload_id,data[0],data[1]);

	return 0;
}

static int synaptics_write_bootload_id(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
  
	data[0] = synaptics_bootload_id%0x100;
	data[1] = synaptics_bootload_id/0x100;
	printk(" write synaptics_bootload_id = 0x%x, data[0]=0x%x, data[1]=0x%x \n",synaptics_bootload_id,data[0],data[1]);
	ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, 2,data);
  	if(0 != ret){
		pr_err("%s: i2c write bootload id error!\n", __func__);
		return -1;
	}
	return 0;
}


/*
return value
0, success
-1, fail
*/
//Check wether the TP support F34 and try to enter bootload mode
static int synaptics_write_page(struct i2c_client *client)
{
	int ret = 0;
	unsigned char upage = 0x00;
	unsigned char f01_rmi_data[2];
	unsigned char status;

	//wirte 0x00 to Page Select register
	//ret = i2c_smbus_write_byte_data(client, 0xff, upage);
	ret = i2c_smbus_write_i2c_block_data(client, 0xff, 1, &upage);
	if(0 > ret){
		pr_err("%s: i2c read flash property error!\n", __func__);
		return -1;
	}
	
	do{
		ret = i2c_smbus_read_i2c_block_data(client, 0x00, 1, &status);
		//ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &status);
		if(0 > ret){
			pr_err("%s: i2c read f34 flash data0 error!\n", __func__);
			return -1;
		}

		if(status & 0x40){
			flash_prog_on_startup = true;
		}

		if(status & 0x80){
			unconfigured = true;
			break;
		}

		pr_info("%s: Status is 0x%x\n", __func__, status);

	} while(status & 0x40);

	if ( flash_prog_on_startup && ! unconfigured ){
		pr_info("%s: Bootloader running!\n", __func__);
	}else if(unconfigured){
		pr_info("%s: UI running\n", __func__);
	}

	synaptics_get_pgt_f34(client);

	if(f34_func_des.function_number == 0){
		pr_err("%s: Func 34 is not supported\n", __func__);
		return -1;
	}

	pr_info("Func 34 addresses Control base:$%02x Query base: $%02x.\n", f34_func_des.ctrl_base, f34_func_des.query_base);

	if(f01_func_des.function_number == 0){
		pr_err("%s: Func 01 is not supported, its members will be set here!\n", __func__);
		f01_func_des.function_number = 0x01;
		f01_func_des.data_base = 0;
		return -1;
	}
	pr_info("Func 01 addresses Control base:$%02x Query base: $%02x.\n", f01_func_des.ctrl_base, f01_func_des.query_base);

	// Get device status
	ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, sizeof(f01_rmi_data), &f01_rmi_data[0]);
	if(0 > ret){
		pr_err("%s: i2c read f01_func_des.data_base error!\n", __func__);
		return -1;
	}

	// Check Device Status
	pr_info("%s: Configured: %s\n", __func__, f01_rmi_data[0] & 0x80 ? "false" : "true");
	pr_info("%s: FlashProg:  %s\n", __func__, f01_rmi_data[0] & 0x40 ? "true" : "false");
	pr_info("%s: StatusCode: 0x%x \n", __func__, f01_rmi_data[0] & 0x0f );
	return 0;
}

static int synaptics_enable_flash_command(struct i2c_client *client)
{
  return i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, (unsigned char *)&f34_reflash_cmd_enable);
}

/*
return value
0, success
-1, fail
*/
static int synaptics_enable_flash(struct i2c_client *client)
{
//	unsigned char data[2]={0,0};
	int ret = 0;
	int count = 0;

	// Read bootload ID
	ret = synaptics_read_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Read bootload err!\n", __func__);
		return -1;
	}
	
	// Write bootID to block data registers
	ret = synaptics_write_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Write bootload err!\n", __func__);
		return -1;
	}
	
	do {
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, (uint8_t *)&page_data[0]);

		// To deal with ASIC physic address error from cdciapi lib when device is busy and not available for read
		if(0>ret && count < 300)
		{
		  count++;
		  page_data[0] = 0;
		  continue;
		}

		// Clear the attention assertion by reading the interrupt status register
		ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &status);
		if(0 > ret){
			pr_err("%s: Read Func34 flashcontrol err!\n", __func__);
			return -1;
		}

	} while(((page_data[0] & 0x0f) != 0x00) && (count <= 300));

	// Issue Enable flash command
	ret = synaptics_enable_flash_command(client);
	if(0 != ret){
		pr_err("%s: Enable flash failed!\n", __func__);
		return -1;
	}

	RMI4WaitATTN(2);
	
	synaptics_get_pgt_f34(client);

	return 0;
}


static unsigned long synaptics_extract_long_from_header(const unsigned char* SynaImage)  // Endian agnostic
{
  return((unsigned long)SynaImage[0] +
         (unsigned long)SynaImage[1]*0x100 +
         (unsigned long)SynaImage[2]*0x10000 +
         (unsigned long)SynaImage[3]*0x1000000);
}

/*
return value
0, success
-1, fail
*/
static int synaptics_read_config_info(struct i2c_client *client)
{
  unsigned char data[2];
  int ret;
  
  //ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockSize, &uData[0], 2);
  ret = i2c_smbus_read_i2c_block_data(client,f34_reflash_query_configblocksize,2,data);
  if(0 > ret){
	pr_err("%s: Read config block size err!\n", __func__);
	return -1;
  }
  
  config_blocksize = data[0] | (data[1] << 8);

  //m_ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockCount, &uData[0], 2);
  ret = i2c_smbus_read_i2c_block_data(client,f34_reflash_query_configblockcount,2,data);
	  if(0 > ret){
		pr_err("%s: Read config block count err!\n", __func__);
		return -1;
	  }
	  
  config_blockcount = data[0] | (data[1] << 8);  
  config_imgsize = config_blocksize*config_blockcount;
    
  pr_info("%s, config block size :%d, config block count :%d, config imge size : 0x%x\n",
  	__func__, config_blocksize, config_blockcount, config_blocksize*config_blockcount);
  return 0;
}

/*
return value
0, success
-1, fail
*/
static int synaptics_read_firmware_info(struct i2c_client *client)
{
	unsigned char data[2];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_firmwareblocksize, 2, data);
	if(0 > ret){
		pr_err("%s: Read firmware block size err!\n", __func__);
		return -1;
	}

	firmware_blocksize = data[0] | (data[1] << 8);
	
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_firmwareblockcount, 2, data);
	if(0 > ret){
		pr_err("%s: Read firmware block count err!\n", __func__);
		return -1;
	}

	firmware_blockcount = data[0] | (data[1] << 8);
	firmware_imgsize = firmware_blockcount * firmware_blocksize;

	pr_info("%s: Firmware block size:%d ,firmware block count:%d, ,fw img size=%lu\n",
		__func__, firmware_blocksize, firmware_blockcount, firmware_imgsize);
	return 0;
}


static void synaptics_cal_checksum(unsigned short * data, unsigned short len, unsigned long * dataBlock)
{
  unsigned long temp = *data++;
  unsigned long sum1;
  unsigned long sum2;

  *dataBlock = 0xffffffff;

  sum1 = *dataBlock & 0xFFFF;
  sum2 = *dataBlock >> 16;

  while (len--)
  {
    sum1 += temp;
    sum2 += sum1;
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }

  *dataBlock = sum2 << 16 | sum1;
}

static unsigned short synaptics_get_frimware_size(void)
{
  return firmware_blocksize * firmware_blockcount;
}

static unsigned short synaptics_get_config_size(void)
{
  return config_blocksize * config_blockcount;
}



/*
return value
0, not match
1, match
*/
static int synaptics_check_config_id(struct i2c_client *client, u8* pfwfile, bool mode )
{
	struct synaptics_rmi_data *ts = i2c_get_clientdata(client);
	u8 chipid, vid, fwvid;


	if ( !client  || ! pfwfile )
		return 0;
	
	if ( mode == true )	// force update
		return 1;

	// get fw vid from fw file
	firmware_imgsize = synaptics_extract_long_from_header(&(pfwfile[8]));
	config_imgdata = (unsigned char*) ( &pfwfile[0] + 0x100 + firmware_imgsize );
	fwvid = *(config_imgdata+1);

	chipid = ts->config_id.chip_type;
	vid = ts->config_id.sensor;

	pr_info("current chip type id = 0x%x(%c)\n", chipid, chipid );
	pr_info("current sensor partner id = 0x%x(%c)\n", vid, vid );
	pr_info("fw file sensor partner id = 0x%x(%c)\n", fwvid, fwvid );

	if( fwvid != vid){
		pr_info("module id dismatch!\n");
		return 0;
	}else{
		pr_info("module id matched!\n");
		return 1;
	}

}

/*
return value
0, success
-1, fail
*/
static int synaptics_read_firmware_header( struct i2c_client *client,u8 *pfwfile, u32 filesize)
{
	unsigned long check_sum;
	//unsigned char data;
	int ret;
	//unsigned long filesize;

	if ( !client || !pfwfile ){
		printk("%s: invalid param!\n", __func__);
		return -1;
	}
	//filesize = sizeof(SynaFirmware) -1;
	filesize --;

	pr_info("\n%s:Scanning SynaFirmware[], header file - len = 0x%x \n\n", __func__, filesize);

	check_sum = synaptics_extract_long_from_header(&(pfwfile[0]));
	synaptics_bootload_imgid = (unsigned int)pfwfile[4] + (unsigned int)pfwfile[5]*0x100;
	firmware_imgver = pfwfile[7];
	firmware_imgsize = synaptics_extract_long_from_header(&(pfwfile[8]));
	config_imgsize = synaptics_extract_long_from_header(&(pfwfile[12]));
	
	pr_info("%s: Target = %s, ", __func__, &pfwfile[16]);
	pr_info("Cksum = 0x%lx, Id = 0x%x, Ver = 0x%x, FwSize = 0x%lx, ConfigSize = 0x%lx \n",
	check_sum, synaptics_bootload_imgid, firmware_imgver, firmware_imgsize, config_imgsize);

	 // Determine firmware organization - read firmware block size and firmware size
	 ret = synaptics_read_firmware_info(client);
	if(ret < 0)
		return -1;

	synaptics_cal_checksum((unsigned short*)&(pfwfile[4]), (unsigned short)(filesize-4)>>1,
						&firmware_imgchecksum);
#if 0
	if (filesize != (0x100+firmware_imgsize+config_imgsize))
	{
		pr_err("%s: Error--SynaFirmware[] size = %d, expected %ld\n", __func__, filesize, (0x100+firmware_imgsize+config_imgsize));
		while(1);
	}
#endif
	if (firmware_imgsize != synaptics_get_frimware_size())
	{
		pr_err("%s: Firmware image size verfication failed!\n", __func__);
		pr_err("\tsize in image 0x%lx did not match device size 0x%x\n", firmware_imgsize, synaptics_get_frimware_size());
		//while(1);
		return -1;
	}

	if (config_imgsize != synaptics_get_config_size())
	{
		pr_err("%s: Configuration size verfication failed!\n", __func__);
		pr_err("\tsize in image 0x%lx did not match device size 0x%x\n", config_imgsize, synaptics_get_config_size());
		//while(1);
		return -1;
	}

	firmware_imgdata=(unsigned char *)((&pfwfile[0]) + 0x100);
	config_imgdata = (unsigned char*) ( &pfwfile[0] + 0x100 + firmware_imgsize );
	// memcpy(m_firmwareImgData, (&SynaFirmware[0])+0x100, firmware_imgsize);
//	memcpy(config_imgdata,   (&pfwfile[0])+0x100+firmware_imgsize, config_imgsize);
//	printk("config img data position = %lu, size = %lu, 0= 0x%x, 1= 0x%x\n",
//		0x100+firmware_imgsize, config_imgsize, config_imgdata[0],config_imgdata[1]);

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &page_data[0]);
	if(0 > ret){
		pr_err("%s: Read Func34 flash control err!\n", __func__);
		return -1 ;
	}
	return 0;
}


static bool synaptics_validate_bootload_id(unsigned short bootloadID, struct i2c_client *client)
{
	int ret = 0;
	pr_info("%s: called!\n", __func__);
	ret = synaptics_read_bootload_id(client);
	if(0 != ret){
		pr_err("%s: read bootload id err!\n", __func__);
		return false;
	}
	
	pr_info("Bootload ID of device: 0x%x, input bootID: 0x%x\n", synaptics_bootload_id, bootloadID);

	// check bootload ID against the value found in firmware--but only for image file format version 0
	return firmware_imgver != 0 || bootloadID == synaptics_bootload_id;
}

static int synaptics_issue_erase_cmd(unsigned char *command, struct i2c_client *client)
{
  int ret;
  // command = 3 - erase all; command = 7 - erase config
  ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, command);

  return ret;
}

static int synaptics_flash_firmware_write(struct i2c_client *client)
{
	unsigned char *buf_firmware_data = firmware_imgdata;
	unsigned char data[2];
	int ret = 0;
	unsigned short blocknum;

	
	pr_info("Synaptics Flash Firmware startshere!\n");

	for(blocknum = 0; blocknum < firmware_blockcount; ++blocknum){

		data[0] = blocknum & 0xff;
		data[1] = (blocknum & 0xff00) >> 8;

		// Write Block Number
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blocknum, 2, data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash blocknum id err, count:%d!\n", __func__, blocknum);
			return -1;
		}

		// Write Data Block
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, firmware_blocksize, buf_firmware_data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash block data err, count:%d!\n", __func__, blocknum);
			return -1;
		}

		// Move to next data block
		buf_firmware_data += firmware_blocksize;

		// Issue Write Firmware Block command
		//m_bAttenAsserted = false;
		data[0] = 2;
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash flash control err, count:%d!\n", __func__, blocknum);
			return -1;
		}
		// Wait ATTN. Read Flash Command register and check error
		//msleep(200);
		RMI4WaitATTN(10);

	}

	pr_info("%s:Flash Firmware done!\n", __func__);

	return 0;
}

/*
return value
0, success
-1, fail
*/
static int synaptics_prog_firmware(struct i2c_client *client)
{
	int ret;
	unsigned char data[1];

	if ( !synaptics_validate_bootload_id(synaptics_bootload_imgid, client) )
	{
		pr_err("%s: Validate bootload id failed!\n", __func__);
		return -1;
	}

	// Write bootID to data block register
	ret = synaptics_write_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Write bootload id err!\n", __func__);
		return -1;
	}
	// Issue the firmware and configuration erase command
	data[0]=3;
	ret = synaptics_issue_erase_cmd(&data[0], client);
	if(0 != ret){
		pr_err("%s: Issue erase cmd err!\n", __func__);
		return -1;
	}

	RMI4WaitATTN(2000);
	
	// Write firmware image
	ret = synaptics_flash_firmware_write(client);
	if(0 != ret){
		pr_err("%s: Flash firmware write err!\n", __func__);
		return -1;
	}

	return 0;
}

static int synaptics_issue_flash_ctl_cmd(struct i2c_client *client, unsigned char *command)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, f34_reflash_flashcontrol, *command);

	return ret;
}

/*
return value
0, success
-1, fail
*/
static int synaptics_prog_configuration(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
	unsigned char *pdata = config_imgdata;
	unsigned short blocknum;
	for(blocknum = 0; blocknum < config_blockcount; blocknum++){
		data[0] =  blocknum%0x100;
		data[1] =  blocknum/0x100;

		// Write Configuration Block Number
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blocknum, 2, &data[0]);
		if(0 > ret){
			pr_err("%s: Write f34 reflash blocknum err, count:%d.\n", __func__, blocknum);
			return -1;
		}

		// Write Data Block
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, config_blocksize, pdata);
		if(0 > ret){
			pr_err("%s: Write Func34 reflash blockdata, count:%d.\n", __func__, blocknum);
			return -1;
		}

		pdata += config_blocksize;

		// Issue Write Configuration Block command to flash command register
		//m_bAttenAsserted = false;
		data[0] = f34_reflash_cmd_config_write;

		ret = synaptics_issue_flash_ctl_cmd(client, &data[0]);
		if(0 != ret){
			pr_err("%s: Flash firmware write err!\n", __func__);
			return -1;
		}

		RMI4WaitATTN(10);

	}
	return 0;
}

static int synaptics_reset_device(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[1];

	data[0] = 1;
	ret = i2c_smbus_write_i2c_block_data(client, f01_func_des.cmd_base, 1, &data[0]);
	return ret;
	
}

static int synaptics_disenable_flash(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
	unsigned int errcount = 0;

	synaptics_reset_device(client);
	RMI4WaitATTN(500);

	syna_clear_intr(client);

	do {
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &page_data[0]);
	 
		// To work around the physical address error from control bridge
		if(ret && errcount < 300)
		{
			errcount++;
			page_data[0] = 0;
			continue;
		}
		pr_info("%s: RMI4WaitATTN after errorCount loop, error count=%d\n", __func__, errcount);
	} while(((page_data[0] & 0x0f) != 0x00) && (errcount <= 300));
 

	syna_clear_intr(client);  
	printk("Checking if reflash finished!\n");


	// Read F01 Status flash prog, ensure the 6th bit is '0'
	do
	{
		ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &data[0]);
		printk("%s:Func01 data register bit 6: 0x%x\n", __func__, data[0]);
	} while((data[0] & 0x40)!= 0);

	// With a new flash image the page description table could change
	synaptics_get_pgt_f34(client);
	
	return 0;//ESuccess;
}


/*
static void syna_power_on_off(struct i2c_client *client,int on_off)
{
	//
	//int(* power)(int on);
	//power=client->dev.platform_data;
	//power(on_off);
	//gpio_direction_output(31, on_off);

	struct synaptics_rmi_data *ts = i2c_get_clientdata(client);
	
	pr_info("syna_power %d\n", on_off);
	if (ts && ts->power) ts->power(on_off);

	return;
}
unsigned short i2c_address;


static int syna_i2c_init(struct i2c_client *client)
{
	//i2c_address=client->addr;//temp
	syna_power_on_off(client,0);
	msleep(200);
	syna_power_on_off(client,1);
	msleep(200);
	//RMI4WritePage();

	return (synaptics_write_page(client));
}
*/


static int syna_rmi4_init(struct i2c_client *client)
{
	int ret=0;
	ret = synaptics_read_config_info(client);
	if(ret<0) return -1;
	
	ret = synaptics_read_firmware_info(client);
	if(ret<0) return -1;
	
	return 0;
}


static int syna_getfwsize(char * firmware_name)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize = 0; 
	char filepath[128];

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s",FW_PATH_SDCARD,firmware_name);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
		if(IS_ERR(pfile)){
			memset(filepath, 0, sizeof(filepath));
			sprintf(filepath, "%s%s", FW_PATH_ETC,firmware_name);
			pfile = filp_open(filepath, O_RDONLY, 0);
			if(IS_ERR(pfile)){
				pr_err("error occured while opening file %s.\n", filepath);
				return -1;
			}
		}
	}
	pr_info("filepath=%s\n", filepath);

	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 

	filp_close(pfile, NULL);

	return fsize;
}

static int syna_getfwinfo(char * firmware_name, unsigned char * firmware_buf)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize; 
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s",FW_PATH_SDCARD,firmware_name);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
		if(IS_ERR(pfile)){
			memset(filepath, 0, sizeof(filepath));
			sprintf(filepath, "%s%s", FW_PATH_ETC,firmware_name);
			pfile = filp_open(filepath, O_RDONLY, 0);
			if(IS_ERR(pfile)){
				pr_err("error occured while opening file %s.\n", filepath);
				return -1;
			}
		}
	}
	pr_info("filepath=%s\n", filepath);

	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 
	//char * buf;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;

	vfs_read(pfile, firmware_buf, fsize, &pos);

	filp_close(pfile, NULL);
	set_fs(old_fs);
	return 0;
}

/*
return value
0, success
-1, fail
*/
static int syna_fw_update(struct i2c_client *client,	
	char *pfwfilename /*,u32 fwsize*/)
{
	
	u32 fwsize = 0;
  	u8*   pbt_buf = 0;
	int ret=0;


	pr_info("%s: ************fw update start!*************\n", __func__);


	disable_irq(client->irq);

	// get fw file 
	fwsize = syna_getfwsize(pfwfilename);
  	pbt_buf = (unsigned char *) kmalloc(fwsize+1,GFP_ATOMIC);
	if(syna_getfwinfo(pfwfilename, pbt_buf)){
		pr_err("get firmware information failed!\n");
		ret = -1;
		goto malloc_error;
	}

	if ( !synaptics_check_config_id(client, pbt_buf, fw_update_mode) ){
		ret = -1;
		goto malloc_error;
	}

	synaptics_write_page(client);
	syna_rmi4_init( client);

	synaptics_set_flash_addr(client);
	synaptics_read_firmware_header(client, pbt_buf, fwsize);
	
	synaptics_enable_flash(client);

	synaptics_prog_firmware(client);
	
	synaptics_prog_configuration(client);
	
	synaptics_disenable_flash(client);
	ret = 0;

malloc_error:

/*	syna_power_on_off(client,0);
	msleep(500);
	syna_power_on_off(client,1);
	msleep(700);
	*/
	if (pbt_buf)
		kfree(pbt_buf);

	enable_irq(client->irq);

	pr_info("%s: fw update exit!\n", __func__);
	return ret;

}

static ssize_t syna_fwupdate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
    return -EPERM;
}

//upgrade from app.bin
static ssize_t syna_fwupdate_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	char fwname[128];
	struct i2c_client *client = syna_i2c_client;

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	fw_update_mode = false;
	if(0 == syna_fw_update( client, fwname ))
		pr_info("%s: update success \n", __func__);
	else
		pr_info("%s: update fail  \n", __func__);

	return count;
}

static DEVICE_ATTR(synafwupdate, S_IRUGO|S_IWUSR, syna_fwupdate_show, syna_fwupdate_store);


static ssize_t syna_force_fwupdate_store(struct device *dev,
						struct device_attribute *attr,
							const char *buf, size_t count)
{
	char fwname[128];
	struct i2c_client *client = syna_i2c_client;

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	fw_update_mode = true;
	if(0 == syna_fw_update( client, fwname ))
		pr_info("%s: update success \n", __func__);
	else
		pr_info("%s: update fail  \n", __func__);

	return count;
}

static DEVICE_ATTR(synafwupdate_force, S_IRUGO|S_IWUSR, syna_fwupdate_show, syna_force_fwupdate_store);



//void ioctol(struct i2c_client *client)
extern struct kobject *firmware_kobj;


/*
return value
0, success
-1, fail
*/
int syna_fwupdate(struct i2c_client *client, char *pfwfilename ) 
{
	if ( !client || !pfwfilename )
		return -1;

	fw_update_mode = false;	//temp for n970 old firmware
	return (syna_fw_update( client, pfwfilename));
}


int syna_fwupdate_init(struct i2c_client *client)
{
	int ret;
	struct kobject * fts_fw_kobj=NULL;


	if (!client)
		return 0;

	syna_i2c_client = client;
	fw_update_mode = false;

	fts_fw_kobj = kobject_get(firmware_kobj);
	if (fts_fw_kobj == NULL) {
		fts_fw_kobj = kobject_create_and_add("firmware", NULL);
		if (fts_fw_kobj == NULL) {
			pr_err("%s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
	}

 	ret=sysfs_create_file(fts_fw_kobj, &dev_attr_synafwupdate.attr);
	if (ret) {
		pr_err("%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	
	ret=sysfs_create_file(fts_fw_kobj, &dev_attr_synafwupdate_force.attr);
	if (ret) {
		pr_err("%s: sysfs_create_file failed\n", __func__);
		return ret;
	}	

	pr_info("%s:synaptics firmware update init succeed!\n", __func__);
	return 0;

}


int syna_fwupdate_deinit(struct i2c_client *client)
{
	struct kobject * fts_fw_kobj=NULL;

	fts_fw_kobj = kobject_get(firmware_kobj);
	if ( !firmware_kobj ){
		printk("%s: error get kobject\n", __func__);
		return -1;
	}
	
	sysfs_remove_file(firmware_kobj, &dev_attr_synafwupdate.attr);
	//	kobject_del(virtual_key_kobj);

	return 0;
}


