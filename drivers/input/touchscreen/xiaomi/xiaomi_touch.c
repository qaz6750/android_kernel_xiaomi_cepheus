#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "xiaomi_touch.h"

#define XIAOMI_TOUCH_DEVT MKDEV(0, 'T')

static struct xiaomi_touch_pdata *touch_pdata;

static int xiaomi_touch_dev_open(struct inode *inode, struct file *file)
{
	struct xiaomi_touch *touch_dev;

	touch_dev = xiaomi_touch_dev_get(iminor(inode));
	if (!touch_dev)
		return -ENODEV;

	file->private_data = dev_get_drvdata(touch_dev->dev);
	return file->private_data ? 0 : -ENODEV;
}

static ssize_t xiaomi_touch_dev_read(struct file *file, char __user *buf,
				     size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t xiaomi_touch_dev_write(struct file *file,
				      const char __user *buf, size_t count,
				      loff_t *pos)
{
	return 0;
}

static __poll_t xiaomi_touch_dev_poll(struct file *file, poll_table *wait)
{
	return 0;
}

static long xiaomi_touch_dev_ioctl(struct file *file, unsigned int cmd,
				   unsigned long arg)
{
	struct xiaomi_touch_pdata *pdata = file->private_data;
	struct xiaomi_touch_interface *touch_data;
	void __user *argp = (void __user *)arg;
	int values[VALUE_TYPE_SIZE] = { 0 };
	int user_cmd = _IOC_NR(cmd);
	int ret;

	if (!pdata || !pdata->touch_data || !pdata->device)
		return -ENODEV;

	touch_data = pdata->touch_data;
	if (copy_from_user(values, argp, sizeof(values)))
		return -EFAULT;

	mutex_lock(&pdata->device->mutex);
	switch (user_cmd) {
	case SET_CUR_VALUE:
		values[0] = touch_data->setModeValue ?
			touch_data->setModeValue(values[0], values[1]) : -EOPNOTSUPP;
		ret = 0;
		break;
	case GET_CUR_VALUE:
	case GET_DEF_VALUE:
	case GET_MIN_VALUE:
	case GET_MAX_VALUE:
		values[0] = touch_data->getModeValue ?
			touch_data->getModeValue(values[0], user_cmd) : -EOPNOTSUPP;
		ret = 0;
		break;
	case RESET_MODE:
		values[0] = touch_data->resetMode ?
			touch_data->resetMode(values[0]) : -EOPNOTSUPP;
		ret = 0;
		break;
	case GET_MODE_VALUE:
		ret = touch_data->getModeAll ?
			touch_data->getModeAll(values[0], values) : -EOPNOTSUPP;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret >= 0 && copy_to_user(argp, values, sizeof(values)))
		ret = -EFAULT;
	mutex_unlock(&pdata->device->mutex);

	return ret;
}

static int xiaomi_touch_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations xiaomitouch_dev_fops = {
	.owner = THIS_MODULE,
	.open = xiaomi_touch_dev_open,
	.read = xiaomi_touch_dev_read,
	.write = xiaomi_touch_dev_write,
	.poll = xiaomi_touch_dev_poll,
	.unlocked_ioctl = xiaomi_touch_dev_ioctl,
	.compat_ioctl = xiaomi_touch_dev_ioctl,
	.release = xiaomi_touch_dev_release,
	.llseek = no_llseek,
};

static struct xiaomi_touch xiaomi_touch_dev = {
	.misc_dev = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "xiaomi-touch",
		.fops = &xiaomitouch_dev_fops,
	},
	.mutex = __MUTEX_INITIALIZER(xiaomi_touch_dev.mutex),
	.palm_mutex = __MUTEX_INITIALIZER(xiaomi_touch_dev.palm_mutex),
	.psensor_mutex = __MUTEX_INITIALIZER(xiaomi_touch_dev.psensor_mutex),
	.wait_queue = __WAIT_QUEUE_HEAD_INITIALIZER(xiaomi_touch_dev.wait_queue),
};

