/* Himax Android Driver Sample Code for HX83100 chipset
*
* Copyright (C) 2017 Himax Corporation.
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

#include "himax_ic.h"

static unsigned char i_TP_CRC_FW_128K[]=
{
	#include "HX_CRC_128.i"
};
static unsigned char i_TP_CRC_FW_64K[]=
{
	#include "HX_CRC_64.i"
};
static unsigned char i_TP_CRC_FW_124K[]=
{
	#include "HX_CRC_124.i"
};
static unsigned char i_TP_CRC_FW_60K[]=
{
	#include "HX_CRC_60.i"
};

extern int i2c_error_count;

extern unsigned long	FW_VER_MAJ_FLASH_ADDR;
extern unsigned long 	FW_VER_MIN_FLASH_ADDR;
extern unsigned long 	CFG_VER_MAJ_FLASH_ADDR;
extern unsigned long 	CFG_VER_MIN_FLASH_ADDR;

extern unsigned long 	FW_VER_MAJ_FLASH_LENG;
extern unsigned long 	FW_VER_MIN_FLASH_LENG;
extern unsigned long 	CFG_VER_MAJ_FLASH_LENG;
extern unsigned long 	CFG_VER_MIN_FLASH_LENG;

#ifdef HX_AUTO_UPDATE_FW
	extern int g_i_FW_VER;
	extern int g_i_CFG_VER;
	extern int g_i_CID_MAJ;
	extern int g_i_CID_MIN;
	extern unsigned char i_CTPM_FW[];
#endif

extern unsigned char	IC_TYPE;
extern unsigned char	IC_CHECKSUM;

extern struct himax_ic_data* ic_data;
extern struct himax_ts_data *private_ts;

int himax_touch_data_size = 124;

#ifdef HX_TP_PROC_2T2R
bool Is_2T2R = false;
#endif

#ifdef HX_USB_DETECT_GLOBAL
//extern kal_bool upmu_is_chr_det(void);
extern void himax_cable_detect_func(bool force_renew);
#endif

int himax_get_touch_data_size(void)
{
	return himax_touch_data_size;
}

#ifdef HX_RST_PIN_FUNC
extern int himax_report_data_init(void);
extern u8 HX_HW_RESET_ACTIVATE;

void himax_pin_reset(void)
{
		I("%s: Now reset the Touch chip.\n", __func__);
		//nothing to be done, need to refer to display
}

void himax_reload_config(void)
{
	if(himax_report_data_init())
		E("%s: allocate data fail\n",__func__);
	himax_power_on_init(private_ts->client);
}

void himax_irq_switch(int switch_on)
{
	int ret = 0;
	if(switch_on)
	{
		
		if (private_ts->use_irq)
			himax_int_enable(private_ts->client->irq,switch_on);
		else
			hrtimer_start(&private_ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	else
	{
		if (private_ts->use_irq)
			himax_int_enable(private_ts->client->irq,switch_on);
		else
		{
			hrtimer_cancel(&private_ts->timer);
			ret = cancel_work_sync(&private_ts->work);
		}
	}
}

void himax_ic_reset(uint8_t loadconfig,uint8_t int_off)
{
	struct himax_ts_data *ts = private_ts;

	HX_HW_RESET_ACTIVATE = 1;

	I("%s,status: loadconfig=%d,int_off=%d\n",__func__,loadconfig,int_off);
	
	if (ts->rst_gpio)
	{
		if(int_off)
		{
			himax_irq_switch(0);
		}

		himax_pin_reset();
		if(loadconfig)
		{
			himax_reload_config();
		}
		if(int_off)
		{
			himax_irq_switch(1);
		}
	}

}
#endif


#if defined(HX_ESD_RECOVERY)
int g_zero_event_count = 0;
int himax_ic_esd_recovery(int hx_esd_event,int hx_zero_event,int length)
{
	
	if(hx_esd_event == length)
	{
		goto checksum_fail;
	}
	else if(hx_zero_event == length)
	{
		g_zero_event_count++;
		I("[HIMAX TP MSG]: ESD event ALL Zero is %d times.\n",g_zero_event_count);
		goto err_workqueue_out;
	}
	
	if(g_zero_event_count > 5)
	{
		I("[HIMAX TP MSG]: ESD event checked - ALL Zero.\n");
		goto checksum_fail;
	}
checksum_fail:
	return CHECKSUM_FAIL;
err_workqueue_out:
	return WORK_OUT;
}
void himax_esd_ic_reset(void)
{
	/*Nothing to do in incell,need to follow display reset*/
}
#endif

int himax_hand_shaking(struct i2c_client *client)    //0:Running, 1:Stop, 2:I2C Fail
{
	int result = 0;
	
	return result;
}

void himax_idle_mode(struct i2c_client *client,int disable)
{
	
}

int himax_determin_diag_rawdata(int diag_command)
{
	return diag_command%10;
}

int himax_determin_diag_storage(int diag_command)
{
	return diag_command/10;
}

int himax_switch_mode(struct i2c_client *client,int mode)
{
	return 1;
}

void himax_return_event_stack(struct i2c_client *client)
{

}

void himax_diag_register_set(struct i2c_client *client, uint8_t diag_command)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];

	if(diag_command != 0)
		diag_command = diag_command + 5;

	tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0x80;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = diag_command;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
}

void himax_flash_dump_func(struct i2c_client *client, uint8_t local_flash_command, int Flash_Size, uint8_t *flash_buffer)
{
	//struct himax_ts_data *ts = container_of(work, struct himax_ts_data, flash_work);

//	uint8_t sector = 0;
//	uint8_t page = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t out_buffer[20];
	uint8_t in_buffer[260];
	int page_prog_start = 0;
	int i = 0;

	himax_sense_off(client);
	himax_burst_enable(client, 0);
	/*=============Dump Flash Start=============*/
	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < Flash_Size; page_prog_start = page_prog_start + 256)
	{
		//=================================
		// SPI Transfer Control
		// Set 256 bytes page read : 0x8000_0020 ==> 0x6940_02FF
		// Set read start address  : 0x8000_0028 ==> 0x0000_0000
		// Set command			   : 0x8000_0024 ==> 0x0000_003B
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x69; tmp_data[2] = 0x40; tmp_data[1] = 0x02; tmp_data[0] = 0xFF;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = 0x00;
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = (uint8_t)(page_prog_start >> 16);
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x3B;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		//==================================
		// AHB_I2C Burst Read
		// Set SPI data register : 0x8000_002C ==> 0x00
		//==================================
		out_buffer[0] = 0x2C;
		out_buffer[1] = 0x00;
		out_buffer[2] = 0x00;
		out_buffer[3] = 0x80;
		i2c_himax_write(client, 0x00 ,out_buffer, 4, 3);

		//==================================
		// Read access : 0x0C ==> 0x00
		//==================================
		out_buffer[0] = 0x00;
		i2c_himax_write(client, 0x0C ,out_buffer, 1, 3);

		//==================================
		// Read 128 bytes two times
		//==================================
		i2c_himax_read(client, 0x08 ,in_buffer, 128, 3);
		for (i = 0; i < 128; i++)
			flash_buffer[i + page_prog_start] = in_buffer[i];

		i2c_himax_read(client, 0x08 ,in_buffer, 128, 3);
		for (i = 0; i < 128; i++)
			flash_buffer[(i + 128) + page_prog_start] = in_buffer[i];

		I("%s:Verify Progress: %x\n", __func__, page_prog_start);
	}

/*=============Dump Flash End=============*/
		//msleep(100);
		/*
		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<64 ; j++)
			{
				setFlashDumpProgress(i*32 + j);
			}
		}
		*/
	himax_sense_on(client, 0x01);

	return;

}

