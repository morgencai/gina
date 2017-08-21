/* Himax Android Driver Sample Code for EXYNOS platform
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

#include "himax_platform.h"
#include "himax_common.h"
#include "himax_ic.h"

//Custom set some config
static int hx_panel_coords[4] = {0,720,0,1280};//[1]=X resolution, [3]=Y resolution
static int hx_display_coords[4] = {0,720,0,1280};
static int report_type = PROTOCOL_TYPE_B;

int i2c_error_count = 0;
int irq_enable_count = 0;

extern struct himax_ic_data* ic_data;
extern void himax_ts_work(struct himax_ts_data *ts);
extern enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer);

#ifdef HX_TP_PROC_DIAG
extern uint8_t getDiagCommand(void);
#endif

extern void himax_log_touch_int_devation(int touched);

int himax_dev_set(struct himax_ts_data *ts)
{
	int ret = 0;

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		E("%s: Failed to allocate input device\n", __func__);
		return ret;
	}
	ts->input_dev->name = "himax-touchscreen";

	return ret;
}

int himax_input_register_device(struct input_dev *input_dev)
{
	return input_register_device(input_dev);
}

int himax_parse_dt(struct himax_ts_data *ts,
				struct himax_i2c_platform_data *pdata)
{
	return 0;
}

#if defined(HX_PLATFOME_DEFINE_KEY)
void himax_platform_key(void)
{
	I("Nothing to be done! Plz cancel it!\n");
}
#endif

int i2c_himax_disp_lgd_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry;
	struct i2c_msg msg[] = {
		{
			.addr = 0x60,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = 0x60,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		msleep(20);
	}
	if (retry == toRetry) {
		E("%s: i2c_read_block retry over %d\n",
			__func__, toRetry);
		//return -EIO;
	}
	return 0;
	

}

int i2c_himax_disp_lgd_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = 0x60,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};
	I("%s: Debug 2 \n", __func__); //Alice
	buf[0] = command;
	memcpy(buf+1, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(20);
	}
	I("%s: Debug 3 \n", __func__); //Alice
	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
			__func__, toRetry);
		//return -EIO;
	}
	return 0;

}

int i2c_himax_disp_hx_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry;
	uint8_t reg[2];
	
	struct i2c_msg msg[] = {
		{
			.addr = 0x60,
			.flags = 0,
			.len = 2,
			.buf = reg,
		},
		{
			.addr = 0x60,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};
	reg[0] = command;
	reg[1] = 0x00;

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		msleep(20);
	}
	if (retry == toRetry) {
		E("%s: i2c_read_block retry over %d\n",
			__func__, toRetry);
		//return -EIO;
	}
	return 0;
	

}

int i2c_himax_disp_hx_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length + 2];

	struct i2c_msg msg[] = {
		{
			.addr = 0x60,
			.flags = 0,
			.len = length + 2,
			.buf = buf,
		}
	};

	buf[0] = command;
	buf[1] = 0x00;
	memcpy(buf+2, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(20);
	}

	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
			__func__, toRetry);
		//return -EIO;
	}
	return 0;

}

int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		msleep(20);
	}
	if (retry == toRetry) {
		E("%s: i2c_read_block retry over %d\n",
			__func__, toRetry);
		i2c_error_count = toRetry;
		return -EIO;
	}
	return 0;

}

int i2c_himax_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length + 1];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = command;
	memcpy(buf+1, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(20);
	}

	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
			__func__, toRetry);
		i2c_error_count = toRetry;
		return -EIO;
	}
	return 0;

}

int i2c_himax_write_command(struct i2c_client *client, uint8_t command, uint8_t toRetry)
{
	return i2c_himax_write(client, command, NULL, 0, toRetry);
}

int i2c_himax_master_write(struct i2c_client *client, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = buf,
		}
	};

	memcpy(buf, data, length);
	
	for (retry = 0; retry < toRetry; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		msleep(20);
	}

	if (retry == toRetry) {
		E("%s: i2c_write_block retry over %d\n",
		       __func__, toRetry);
		i2c_error_count = toRetry;
		return -EIO;
	}
	return 0;
}

void himax_int_enable(int irqnum, int enable)
{
	if (enable == 1 && irq_enable_count == 0) {
		enable_irq(irqnum);
		irq_enable_count++;
		private_ts->irq_enabled = 1;
	} else if (enable == 0 && irq_enable_count == 1) {
		disable_irq_nosync(irqnum);
		irq_enable_count--;
		private_ts->irq_enabled = 0;
	}
	I("irq_enable_count = %d", irq_enable_count);
}

#ifdef HX_RST_PIN_FUNC
void himax_rst_gpio_set(int pinnum, uint8_t value)
{
	gpio_direction_output(pinnum, value);
}
#endif

uint8_t himax_int_gpio_read(int pinnum)
{
	return gpio_get_value(pinnum);
}

int himax_gpio_power_config(struct i2c_client *client,struct himax_i2c_platform_data *pdata)
{
	int error=0;
	//pdata = client->dev.platform_data;
	pdata->gpio_irq = 186;
	pdata->gpio_reset = 64;
	I("%s: gpio_irq_pin=(%d)\n", __func__,pdata->gpio_irq);
	I("%s: gpio_reset_pin=(%d) low\n", __func__,pdata->gpio_reset);

	client->irq = pdata->gpio_irq;
	
	//Set device tree data
	//Set panel coordinates
	pdata->abs_x_min = hx_panel_coords[0], pdata->abs_x_max = hx_panel_coords[1];
	pdata->abs_y_min = hx_panel_coords[2], pdata->abs_y_max = hx_panel_coords[3];
	I(" %s:panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
	pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);

	//Set display coordinates
	pdata->screenWidth  = hx_display_coords[1];
	pdata->screenHeight = hx_display_coords[3];
	I(" %s:display-coords = (%d, %d)\n", __func__,pdata->screenWidth,
	pdata->screenHeight);
	//report type
	pdata->protocol_type = report_type;

#ifdef HX_RST_PIN_FUNC
	if (pdata->gpio_reset >= 0) {
		error = gpio_direction_output(pdata->gpio_reset, 0);
		I("%s: gpio_reset(%d) low\n", __func__,pdata->gpio_reset);
	}
#endif
	if (pdata->gpio_3v3_en >= 0) {
		error = gpio_request(pdata->gpio_3v3_en, "himax-3v3_en");
		if (error < 0)
			E("%s: request 3v3_en pin failed\n", __func__);
		gpio_direction_output(pdata->gpio_3v3_en, 1);
		I("3v3_en pin =%d\n", gpio_get_value(pdata->gpio_3v3_en));
	}
	msleep(20);
#ifdef HX_RST_PIN_FUNC
	if (pdata->gpio_reset >= 0) {
		error = gpio_direction_output(pdata->gpio_reset, 1);
		I("%s: gpio_reset(%d) high\n", __func__,pdata->gpio_reset);
		gpio_free(pdata->gpio_reset);
		I("%s: gpio_reset(%d) free\n", __func__,pdata->gpio_reset);
	}
#endif
return error;
}

static void himax_ts_isr_func(struct himax_ts_data *ts)
{
	himax_ts_work(ts);
}

irqreturn_t himax_ts_thread(int irq, void *ptr)
{
	struct himax_ts_data *ts = ptr;

	if (ts->debug_log_level & BIT(2))
	{
		himax_log_touch_int_devation(HX_FINGER_ON);
	}

	himax_ts_isr_func((struct himax_ts_data *)ptr);

	if(ts->debug_log_level & BIT(2))
	{
		himax_log_touch_int_devation(HX_FINGER_LEAVE);
	}
	return IRQ_HANDLED;
}

static void himax_ts_work_func(struct work_struct *work)
{
	struct himax_ts_data *ts = container_of(work, struct himax_ts_data, work);
	himax_ts_work(ts);
}

int himax_int_register_trigger(struct i2c_client *client)
{
	int ret = 0;
	struct himax_ts_data *ts = i2c_get_clientdata(client);

	if(ic_data->HX_INT_IS_EDGE)
	{
		I("%s edge triiger falling\n ",__func__);
		ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, ts);
	}
	else
	{
		I("%s level trigger low\n ",__func__);
		ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->name, ts);
	}
	return ret;
}

int himax_int_en_set(struct i2c_client *client)
{
	int ret = NO_ERR;
	ret = himax_int_register_trigger(client);
	return ret;
}

int himax_ts_register_interrupt(struct i2c_client *client)
{
	struct himax_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;

	ts->irq_enabled = 0;
	//Work functon
	if (client->irq) {/*INT mode*/
		ts->use_irq = 1;
		ret = himax_int_register_trigger(client);
		if (ret == 0) {
			ts->irq_enabled = 1;
			irq_enable_count = 1;
			I("%s: irq enabled at qpio: %d\n", __func__, client->irq);
#ifdef HX_SMART_WAKEUP
			irq_set_irq_wake(client->irq, 1);
#endif
		} else {
			ts->use_irq = 0;
			E("%s: request_irq failed\n", __func__);
		}
	} else {
		I("%s: client->irq is empty, use polling mode.\n", __func__);
	}

	if (!ts->use_irq) {/*if use polling mode need to disable HX_ESD_RECOVERY function*/
		ts->himax_wq = create_singlethread_workqueue("himax_touch");

		INIT_WORK(&ts->work, himax_ts_work_func);

		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = himax_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		I("%s: polling mode enabled\n", __func__);
	}
	return ret;
}