struct xiaomi_touch *xiaomi_touch_dev_get(int minor)
{
	return xiaomi_touch_dev.misc_dev.minor == minor ? &xiaomi_touch_dev : NULL;
}

struct class *get_xiaomi_touch_class(void)
{
	return xiaomi_touch_dev.class;
}

struct device *get_xiaomi_touch_dev(void)
{
	return xiaomi_touch_dev.dev;
}

int xiaomitouch_register_modedata(struct xiaomi_touch_interface *data)
{
	struct xiaomi_touch_interface *touch_data;

	if (!touch_pdata || !data)
		return -ENODEV;

	touch_data = touch_pdata->touch_data;
	mutex_lock(&xiaomi_touch_dev.mutex);
	touch_data->setModeValue = data->setModeValue;
	touch_data->getModeValue = data->getModeValue;
	touch_data->resetMode = data->resetMode;
	touch_data->getModeAll = data->getModeAll;
	touch_data->palm_sensor_read = data->palm_sensor_read;
	touch_data->palm_sensor_write = data->palm_sensor_write;
	touch_data->p_sensor_read = data->p_sensor_read;
	touch_data->p_sensor_write = data->p_sensor_write;
	mutex_unlock(&xiaomi_touch_dev.mutex);

	return 0;
}

int update_palm_sensor_value(int value)
{
	mutex_lock(&xiaomi_touch_dev.palm_mutex);
	if (!touch_pdata) {
		mutex_unlock(&xiaomi_touch_dev.palm_mutex);
		return -ENODEV;
	}

	if (value != touch_pdata->palm_value) {
		touch_pdata->palm_value = value;
		touch_pdata->palm_changed = true;
		wake_up_interruptible(&xiaomi_touch_dev.wait_queue);
	}
	mutex_unlock(&xiaomi_touch_dev.palm_mutex);

	return 0;
}

static ssize_t palm_sensor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct xiaomi_touch_pdata *pdata = dev_get_drvdata(dev);
	int ret;

	ret = wait_event_interruptible(xiaomi_touch_dev.wait_queue,
				       READ_ONCE(pdata->palm_changed));
	if (ret)
		return ret;

	WRITE_ONCE(pdata->palm_changed, false);
	return scnprintf(buf, PAGE_SIZE, "%d\n", READ_ONCE(pdata->palm_value));
}

static ssize_t palm_sensor_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct xiaomi_touch_pdata *pdata = dev_get_drvdata(dev);
	unsigned int input;
	int ret;

	ret = kstrtouint(buf, 0, &input);
	if (ret)
		return ret;
	if (!pdata->touch_data->palm_sensor_write)
		return -EOPNOTSUPP;

	ret = pdata->touch_data->palm_sensor_write(!!input);
	return ret < 0 ? ret : count;
}

int update_p_sensor_value(int value)
{
	mutex_lock(&xiaomi_touch_dev.psensor_mutex);
	if (!touch_pdata) {
		mutex_unlock(&xiaomi_touch_dev.psensor_mutex);
		return -ENODEV;
	}

	if (value != touch_pdata->psensor_value) {
		touch_pdata->psensor_value = value;
		touch_pdata->psensor_changed = true;
		wake_up_interruptible(&xiaomi_touch_dev.wait_queue);
	}
	mutex_unlock(&xiaomi_touch_dev.psensor_mutex);

	return 0;
}

static ssize_t p_sensor_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct xiaomi_touch_pdata *pdata = dev_get_drvdata(dev);
	int ret;

	ret = wait_event_interruptible(xiaomi_touch_dev.wait_queue,
				       READ_ONCE(pdata->psensor_changed));
	if (ret)
		return ret;

	WRITE_ONCE(pdata->psensor_changed, false);
	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 READ_ONCE(pdata->psensor_value));
}

static ssize_t p_sensor_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t count)
{
	struct xiaomi_touch_pdata *pdata = dev_get_drvdata(dev);
	unsigned int input;
	int ret;

	ret = kstrtouint(buf, 0, &input);
	if (ret)
		return ret;
	if (!pdata->touch_data->p_sensor_write)
		return -EOPNOTSUPP;

	ret = pdata->touch_data->p_sensor_write(!!input);
	return ret < 0 ? ret : count;
}