int himax_chip_self_test(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[128];
	int pf_value=0x00;
	uint8_t test_result_id = 0;
	int j;

	memset(tmp_addr, 0x00, sizeof(tmp_addr));
	memset(tmp_data, 0x00, sizeof(tmp_data));

	himax_interface_on(client);
	himax_sense_off(client);

	//Set criteria
	himax_burst_enable(client, 1);

	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x94;
	tmp_data[3] = 0x14; tmp_data[2] = 0xC8; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_data[7] = 0x20; tmp_data[6] = 0x58; tmp_data[5] = 0x10; tmp_data[4] = 0x96;

	himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 8);

	//start selftest
	// 0x9008_805C ==> 0x0000_0001
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x5C;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	himax_sense_on(client, 1);

	msleep(2000);

	himax_sense_off(client);
	msleep(20);

	//=====================================
	// Read test result ID : 0x9008_8078 ==> 0xA/0xB/0xC/0xF
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x78;
	himax_register_read(client, tmp_addr, 4, tmp_data, false);

	test_result_id = tmp_data[0];

	I("%s: check test result, test_result_id=%x, test_result=%x\n", __func__
	,test_result_id,tmp_data[0]);

	if (test_result_id==0xF) {
		I("[Himax]: self-test pass\n");
		pf_value = 0x1;
	} else {
		E("[Himax]: self-test fail\n");
		pf_value = 0x0;
	}
	himax_burst_enable(client, 1);

	for (j = 0;j < 10; j++){
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x06; tmp_addr[1] = 0x00; tmp_addr[0] = 0x0C;
		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		I("[Himax]: 9006000C = %d\n", tmp_data[0]);
		if (tmp_data[0] != 0){
		tmp_data[3] = 0x90; tmp_data[2] = 0x06; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		if ( i2c_himax_write(client, 0x00 ,tmp_data, 4, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
		}
		tmp_data[0] = 0x00;
		if ( i2c_himax_write(client, 0x0C ,tmp_data, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
		}
			i2c_himax_read(client, 0x08, tmp_data, 124,HIMAX_I2C_RETRY_TIMES);
		}else{
			break;
		}
	}

	himax_sense_on(client, 1);
	msleep(120);

	return pf_value;
}

void himax_set_HSEN_enable(struct i2c_client *client, uint8_t HSEN_enable, bool suspended)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t retry_cnt = 0;

	do
	{
		// 0x9008_8054 ==> 0x0000_0000/0001
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x50;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00;	tmp_data[0] = HSEN_enable;

		himax_flash_write_burst(client, tmp_addr, tmp_data);

		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		//I("%s: tmp_data[0]=%d, HSEN_enable=%d, retry_cnt=%d \n", __func__, tmp_data[0],HSEN_enable,retry_cnt);
		retry_cnt++;
	}while (tmp_data[0] != HSEN_enable && retry_cnt < HIMAX_REG_RETRY_TIMES);
}

int himax_palm_detect(uint8_t *buf)
{

	return GESTURE_DETECT_FAIL;

}

void himax_set_SMWP_enable(struct i2c_client *client, uint8_t SMWP_enable, bool suspended)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t retry_cnt = 0;

	do
	{
		// 0x9008_8054 ==> 0x0000_0000/0001
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x54;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00;	tmp_data[0] = SMWP_enable;

		himax_flash_write_burst(client, tmp_addr, tmp_data);

		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		//I("%s: tmp_data[0]=%d, SMWP_enable=%d, retry_cnt=%d \n", __func__, tmp_data[0],SMWP_enable,retry_cnt);
		retry_cnt++;
	}while (tmp_data[0] != SMWP_enable && retry_cnt < HIMAX_REG_RETRY_TIMES);

}

void himax_usb_detect_set(struct i2c_client *client,uint8_t *cable_config)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t retry_cnt = 0;

	do
	{
		//notify USB plug/unplug
		// 0x9008_8060 ==> 0x0000_0000/0001
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x60;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00;	tmp_data[0] = cable_config[1];

		himax_flash_write_burst(client, tmp_addr, tmp_data);

		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		//I("%s: tmp_data[0]=%d, cable_config[1]=%d, retry_cnt=%d \n", __func__, tmp_data[0],cable_config[1],retry_cnt);
		retry_cnt++;
	}while (tmp_data[0] != cable_config[1] && retry_cnt < HIMAX_REG_RETRY_TIMES);

}