static int himax_common_suspend(struct device *dev)
{
	struct himax_ts_data *ts = dev_get_drvdata(dev);

	I("%s: enter \n", __func__);

	himax_chip_common_suspend(ts);
	return 0;
}

static int himax_common_resume(struct device *dev)
{
	struct himax_ts_data *ts = dev_get_drvdata(dev);

	I("%s: enter \n", __func__);

	himax_chip_common_resume(ts);
	return 0;
}

#if defined(CONFIG_FB)
int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct himax_ts_data *ts=
		container_of(self, struct himax_ts_data, fb_notif);

	I(" %s\n", __func__);
	if (evdata && evdata->data && event == FB_EVENT_BLANK && ts &&
			ts->client) {
		blank = evdata->data;
		switch (*blank) {
		case FB_BLANK_UNBLANK:
			himax_common_resume(&ts->client->dev);
		break;

		case FB_BLANK_POWERDOWN:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_NORMAL:
			himax_common_suspend(&ts->client->dev);
		break;
		}
	}

	return 0;
}
#endif

static const struct i2c_device_id himax_common_ts_id[] = {
	{HIMAX_common_NAME, 0 },
	{}
};

static const struct dev_pm_ops himax_common_pm_ops = {
#if (!defined(CONFIG_FB))
	.suspend = himax_common_suspend,
	.resume  = himax_common_resume,
#endif
};

#define himax_match_table NULL

static struct i2c_driver himax_common_driver = {
	.id_table	= himax_common_ts_id,
	.probe		= himax_chip_common_probe,
	.remove		= himax_chip_common_remove,
	.driver		= {
	.name = HIMAX_common_NAME,
	.owner = THIS_MODULE,
	.of_match_table = himax_match_table,
#ifdef CONFIG_PM
	.pm				= &himax_common_pm_ops,
#endif
	},
};

static void __init himax_common_init_async(void *unused, async_cookie_t cookie)
{
	I("%s:Enter \n", __func__);
	i2c_add_driver(&himax_common_driver);
}

static int __init himax_common_init(void)
{
	I("Himax common touch panel driver init\n");
	async_schedule(himax_common_init_async, NULL);
	return 0;
}

static void __exit himax_common_exit(void)
{
	i2c_del_driver(&himax_common_driver);
}

module_init(himax_common_init);
module_exit(himax_common_exit);

MODULE_DESCRIPTION("Himax_common driver");
MODULE_LICENSE("GPL");