static DEVICE_ATTR_RW(palm_sensor);
static DEVICE_ATTR_RW(p_sensor);

static struct attribute *touch_attrs[] = {
	&dev_attr_palm_sensor.attr,
	&dev_attr_p_sensor.attr,
	NULL,
};

static const struct of_device_id xiaomi_touch_of_match[] = {
	{ .compatible = "xiaomi-touch" },
	{ }
};
MODULE_DEVICE_TABLE(of, xiaomi_touch_of_match);

static int xiaomi_touch_probe(struct platform_device *pdev)
{
	struct xiaomi_touch_pdata *pdata;
	int ret;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	ret = of_property_read_string(pdev->dev.of_node, "touch,name",
				      &pdata->name);
	if (ret)
		return ret;

	ret = misc_register(&xiaomi_touch_dev.misc_dev);
	if (ret)
		return ret;

	xiaomi_touch_dev.class = class_create(THIS_MODULE, "touch");
	if (IS_ERR(xiaomi_touch_dev.class)) {
		ret = PTR_ERR(xiaomi_touch_dev.class);
		xiaomi_touch_dev.class = NULL;
		goto err_misc;
	}

	xiaomi_touch_dev.dev = device_create(xiaomi_touch_dev.class, NULL,
					     XIAOMI_TOUCH_DEVT, NULL,
					     "touch_dev");
	if (IS_ERR(xiaomi_touch_dev.dev)) {
		ret = PTR_ERR(xiaomi_touch_dev.dev);
		xiaomi_touch_dev.dev = NULL;
		goto err_class;
	}

	pdata->touch_data = devm_kzalloc(&pdev->dev, sizeof(*pdata->touch_data),
					GFP_KERNEL);
	if (!pdata->touch_data) {
		ret = -ENOMEM;
		goto err_device;
	}

	pdata->device = &xiaomi_touch_dev;
	dev_set_drvdata(xiaomi_touch_dev.dev, pdata);
	platform_set_drvdata(pdev, pdata);
	touch_pdata = pdata;

	xiaomi_touch_dev.attrs.attrs = touch_attrs;
	ret = sysfs_create_group(&xiaomi_touch_dev.dev->kobj,
				 &xiaomi_touch_dev.attrs);
	if (ret)
		goto err_pdata;

	return 0;

err_pdata:
	touch_pdata = NULL;
	platform_set_drvdata(pdev, NULL);
err_device:
	device_destroy(xiaomi_touch_dev.class, XIAOMI_TOUCH_DEVT);
	xiaomi_touch_dev.dev = NULL;
err_class:
	class_destroy(xiaomi_touch_dev.class);
	xiaomi_touch_dev.class = NULL;
err_misc:
	misc_deregister(&xiaomi_touch_dev.misc_dev);
	return ret;
}

static int xiaomi_touch_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&xiaomi_touch_dev.dev->kobj, &xiaomi_touch_dev.attrs);
	touch_pdata = NULL;
	device_destroy(xiaomi_touch_dev.class, XIAOMI_TOUCH_DEVT);
	xiaomi_touch_dev.dev = NULL;
	class_destroy(xiaomi_touch_dev.class);
	xiaomi_touch_dev.class = NULL;
	misc_deregister(&xiaomi_touch_dev.misc_dev);
	return 0;
}

static struct platform_driver xiaomi_touch_driver = {
	.probe = xiaomi_touch_probe,
	.remove = xiaomi_touch_remove,
	.driver = {
		.name = "xiaomi-touch",
		.of_match_table = xiaomi_touch_of_match,
	},
};

static int __init xiaomi_touch_init(void)
{
	return platform_driver_register(&xiaomi_touch_driver);
}
subsys_initcall(xiaomi_touch_init);

static void __exit xiaomi_touch_exit(void)
{
	platform_driver_unregister(&xiaomi_touch_driver);
}
module_exit(xiaomi_touch_exit);

MODULE_DESCRIPTION("Xiaomi touch feature interface");
MODULE_LICENSE("GPL v2");