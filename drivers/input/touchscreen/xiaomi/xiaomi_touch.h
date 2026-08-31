#ifndef __XIAOMI_TOUCH_H
#define __XIAOMI_TOUCH_H

#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>

#define VALUE_TYPE_SIZE 6
#define VALUE_GRIP_SIZE 9

enum MODE_CMD {
	SET_CUR_VALUE = 0,
	GET_CUR_VALUE,
	GET_DEF_VALUE,
	GET_MIN_VALUE,
	GET_MAX_VALUE,
	GET_MODE_VALUE,
	RESET_MODE,
};

enum MODE_TYPE {
	Touch_Game_Mode = 0,
	Touch_Active_MODE = 1,
	Touch_UP_THRESHOLD = 2,
	Touch_Tolerance = 3,
	Touch_Wgh_Min = 4,
	Touch_Wgh_Max = 5,
	Touch_Wgh_Step = 6,
	Touch_Edge_Filter = 7,
	Touch_Panel_Orientation = 8,
	Touch_Report_Rate = 9,
	Touch_Fod_Enable = 10,
	Touch_Aod_Enable = 11,
	Touch_Mode_NUM = 14,
};

struct xiaomi_touch_interface {
	int touch_mode[Touch_Mode_NUM][VALUE_TYPE_SIZE];
	int touch_edge[VALUE_GRIP_SIZE];
	int (*setModeValue)(int mode, int value);
	int (*getModeValue)(int mode, int value_type);
	int (*getModeAll)(int mode, int *modevalue);
	int (*resetMode)(int mode);
	int (*p_sensor_read)(void);
	int (*p_sensor_write)(int on);
	int (*palm_sensor_read)(void);
	int (*palm_sensor_write)(int on);
};

struct xiaomi_touch {
	struct miscdevice misc_dev;
	struct device *dev;
	struct class *class;
	struct attribute_group attrs;
	struct mutex mutex;
	struct mutex palm_mutex;
	struct mutex psensor_mutex;
	wait_queue_head_t wait_queue;
};

struct xiaomi_touch_pdata {
	struct xiaomi_touch *device;
	struct xiaomi_touch_interface *touch_data;
	int palm_value;
	bool palm_changed;
	int psensor_value;
	bool psensor_changed;
	const char *name;
};

struct xiaomi_touch *xiaomi_touch_dev_get(int minor);
struct class *get_xiaomi_touch_class(void);
struct device *get_xiaomi_touch_dev(void);
int update_palm_sensor_value(int value);
int update_p_sensor_value(int value);
int xiaomitouch_register_modedata(struct xiaomi_touch_interface *data);

#endif