void himax_burst_enable(struct i2c_client *client, uint8_t auto_add_4_byte)
{
	uint8_t tmp_data[4];

	tmp_data[0] = 0x31;
	if ( i2c_himax_write(client, 0x13 ,tmp_data, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}
	
	tmp_data[0] = (0x10 | auto_add_4_byte);
	if ( i2c_himax_write(client, 0x0D ,tmp_data, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

}

void himax_register_read(struct i2c_client *client, uint8_t *read_addr, int read_length, uint8_t *read_data, bool cfg_flag)
{
	uint8_t tmp_data[4];
	int i = 0;
	int address = 0;

	if(read_length>256)
	{
		E("%s: read len over 256!\n", __func__);
		return;
	}
	if (read_length > 4)
		himax_burst_enable(client, 1);
	else
		himax_burst_enable(client, 0);

	address = (read_addr[3] << 24) + (read_addr[2] << 16) + (read_addr[1] << 8) + read_addr[0];
	i = address;
		tmp_data[0] = (uint8_t)i;
		tmp_data[1] = (uint8_t)(i >> 8);
		tmp_data[2] = (uint8_t)(i >> 16);
		tmp_data[3] = (uint8_t)(i >> 24);
		if ( i2c_himax_write(client, 0x00 ,tmp_data, 4, HIMAX_I2C_RETRY_TIMES) < 0) {
				E("%s: i2c access fail!\n", __func__);
				return;
		 	}
		tmp_data[0] = 0x00;
		if ( i2c_himax_write(client, 0x0C ,tmp_data, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
				E("%s: i2c access fail!\n", __func__);
				return;
		 	}
		
		if ( i2c_himax_read(client, 0x08 ,read_data, read_length, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}
	if (read_length > 4)
		himax_burst_enable(client, 0);
}

void himax_flash_read(struct i2c_client *client, uint8_t *reg_byte, uint8_t *read_data)
{
    uint8_t tmpbyte[2];
    
    if ( i2c_himax_write(client, 0x00 ,&reg_byte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(client, 0x01 ,&reg_byte[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(client, 0x02 ,&reg_byte[2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(client, 0x03 ,&reg_byte[3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

    tmpbyte[0] = 0x00;
    if ( i2c_himax_write(client, 0x0C ,&tmpbyte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(client, 0x08 ,&read_data[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(client, 0x09 ,&read_data[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(client, 0x0A ,&read_data[2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(client, 0x0B ,&read_data[3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_read(client, 0x18 ,&tmpbyte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}// No bus request

	if ( i2c_himax_read(client, 0x0F ,&tmpbyte[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}// idle state

}

void himax_flash_write_burst(struct i2c_client *client, uint8_t * reg_byte, uint8_t * write_data)
{
    uint8_t data_byte[8];
	int i = 0, j = 0;

     for (i = 0; i < 4; i++)
     { 
         data_byte[i] = reg_byte[i];
     }
     for (j = 4; j < 8; j++)
     {
         data_byte[j] = write_data[j-4];
     }
	 
	 if ( i2c_himax_write(client, 0x00 ,data_byte, 8, HIMAX_I2C_RETRY_TIMES) < 0) {
		 E("%s: i2c access fail!\n", __func__);
		 return;
	 }

}

void himax_flash_write_burst_lenth(struct i2c_client *client, uint8_t *reg_byte, uint8_t *write_data, int length)
{
    uint8_t data_byte[256];
	int i = 0, j = 0;

    for (i = 0; i < 4; i++)
    {
        data_byte[i] = reg_byte[i];
    }
    for (j = 4; j < length + 4; j++)
    {
        data_byte[j] = write_data[j - 4];
    }
   
	if ( i2c_himax_write(client, 0x00 ,data_byte, length + 4, HIMAX_I2C_RETRY_TIMES) < 0) {
		 E("%s: i2c access fail!\n", __func__);
		 return;
	}
}

void himax_register_write(struct i2c_client *client, uint8_t *write_addr, int write_length, uint8_t *write_data, bool cfg_flag)
{
	int i =0, address = 0;

	address = (write_addr[3] << 24) + (write_addr[2] << 16) + (write_addr[1] << 8) + write_addr[0];

	for (i = address; i < address + write_length; i++)
	{
		if (write_length > 4)
		{
			himax_burst_enable(client, 1);
		}
		else
		{
			himax_burst_enable(client, 0);
		}
		himax_flash_write_burst_lenth(client, write_addr, write_data, write_length);
	}
}

void himax_sense_off(struct i2c_client *client)
{
	uint8_t wdt_off = 0x00;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[5];	

	himax_burst_enable(client, 0);

	while(wdt_off == 0x00)
	{
		// 0x9000_800C ==> 0x0000_AC53
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x80; tmp_addr[0] = 0x0C;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0xAC; tmp_data[0] = 0x53;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		//=====================================
    // Read Watch Dog disable password : 0x9000_800C ==> 0x0000_AC53
    //=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x80; tmp_addr[0] = 0x0C;
		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		
		//Check WDT
		if (tmp_data[0] == 0x53 && tmp_data[1] == 0xAC && tmp_data[2] == 0x00 && tmp_data[3] == 0x00)
			wdt_off = 0x01;
		else
			wdt_off = 0x00;
	}

	// VCOM		//0x9008_806C ==> 0x0000_0001
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x08; tmp_addr[1] = 0x80; tmp_addr[0] = 0x6C;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	msleep(20);

	// 0x9000_0010 ==> 0x0000_00DA
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xDA;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Read CPU clock off password : 0x9000_0010 ==> 0x0000_00DA
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	himax_register_read(client, tmp_addr, 4, tmp_data, false);
	I("%s: CPU clock off password data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
		,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

}

void himax_interface_on(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[5];

    //===========================================
    //  Any Cmd for ineterface on : 0x9000_0000 ==> 0x0000_0000
    //===========================================
    tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
    himax_flash_read(client, tmp_addr, tmp_data); //avoid RD/WR fail
}

bool wait_wip(struct i2c_client *client, int Timing)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t in_buffer[10];
	//uint8_t out_buffer[20];
	int retry_cnt = 0;

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	in_buffer[0] = 0x01;

	do
	{
		//=====================================
		// SPI Transfer Control : 0x8000_0020 ==> 0x4200_0003
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x42; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x03;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		//=====================================
		// SPI Command : 0x8000_0024 ==> 0x0000_0005
		// read 0x8000_002C for 0x01, means wait success
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x05;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		in_buffer[0] = in_buffer[1] = in_buffer[2] = in_buffer[3] = 0xFF;
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		himax_register_read(client, tmp_addr, 4, in_buffer, false);
		
		if ((in_buffer[0] & 0x01) == 0x00)
			return true;

		retry_cnt++;
		
		if (in_buffer[0] != 0x00 || in_buffer[1] != 0x00 || in_buffer[2] != 0x00 || in_buffer[3] != 0x00)
        	I("%s:Wait wip retry_cnt:%d, buffer[0]=%d, buffer[1]=%d, buffer[2]=%d, buffer[3]=%d \n", __func__, 
            retry_cnt,in_buffer[0],in_buffer[1],in_buffer[2],in_buffer[3]);

		if (retry_cnt > 100)
        {        	
			E("%s: Wait wip error!\n", __func__);
            return false;
        }
		msleep(Timing);
	}while ((in_buffer[0] & 0x01) == 0x01);
	return true;
}

void himax_sense_on(struct i2c_client *client, uint8_t FlashMode)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[128];

	himax_interface_on(client);
	himax_burst_enable(client, 0);
	//CPU reset
	// 0x9000_0014 ==> 0x0000_00CA
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xCA;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Read pull low CPU reset signal : 0x9000_0014 ==> 0x0000_00CA
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	himax_register_read(client, tmp_addr, 4, tmp_data, false);

	I("%s: check pull low CPU reset signal  data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
	,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

	// 0x9000_0014 ==> 0x0000_0000
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Read revert pull low CPU reset signal : 0x9000_0014 ==> 0x0000_0000
	//=====================================
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x14;
	himax_register_read(client, tmp_addr, 4, tmp_data, false);

	I("%s: revert pull low CPU reset signal data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n", __func__
	,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);

	//=====================================
  // Reset TCON
  //=====================================
  tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0xE0;
  tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
  himax_flash_write_burst(client, tmp_addr, tmp_data);
	msleep(10);
  tmp_addr[3] = 0x80; tmp_addr[2] = 0x02; tmp_addr[1] = 0x01; tmp_addr[0] = 0xE0;
  tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
  himax_flash_write_burst(client, tmp_addr, tmp_data);

	if (FlashMode == 0x00)	//SRAM
	{
		//=====================================
		//			Re-map
		//=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xF1;
		himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 4);
		I("%s:83100_Chip_Re-map ON\n", __func__);
	}
	else
	{
		//=====================================
		//			Re-map off
		//=====================================
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 4);
		I("%s:83100_Chip_Re-map OFF\n", __func__);
	}
	//=====================================
	//			CPU clock on
	//=====================================
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 4);

}

void himax_chip_erase(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];

	himax_burst_enable(client, 0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
	//				  2. 0x8000_0024 ==> 0x0000_0006
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Erase Command : 0x8000_0024 ==> 0x0000_00C7
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0xC7;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
	
	msleep(2000);
	
	if (!wait_wip(client, 100))
		E("%s:83100_Chip_Erase Fail\n", __func__);

}

bool himax_block_erase(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];

	himax_burst_enable(client, 0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Chip Erase
	// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
	//				  2. 0x8000_0024 ==> 0x0000_0006
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	//=====================================
	// Block Erase
	// Erase Command : 0x8000_0028 ==> 0x0000_0000 //SPI addr
	//				   0x8000_0020 ==> 0x6700_0000 //control
	//				   0x8000_0024 ==> 0x0000_0052 //BE
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
	
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x67; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
	
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x52;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	msleep(1000);

	if (!wait_wip(client, 100))
	{
		E("%s:83100_Erase Fail\n", __func__);
		return false;
	}
	else
	{
		return true;
	}

}

bool himax_sector_erase(struct i2c_client *client, int start_addr)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	int page_prog_start = 0;

	himax_burst_enable(client, 0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
	for (page_prog_start = start_addr; page_prog_start < start_addr + 0x0F000; page_prog_start = page_prog_start + 0x1000)
		{
			//=====================================
			// Chip Erase
			// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
			//				  2. 0x8000_0024 ==> 0x0000_0006
			//=====================================
			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
			tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			himax_flash_write_burst(client, tmp_addr, tmp_data);

			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
			tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
			himax_flash_write_burst(client, tmp_addr, tmp_data);

			//=====================================
			// Sector Erase
			// Erase Command : 0x8000_0028 ==> 0x0000_0000 //SPI addr
			// 				0x8000_0020 ==> 0x6700_0000 //control
			// 				0x8000_0024 ==> 0x0000_0020 //SE
			//=====================================
			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
			if (page_prog_start < 0x100)
			{
			 tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = (uint8_t)page_prog_start;
			}
			else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
			{
			 tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = (uint8_t)(page_prog_start >> 8); tmp_data[0] = (uint8_t)page_prog_start;
			}
			else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
			{
			 tmp_data[3] = 0x00; tmp_data[2] = (uint8_t)(page_prog_start >> 16); tmp_data[1] = (uint8_t)(page_prog_start >> 8); tmp_data[0] = (uint8_t)page_prog_start;
			}
			himax_flash_write_burst(client, tmp_addr, tmp_data);

			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
			tmp_data[3] = 0x67; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
			himax_flash_write_burst(client, tmp_addr, tmp_data);

			tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
			tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x20;
			himax_flash_write_burst(client, tmp_addr, tmp_data);

			msleep(200);

			if (!wait_wip(client, 100))
			{
				E("%s:83100_Erase Fail\n", __func__);
				return false;
			}
		}	
		return true;
}

void himax_sram_write(struct i2c_client *client, uint8_t *FW_content)
{
	int i = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[128];
	int FW_length = 0x4000; // 0x4000 = 16K bin file
	
	//himax_sense_off(client);

	for (i = 0; i < FW_length; i = i + 128) 
	{
		himax_burst_enable(client, 1);

		if (i < 0x100)
		{
			tmp_addr[3] = 0x08; 
			tmp_addr[2] = 0x00; 
			tmp_addr[1] = 0x00; 
			tmp_addr[0] = i;
		}
		else if (i >= 0x100 && i < 0x10000)
		{
			tmp_addr[3] = 0x08; 
			tmp_addr[2] = 0x00; 
			tmp_addr[1] = (i >> 8); 
			tmp_addr[0] = i;
		}

		memcpy(&tmp_data[0], &FW_content[i], 128);
		himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 128);

	}

	if (!wait_wip(client, 100))
		E("%s:83100_Sram_Write Fail\n", __func__);
}

bool himax_sram_verify(struct i2c_client *client, uint8_t *FW_File, int FW_Size)
{
	int i = 0;
	uint8_t out_buffer[20];
	uint8_t in_buffer[128];
	uint8_t *get_fw_content;

	get_fw_content = kzalloc(0x4000*sizeof(uint8_t), GFP_KERNEL);

	for (i = 0; i < 0x4000; i = i + 128)
	{
		himax_burst_enable(client, 1);

		//==================================
		//	AHB_I2C Burst Read
		//==================================
		if (i < 0x100)
		{
			out_buffer[3] = 0x08; 
			out_buffer[2] = 0x00; 
			out_buffer[1] = 0x00; 
			out_buffer[0] = i;
		}
		else if (i >= 0x100 && i < 0x10000)
		{
			out_buffer[3] = 0x08; 
			out_buffer[2] = 0x00; 
			out_buffer[1] = (i >> 8); 
			out_buffer[0] = i;
		}

		if ( i2c_himax_write(client, 0x00 ,out_buffer, 4, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		out_buffer[0] = 0x00;		
		if ( i2c_himax_write(client, 0x0C ,out_buffer, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		if ( i2c_himax_read(client, 0x08 ,in_buffer, 128, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}
		memcpy(&get_fw_content[i], &in_buffer[0], 128);
	}

	for (i = 0; i < FW_Size; i++)
		{
	        if (FW_File[i] != get_fw_content[i])
	        	{
					E("%s: fail! SRAM[%x]=%x NOT CRC_ifile=%x\n", __func__,i,get_fw_content[i],FW_File[i]);
		            return false;
	        	}
		}

	kfree(get_fw_content);

	return true;
}

void himax_flash_programming(struct i2c_client *client, uint8_t *FW_content, int FW_Size)
{
	int page_prog_start = 0;
	int program_length = 48;
	int i = 0, j = 0, k = 0;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t buring_data[256];    // Read for flash data, 128K
									 // 4 bytes for 0x80002C padding

	//himax_interface_on(client);
	himax_burst_enable(client, 0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < FW_Size; page_prog_start = page_prog_start + 256)
	{
		//msleep(5);
		//=====================================
		// Write Enable : 1. 0x8000_0020 ==> 0x4700_0000
		//				  2. 0x8000_0024 ==> 0x0000_0006
		//=====================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x47; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x06;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		//=================================
		// SPI Transfer Control
		// Set 256 bytes page write : 0x8000_0020 ==> 0x610F_F000
		// Set read start address	: 0x8000_0028 ==> 0x0000_0000			
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x61; tmp_data[2] = 0x0F; tmp_data[1] = 0xF0; tmp_data[0] = 0x00;
		// data bytes should be 0x6100_0000 + ((word_number)*4-1)*4096 = 0x6100_0000 + 0xFF000 = 0x610F_F000
		// Programmable size = 1 page = 256 bytes, word_number = 256 byte / 4 = 64
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		//tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00; // Flash start address 1st : 0x0000_0000

		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = 0x00; 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = 0x00; 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00; 
			tmp_data[2] = (uint8_t)(page_prog_start >> 16); 
			tmp_data[1] = (uint8_t)(page_prog_start >> 8); 
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		
		himax_flash_write_burst(client, tmp_addr, tmp_data);


		//=================================
		// Send 16 bytes data : 0x8000_002C ==> 16 bytes data	  
		//=================================
		buring_data[0] = 0x2C;
		buring_data[1] = 0x00;
		buring_data[2] = 0x00;
		buring_data[3] = 0x80;
		
		for (i = /*0*/page_prog_start, j = 0; i < 16 + page_prog_start/**/; i++, j++)	/// <------ bin file
		{
			buring_data[j + 4] = FW_content[i];
		}


		if ( i2c_himax_write(client, 0x00 ,buring_data, 20, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}
		//=================================
		// Write command : 0x8000_0024 ==> 0x0000_0002
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x02;
		himax_flash_write_burst(client, tmp_addr, tmp_data);

		//=================================
		// Send 240 bytes data : 0x8000_002C ==> 240 bytes data	 
		//=================================

		for (j = 0; j < 5; j++)
		{
			for (i = (page_prog_start + 16 + (j * 48)), k = 0; i < (page_prog_start + 16 + (j * 48)) + program_length; i++, k++)   /// <------ bin file
			{
				buring_data[k+4] = FW_content[i];//(byte)i;
			}

			if ( i2c_himax_write(client, 0x00 ,buring_data, program_length+4, HIMAX_I2C_RETRY_TIMES) < 0) {
				E("%s: i2c access fail!\n", __func__);
				return;
			}

		}

		if (!wait_wip(client, 1))
			E("%s:83100_Flash_Programming Fail\n", __func__);
	}
}

bool himax_check_chip_version(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t ret_data = 0x00;
	int i = 0;
	himax_sense_off(client);
	for (i = 0; i < 5; i++)
	{
		// 1. Set DDREG_Req = 1 (0x9000_0020 = 0x0000_0001) (Lock register R/W from driver)
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
		himax_register_write(client, tmp_addr, 4, tmp_data, false);

		// 2. Set bank as 0 (0x8001_BD01 = 0x0000_0000)
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xBD; tmp_addr[0] = 0x01;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_register_write(client, tmp_addr, 4, tmp_data, false);

		// 3. Read driver ID register RF4H 1 byte (0x8001_F401)
		//	  Driver register RF4H 1 byte value = 0x84H, read back value will become 0x84848484
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xF4; tmp_addr[0] = 0x01;
		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		ret_data = tmp_data[0];

		I("%s:Read driver IC ID = %X\n", __func__, ret_data);
		if (ret_data == 0x84)
		{
			IC_TYPE         = HX_83100A_SERIES_PWON;
			//himax_sense_on(client, 0x01);
			ret_data = true;
			break;
		}
		else
		{
			ret_data = false;
			E("%s:Read driver ID register Fail:\n", __func__);
		}
	}
	// 4. After read finish, set DDREG_Req = 0 (0x9000_0020 = 0x0000_0000) (Unlock register R/W from driver)
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_register_write(client, tmp_addr, 4, tmp_data, false);
	//himax_sense_on(client, 0x01);
	return ret_data;
}

#if 1
int himax_check_CRC(struct i2c_client *client, int mode)
{
	bool burnFW_success = false;
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	int tmp_value;
	int CRC_value = 0;

	memset(tmp_data, 0x00, sizeof(tmp_data));

	if (1)
	{
		if(mode == fw_image_60k)
		{
			himax_sram_write(client, (i_TP_CRC_FW_60K));
			burnFW_success = himax_sram_verify(client, i_TP_CRC_FW_60K, 0x4000);
		}
		else if(mode == fw_image_64k)
		{
			himax_sram_write(client, (i_TP_CRC_FW_64K));
			burnFW_success = himax_sram_verify(client, i_TP_CRC_FW_64K, 0x4000);
		}
		else if(mode == fw_image_124k)
		{
			himax_sram_write(client, (i_TP_CRC_FW_124K));
			burnFW_success = himax_sram_verify(client, i_TP_CRC_FW_124K, 0x4000);
		}
		else if(mode == fw_image_128k)
		{
			himax_sram_write(client, (i_TP_CRC_FW_128K));
			burnFW_success = himax_sram_verify(client, i_TP_CRC_FW_128K, 0x4000);
		}
		if (burnFW_success)
		{
			I("%s: Start to do CRC FW mode=%d \n", __func__,mode);
			himax_sense_on(client, 0x00);	// run CRC firmware

			while(true)
			{
				msleep(100);

				tmp_addr[3] = 0x90; 
				tmp_addr[2] = 0x08; 
				tmp_addr[1] = 0x80; 
				tmp_addr[0] = 0x94;
				himax_register_read(client, tmp_addr, 4, tmp_data, false);

				I("%s: CRC from firmware is %x, %x, %x, %x \n", __func__,tmp_data[3],
					tmp_data[2],tmp_data[1],tmp_data[0]);

				if (tmp_data[3] == 0xFF && tmp_data[2] == 0xFF && tmp_data[1] == 0xFF && tmp_data[0] == 0xFF)
				{ 
					}
				else
					break;
			}

			CRC_value = tmp_data[3];

			tmp_value = tmp_data[2] << 8;
			CRC_value += tmp_value;

			tmp_value = tmp_data[1] << 16;
			CRC_value += tmp_value;

			tmp_value = tmp_data[0] << 24;
			CRC_value += tmp_value;

			I("%s: CRC Value is %x \n", __func__, CRC_value);

			//Close Remapping
	        //=====================================
	        //          Re-map close
	        //=====================================
	        tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x00;
	        tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	        himax_flash_write_burst_lenth(client, tmp_addr, tmp_data, 4);
			return CRC_value;				
		}
		else
		{
			E("%s: SRAM write fail\n", __func__);
			return 0;
		}		
	}
	else
		I("%s: NO CRC Check File \n", __func__);

	return 0;
}

bool Calculate_CRC_with_AP(unsigned char *FW_content , int CRC_from_FW, int mode)
{
	uint8_t tmp_data[4];
	int i, j;
	int fw_data;
	int fw_data_2;
	int CRC = 0xFFFFFFFF;
	int PolyNomial = 0x82F63B78;
	int length = 0;
	
	if (mode == fw_image_128k)
		length = 0x8000;
	else if (mode == fw_image_124k)
		length = 0x7C00;
	else if (mode == fw_image_64k)
		length = 0x4000;
	else //if (mode == fw_image_60k)
		length = 0x3C00;

	for (i = 0; i < length; i++)
	{
		fw_data = FW_content[i * 4 ];
		
		for (j = 1; j < 4; j++)
		{
			fw_data_2 = FW_content[i * 4 + j];
			fw_data += (fw_data_2) << (8 * j);
		}

		CRC = fw_data ^ CRC;

		for (j = 0; j < 32; j++)
		{
			if ((CRC % 2) != 0)
			{
				CRC = ((CRC >> 1) & 0x7FFFFFFF) ^ PolyNomial;				
			}
			else
			{
				CRC = (((CRC >> 1) ^ 0x7FFFFFFF)& 0x7FFFFFFF);				
			}
		}
	}

	I("%s: CRC calculate from bin file is %x \n", __func__, CRC);

	tmp_data[0] = (uint8_t)(CRC >> 24);
	tmp_data[1] = (uint8_t)(CRC >> 16);
	tmp_data[2] = (uint8_t)(CRC >> 8);
	tmp_data[3] = (uint8_t) CRC;

	CRC = tmp_data[0];
	CRC += tmp_data[1] << 8;			
	CRC += tmp_data[2] << 16;
	CRC += tmp_data[3] << 24;

	I("%s: CRC calculate from bin file REVERSE %x \n", __func__, CRC);
	I("%s: CRC calculate from FWis %x \n", __func__, CRC_from_FW);
	if (CRC_from_FW == CRC)
		return true;
	else
		return false;
}
#endif

int fts_ctpm_fw_upgrade_with_sys_fs_60k(struct i2c_client *client, unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;

	if (len != 0x10000)   //64k
    {
    	E("%s: The file size is not 64K bytes\n", __func__);
    	return false;
		}
	himax_sense_off(client);
	msleep(500);
	himax_interface_on(client);
  if (!himax_sector_erase(client, 0x00000))
			{
            E("%s:Sector erase fail!Please restart the IC.\n", __func__);
            return false;
      }
	himax_flash_programming(client, fw, 0x0F000);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;

	CRC_from_FW = himax_check_CRC(client,fw_image_60k);
	burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_60k);
	//himax_sense_on(client, 0x01);
	return burnFW_success;
}

int fts_ctpm_fw_upgrade_with_sys_fs_32k(struct i2c_client *client, unsigned char *fw, int len, bool change_iref)
{
	/* Not use in incell */
	return 0;
}

int fts_ctpm_fw_upgrade_with_sys_fs_64k(struct i2c_client *client, unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;

	if (len != 0x10000)   //64k
  {
    	E("%s: The file size is not 64K bytes\n", __func__);
    	return false;
	}
	himax_sense_off(client);
	msleep(500);
	himax_interface_on(client);
	himax_chip_erase(client);
	himax_flash_programming(client, fw, len);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;

	CRC_from_FW = himax_check_CRC(client,fw_image_64k);
	burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_64k);
	//himax_sense_on(client, 0x01);
	return burnFW_success;
}

int fts_ctpm_fw_upgrade_with_sys_fs_124k(struct i2c_client *client, unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;

	if (len != 0x20000)   //128k
  {
    	E("%s: The file size is not 128K bytes\n", __func__);
    	return false;
	}
	himax_sense_off(client);
	msleep(500);
	himax_interface_on(client);
	if (!himax_block_erase(client))
    	{
            E("%s:Block erase fail!Please restart the IC.\n", __func__);
            return false;
      }

    if (!himax_sector_erase(client, 0x10000))
    	{
            E("%s:Sector erase fail!Please restart the IC.\n", __func__);
            return false;
      }
	himax_flash_programming(client, fw, 0x1F000);


	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;

	CRC_from_FW = himax_check_CRC(client,fw_image_124k);
	burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_124k);
	//himax_sense_on(client, 0x01);
	return burnFW_success;
}

int fts_ctpm_fw_upgrade_with_sys_fs_128k(struct i2c_client *client, unsigned char *fw, int len, bool change_iref)
{
	int CRC_from_FW = 0;
	int burnFW_success = 0;

	if (len != 0x20000)   //128k
  {
    	E("%s: The file size is not 128K bytes\n", __func__);
    	return false;
	}
	himax_sense_off(client);
	msleep(500);
	himax_interface_on(client);
	himax_chip_erase(client);

	himax_flash_programming(client, fw, len);

	//burnFW_success = himax_83100_Verify(fw, len);
	//if(burnFW_success==false)
	//	return burnFW_success;

	CRC_from_FW = himax_check_CRC(client,fw_image_128k);
	burnFW_success = Calculate_CRC_with_AP(fw, CRC_from_FW,fw_image_128k);
	//himax_sense_on(client, 0x01);
	return burnFW_success;
}

void himax_touch_information(struct i2c_client *client)
{
	uint8_t cmd[4];
	char data[12] = {0};

	I("%s:IC_TYPE =%d\n", __func__,IC_TYPE);

	if(IC_TYPE == HX_83100A_SERIES_PWON)
	{
		cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xF8;
		himax_register_read(client, cmd, 4, data, false);

		ic_data->HX_RX_NUM				= data[1];
		ic_data->HX_TX_NUM				= data[2];
		ic_data->HX_MAX_PT				= data[3];

		cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xFC;
		himax_register_read(client, cmd, 4, data, false);

	  if((data[1] & 0x04) == 0x04) {
			ic_data->HX_XY_REVERSE = true;
	  } else {
			ic_data->HX_XY_REVERSE = false;
	  }
		ic_data->HX_Y_RES = data[3]*256;
		cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x01; cmd[0] = 0x00;
		himax_register_read(client, cmd, 4, data, false);
		ic_data->HX_Y_RES = ic_data->HX_Y_RES + data[0];
		ic_data->HX_X_RES = data[1]*256 + data[2];
		cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x8C;
		himax_register_read(client, cmd, 4, data, false);
	  if((data[0] & 0x01) == 1) {
			ic_data->HX_INT_IS_EDGE = true;
	  } else {
			ic_data->HX_INT_IS_EDGE = false;
	  }
	  if (ic_data->HX_RX_NUM > 40)
			ic_data->HX_RX_NUM = 29;
	  if (ic_data->HX_TX_NUM > 20)
			ic_data->HX_TX_NUM = 16;
	  if (ic_data->HX_MAX_PT > 10)
			ic_data->HX_MAX_PT = 10;
	  if (ic_data->HX_Y_RES > 2000)
			ic_data->HX_Y_RES = 1280;
	  if (ic_data->HX_X_RES > 2000)
			ic_data->HX_X_RES = 720;
#ifdef HX_EN_MUT_BUTTON
		cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0xE8;
		himax_register_read(client, cmd, 4, data, false);
		ic_data->HX_BT_NUM				= data[3];
#endif
		I("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,ic_data->HX_RX_NUM,ic_data->HX_TX_NUM,ic_data->HX_MAX_PT);
		I("%s:HX_XY_REVERSE =%d,HX_Y_RES =%d,HX_X_RES=%d \n", __func__,ic_data->HX_XY_REVERSE,ic_data->HX_Y_RES,ic_data->HX_X_RES);
		I("%s:HX_INT_IS_EDGE =%d \n", __func__,ic_data->HX_INT_IS_EDGE);
	}
	else
	{
		ic_data->HX_RX_NUM				= 0;
		ic_data->HX_TX_NUM				= 0;
		ic_data->HX_BT_NUM				= 0;
		ic_data->HX_X_RES				= 0;
		ic_data->HX_Y_RES				= 0;
		ic_data->HX_MAX_PT				= 0;
		ic_data->HX_XY_REVERSE		= false;
		ic_data->HX_INT_IS_EDGE		= false;
	}
}

int himax_read_i2c_status(struct i2c_client *client)
{
	return i2c_error_count; //
}

int himax_read_ic_trigger_type(struct i2c_client *client)
{
	uint8_t cmd[4];
	char data[12] = {0};
	int trigger_type = false;
	
	himax_sense_off(client);

	cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x8C;
	himax_register_read(client, cmd, 4, data, false);
	if((data[0] & 0x01) == 1)
		trigger_type = true;
		else
		trigger_type = false;

	himax_sense_on(client, 0x01);

	return trigger_type;
}

void himax_read_FW_ver(struct i2c_client *client)
{
	uint8_t cmd[4];
	uint8_t data[64];

	//=====================================
	// Read FW version : 0x0000_E303
	//=====================================
	cmd[3] = 0x00; cmd[2] = 0x00; cmd[1] = 0xE3; cmd[0] = 0x00;
	himax_register_read(client, cmd, 4, data, false);

	ic_data->vendor_fw_ver = data[3]<<8;

	cmd[3] = 0x00; cmd[2] = 0x00; cmd[1] = 0xE3; cmd[0] = 0x04;
	himax_register_read(client, cmd, 4, data, false);

	ic_data->vendor_fw_ver = data[0] | ic_data->vendor_fw_ver;
	I("FW_VER : %X \n",ic_data->vendor_fw_ver);

	cmd[3] = 0x08; cmd[2] = 0x00; cmd[1] = 0x00; cmd[0] = 0x28;
	himax_register_read(client, cmd, 4, data, false);

	ic_data->vendor_config_ver = data[0]<<8 | data[1];
	I("CFG_VER : %X \n",ic_data->vendor_config_ver);
	
	ic_data->vendor_panel_ver = -1;
	
	ic_data->vendor_cid_maj_ver = -1;
	ic_data->vendor_cid_min_ver = -1;

	return;
}

bool himax_ic_package_check(struct i2c_client *client)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t ret_data = 0x00;
	int i = 0;

	himax_sense_off(client);

	for (i = 0; i < 5; i++)
	{
		// 1. Set DDREG_Req = 1 (0x9000_0020 = 0x0000_0001) (Lock register R/W from driver)
		tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x01;
		himax_register_write(client, tmp_addr, 4, tmp_data, false);

		// 2. Set bank as 0 (0x8001_BD01 = 0x0000_0000)
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xBD; tmp_addr[0] = 0x01;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
		himax_register_write(client, tmp_addr, 4, tmp_data, false);

		// 3. Read driver ID register RF4H 1 byte (0x8001_F401)
		//	  Driver register RF4H 1 byte value = 0x84H, read back value will become 0x84848484
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x01; tmp_addr[1] = 0xF4; tmp_addr[0] = 0x01;
		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		ret_data = tmp_data[0];

		I("%s:Read driver IC ID = %X\n", __func__, ret_data);
		if (ret_data == 0x84)
		{
			IC_TYPE         		= HX_83100A_SERIES_PWON;
			IC_CHECKSUM 			= HX_TP_BIN_CHECKSUM_CRC;
		    //Himax: Set FW and CFG Flash Address
		    FW_VER_MAJ_FLASH_ADDR   = 58115;  //0xE303
		    FW_VER_MAJ_FLASH_LENG   = 1;
		    FW_VER_MIN_FLASH_ADDR   = 58116;  //0xE304
		    FW_VER_MIN_FLASH_LENG   = 1;
		    CFG_VER_MAJ_FLASH_ADDR 	= 57384;   //0x00
		    CFG_VER_MAJ_FLASH_LENG 	= 1;
		    CFG_VER_MIN_FLASH_ADDR 	= 57385;   //0x00
		    CFG_VER_MIN_FLASH_LENG 	= 1;
#ifdef HX_AUTO_UPDATE_FW
			g_i_FW_VER = i_CTPM_FW[FW_VER_MAJ_FLASH_ADDR]<<8 |i_CTPM_FW[FW_VER_MIN_FLASH_ADDR];
			g_i_CFG_VER = i_CTPM_FW[CFG_VER_MAJ_FLASH_ADDR]<<8 |i_CTPM_FW[CFG_VER_MIN_FLASH_ADDR];
			g_i_CID_MAJ = -1;
			g_i_CID_MIN = -1;
#endif
			I("Himax IC package 83100_in\n");
			ret_data = true;
			break;
		}
		else
		{
			ret_data = false;
			E("%s:Read driver ID register Fail:\n", __func__);
		}
	}
	// 4. After read finish, set DDREG_Req = 0 (0x9000_0020 = 0x0000_0000) (Unlock register R/W from driver)
	tmp_addr[3] = 0x90; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_register_write(client, tmp_addr, 4, tmp_data, false);

	return ret_data;
}

void himax_power_on_init(struct i2c_client *client)
{
	I("%s:\n", __func__);
	himax_touch_information(client);
	himax_sense_on(client, 0x01);
}

bool himax_read_event_stack(struct i2c_client *client, uint8_t *buf, uint8_t length)
{
	uint8_t cmd[4];

	if(length > 56)
		length = 124;
	//=====================
	//AHB I2C Burst Read
	//=====================
	cmd[0] = 0x31;
	if ( i2c_himax_write(client, 0x13 ,cmd, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return 0;
	}

	cmd[0] = 0x10;
	if ( i2c_himax_write(client, 0x0D ,cmd, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return 0;
	}

	cmd[3] = 0x90; cmd[2] = 0x06; cmd[1] = 0x00; cmd[0] = 0x00;	
	if ( i2c_himax_write(client, 0x00 ,cmd, 4, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);		
		return 0;
	}
	//=====================
	//Read event stack
	//=====================
	cmd[0] = 0x00;		
	if ( i2c_himax_write(client, 0x0C ,cmd, 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return 0;
	}

	i2c_himax_read(client, 0x08, buf, length, HIMAX_I2C_RETRY_TIMES);
	return 1;
}

#if 0
static void himax_83100_Flash_Write(uint8_t * reg_byte, uint8_t * write_data)
{
	uint8_t tmpbyte[2];

    if ( i2c_himax_write(private_ts->client, 0x00 ,&reg_byte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x01 ,&reg_byte[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x02 ,&reg_byte[2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x03 ,&reg_byte[3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x04 ,&write_data[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x05 ,&write_data[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x06 ,&write_data[2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x07 ,&write_data[3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

    if (isBusrtOn == false)
    {
        tmpbyte[0] = 0x01;
		if ( i2c_himax_write(private_ts->client, 0x0C ,&tmpbyte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
		}
    }
}
#endif
#if 0
static void himax_83100_Flash_Burst_Write(uint8_t * reg_byte, uint8_t * write_data)
{
    //uint8_t tmpbyte[2];
	int i = 0;

	if ( i2c_himax_write(private_ts->client, 0x00 ,&reg_byte[0], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x01 ,&reg_byte[1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x02 ,&reg_byte[2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

	if ( i2c_himax_write(private_ts->client, 0x03 ,&reg_byte[3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
		E("%s: i2c access fail!\n", __func__);
		return;
	}

    // Write 256 bytes with continue burst mode
    for (i = 0; i < 256; i = i + 4)
    {
		if ( i2c_himax_write(private_ts->client, 0x04 ,&write_data[i], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x05 ,&write_data[i+1], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x06 ,&write_data[i+2], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}

		if ( i2c_himax_write(private_ts->client, 0x07 ,&write_data[i+3], 1, HIMAX_I2C_RETRY_TIMES) < 0) {
			E("%s: i2c access fail!\n", __func__);
			return;
		}
    }

    //if (isBusrtOn == false)
    //{
    //   tmpbyte[0] = 0x01;
	//	if ( i2c_himax_write(private_ts->client, 0x0C ,&tmpbyte[0], 1, 3) < 0) {
	//	E("%s: i2c access fail!\n", __func__);
	//	return;
	//	}
    //}

}
#endif

#if 0
static bool himax_83100_Verify(uint8_t *FW_File, int FW_Size)
{
	uint8_t tmp_addr[4];
	uint8_t tmp_data[4];
	uint8_t out_buffer[20];
	uint8_t in_buffer[260];

	int fail_addr=0, fail_cnt=0;
	int page_prog_start = 0;
	int i = 0;

	himax_interface_on(private_ts->client);
	himax_burst_enable(private_ts->client, 0);

	//=====================================
	// SPI Transfer Format : 0x8000_0010 ==> 0x0002_0780
	//=====================================
	tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x10;
	tmp_data[3] = 0x00; tmp_data[2] = 0x02; tmp_data[1] = 0x07; tmp_data[0] = 0x80;
	himax_83100_Flash_Write(tmp_addr, tmp_data);

	for (page_prog_start = 0; page_prog_start < FW_Size; page_prog_start = page_prog_start + 256)
	{
		//=================================
		// SPI Transfer Control
		// Set 256 bytes page read : 0x8000_0020 ==> 0x6940_02FF
		// Set read start address  : 0x8000_0028 ==> 0x0000_0000
		// Set command			   : 0x8000_0024 ==> 0x0000_003B
		//=================================
		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x20;
		tmp_data[3] = 0x69; tmp_data[2] = 0x40; tmp_data[1] = 0x02; tmp_data[0] = 0xFF;
		himax_83100_Flash_Write(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x28;
		if (page_prog_start < 0x100)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = 0x00;
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x100 && page_prog_start < 0x10000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = 0x00;
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		else if (page_prog_start >= 0x10000 && page_prog_start < 0x1000000)
		{
			tmp_data[3] = 0x00;
			tmp_data[2] = (uint8_t)(page_prog_start >> 16);
			tmp_data[1] = (uint8_t)(page_prog_start >> 8);
			tmp_data[0] = (uint8_t)page_prog_start;
		}
		himax_83100_Flash_Write(tmp_addr, tmp_data);

		tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x24;
		tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x00; tmp_data[0] = 0x3B;
		himax_83100_Flash_Write(tmp_addr, tmp_data);

		//==================================
		// AHB_I2C Burst Read
		// Set SPI data register : 0x8000_002C ==> 0x00
		//==================================
		out_buffer[0] = 0x2C;
		out_buffer[1] = 0x00;
		out_buffer[2] = 0x00;
		out_buffer[3] = 0x80;
		i2c_himax_write(private_ts->client, 0x00 ,out_buffer, 4, HIMAX_I2C_RETRY_TIMES);

		//==================================
		// Read access : 0x0C ==> 0x00
		//==================================
		out_buffer[0] = 0x00;
		i2c_himax_write(private_ts->client, 0x0C ,out_buffer, 1, HIMAX_I2C_RETRY_TIMES);

		//==================================
		// Read 128 bytes two times
		//==================================
		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, HIMAX_I2C_RETRY_TIMES);
		for (i = 0; i < 128; i++)
			flash_buffer[i + page_prog_start] = in_buffer[i];

		i2c_himax_read(private_ts->client, 0x08 ,in_buffer, 128, HIMAX_I2C_RETRY_TIMES);
		for (i = 0; i < 128; i++)
			flash_buffer[(i + 128) + page_prog_start] = in_buffer[i];

		//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		//himax_register_read(tmp_addr, 32, out in_buffer);
		//for (int i = 0; i < 128; i++)
		//	  flash_buffer[i + page_prog_start] = in_buffer[i];
		//tmp_addr[3] = 0x80; tmp_addr[2] = 0x00; tmp_addr[1] = 0x00; tmp_addr[0] = 0x2C;
		//himax_register_read(tmp_addr, 32, out in_buffer);
		//for (int i = 0; i < 128; i++)
		//	  flash_buffer[i + page_prog_start] = in_buffer[i];

		I("%s:Verify Progress: %x\n", __func__, page_prog_start);
	}

	fail_cnt = 0;
	for (i = 0; i < FW_Size; i++)
	{
		if (FW_File[i] != flash_buffer[i])
		{
			if (fail_cnt == 0)
				fail_addr = i;

			fail_cnt++;
			//E("%s Fail Block:%x\n", __func__, i);
			//return false;
		}
	}
	if (fail_cnt > 0)
	{
		E("%s:Start Fail Block:%x and fail block count=%x\n" , __func__,fail_addr,fail_cnt);
		return false;
	}

	I("%s:Byte read verify pass.\n", __func__);
	return true;

}
#endif

void himax_get_DSRAM_data(struct i2c_client *client, uint8_t *info_data)
{
	int i;
	int cnt = 0;
	unsigned char tmp_addr[4];
	unsigned char tmp_data[4];
	uint8_t max_i2c_size = 128;
	int total_size = ic_data->HX_TX_NUM * ic_data->HX_RX_NUM * 2;
	int total_read_times = 0;
	unsigned long address = 0x08000468;

	//1. Start DSRAM Rawdata
	tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x04; tmp_addr[0] = 0x64;
	tmp_data[3] = 0x00; tmp_data[2] = 0x00; tmp_data[1] = 0x5A; tmp_data[0] = 0xA5;
	//tmp_data[3] = 0x3A; tmp_data[2] = 0xA3; tmp_data[1] = 0x00; tmp_data[0] = 0x00;
	himax_flash_write_burst(client, tmp_addr, tmp_data);

  //2. Wait Data Ready
  do
	{
		cnt++;
		himax_register_read(client, tmp_addr, 4, tmp_data, false);
		//I("%s: tmp_data[0] = 0x%02X,tmp_data[1] = 0x%02X,tmp_data[2] = 0x%02X,tmp_data[3] = 0x%02X \n", __func__,tmp_data[0],tmp_data[1],tmp_data[2],tmp_data[3]);
		msleep(10);
	} while ((tmp_data[1] != 0xA5 || tmp_data[0] != 0x5A) && cnt < 100);

  //3. Read RawData
  tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x04; tmp_addr[0] = 0x68;

  //4. Re-mapping

  if (total_size % max_i2c_size == 0)
	{
		total_read_times = total_size / max_i2c_size;
	}
	else
	{
		total_read_times = total_size / max_i2c_size + 1;
	}
	for (i = 0; i < (total_read_times); i++)
	{
		if ( total_size >= max_i2c_size)
		{
			himax_register_read(client, tmp_addr, max_i2c_size, &info_data[i*max_i2c_size], false);
			total_size = total_size - max_i2c_size;
		}
		else
		{
			himax_register_read(client, tmp_addr, total_size % max_i2c_size, &info_data[i*max_i2c_size], false);
		}
		address += max_i2c_size;
		tmp_addr[1] = (uint8_t)((address>>8)&0x00FF);
		tmp_addr[0] = (uint8_t)((address)&0x00FF);
	}
	//5. Handshake over
	tmp_addr[3] = 0x08; tmp_addr[2] = 0x00; tmp_addr[1] = 0x04; tmp_addr[0] = 0x64;
	tmp_data[3] = 0x11; tmp_data[2] = 0x22; tmp_data[1] = 0x33; tmp_data[0] = 0x44;
	himax_flash_write_burst(client, tmp_addr, tmp_data);
}

bool himax_calculateChecksum(struct i2c_client *client, bool change_iref)
{
	return 1;
}

//ts_work
int cal_data_len(int raw_cnt_rmd, int HX_MAX_PT, int raw_cnt_max){
	int RawDataLen;
	if (raw_cnt_rmd != 0x00) {
		RawDataLen = 124 - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
	}else{
		RawDataLen = 124 - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
	}
	return RawDataLen;
}

bool diag_check_sum( struct himax_report_data *hx_touch_data ) //return checksum value
{
	uint16_t check_sum_cal = 0;
	int i;

	//Check 124th byte CRC
	for (i = 0, check_sum_cal = 0; i < (hx_touch_data->touch_all_size - hx_touch_data->touch_info_size); i=i+2)
	{
		check_sum_cal += (hx_touch_data->hx_rawdata_buf[i+1]*256 + hx_touch_data->hx_rawdata_buf[i]);
	}
	if (check_sum_cal % 0x10000 != 0)
	{
		I("%s: diag check sum fail! check_sum_cal=%X, hx_touch_info_size=%d, \n",__func__,check_sum_cal, hx_touch_data->touch_info_size);
		return 0;
	}
	return 1;
}


void diag_parse_raw_data(struct himax_report_data *hx_touch_data,int mul_num, int self_num,uint8_t diag_cmd, int16_t *mutual_data, int16_t *self_data)
{
	int RawDataLen_word;
	int index = 0;
	int temp1, temp2,i;

	if (hx_touch_data->hx_rawdata_buf[0] == 0x3A 
	&& hx_touch_data->hx_rawdata_buf[1] == 0xA3
	&& hx_touch_data->hx_rawdata_buf[2] > 0 
	&& hx_touch_data->hx_rawdata_buf[3] == diag_cmd+5)
	{
		RawDataLen_word = hx_touch_data->rawdata_size/2;
		index = (hx_touch_data->hx_rawdata_buf[2] - 1) * RawDataLen_word;
		//I("%s: Header[%d]: %x, %x, %x, %x, mutual: %d, self: %d\n", __func__, index, buf[56], buf[57], buf[58], buf[59], mul_num, self_num);
		//I("RawDataLen=%d , RawDataLen_word=%d , hx_touch_info_size=%d\n", RawDataLen, RawDataLen_word, hx_touch_info_size);
		for (i = 0; i < RawDataLen_word; i++)
		{
			temp1 = index + i;

			if (temp1 < mul_num)
			{ //mutual
					mutual_data[index + i] = hx_touch_data->hx_rawdata_buf[i*2 + 4 + 1]*256 + hx_touch_data->hx_rawdata_buf[i*2 + 4];	//4: RawData Header, 1:HSB
			}
			else
			{//self
				temp1 = i + index;
				temp2 = self_num + mul_num;

				if (temp1 >= temp2)
				{
					break;
				}
				self_data[i+index-mul_num] = hx_touch_data->hx_rawdata_buf[i*2 + 4];	//4: RawData Header
				self_data[i+index-mul_num+1] = hx_touch_data->hx_rawdata_buf[i*2 + 4 + 1];
			}
		}
	}
	
}

uint8_t himax_read_DD_status(uint8_t *cmd_set, uint8_t *tmp_data)
{
	return -1;
}

int himax_read_FW_status(uint8_t *state_addr, uint8_t *tmp_addr)
{
	return -1;
}

#if defined(HX_SMART_WAKEUP)||defined(HX_HIGH_SENSE)||defined(HX_USB_DETECT_GLOBAL)
void himax_resend_cmd_func(bool suspended)
{
	struct himax_ts_data *ts;

	ts = private_ts;

#ifdef HX_SMART_WAKEUP
	himax_set_SMWP_enable(ts->client,ts->SMWP_enable,suspended);
#endif
#ifdef HX_HIGH_SENSE
	himax_set_HSEN_enable(ts->client,ts->HSEN_enable,suspended);
#endif
#ifdef HX_USB_DETECT_GLOBAL
	himax_cable_detect_func(true);
#endif
}
#endif

void himax_resume_ic_action(struct i2c_client *client)
{
	return;
}

void himax_suspend_ic_action(struct i2c_client *client)
{
	return;
}