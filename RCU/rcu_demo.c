// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo usage of RCU to read a counter
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rcupdate.h>

static struct class *demo_rcu_class;
static int demo_rcu_major;

struct save_value {
	struct rcu_head rcu;
	char str[];
};

static struct global_data {
	struct cdev cdev;
	struct save_value __rcu *cur_data;
} global;

static ssize_t
demo_rcu_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	struct save_value *data;
	loff_t pos;
	ssize_t ret;
	size_t len;

	rcu_read_lock();
	data = rcu_dereference(global.cur_data);
	if (data == NULL) {
		ret = -EINVAL;	/* no value has been set */
	} else {
		len = strlen(data->str);
		ret = min_t(ssize_t, count, len - pos);
		if (copy_to_user(buf, data->str + pos, ret))
			ret = -EFAULT;
		else
			*off += ret;
	}
	rcu_read_unlock();
	return ret;
}

static ssize_t
demo_rcu_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	struct save_value *data;
	int ret;

	data = kmalloc(sizeof(*data) + len, GFP_USER);
	if (data == NULL)
		return -ENOMEM;

	ret = strncpy_from_user(data->str, buf, len);
	if (ret < 0)
		return ret;

	/* replace original pointer with new value */
	data = xchg(&global.cur_data, data);

	/* Free old one after grace period */
	if (data != NULL)
		kfree_rcu(data, rcu);

	return len;
}

static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = demo_rcu_read,
	.write  = demo_rcu_write,
};

static int __init demo_rcu_init(void)
{
	struct device *cdev;
	int retval;
	dev_t dev;

	retval = alloc_chrdev_region(&dev, 0, 1, "demo_rcu");
	if (retval < 0) {
		pr_err("Cannot allocate major number\n");
		goto err_region;
	}
	demo_rcu_major = MAJOR(dev);

	RCU_INIT_POINTER(global.cur_data, NULL);

	cdev_init(&global.cdev, &fops);

	retval = cdev_add(&global.cdev, dev, 1);
	if (retval < 0) {
		pr_err("Cannot add the device to the system\n");
		goto err_add;
	}

	demo_rcu_class = class_create(THIS_MODULE, "demo_rcu");
	if (IS_ERR(demo_rcu_class)) {
		retval = PTR_ERR(demo_rcu_class);
		pr_err("cannot create the struct class\n");
		goto err_class;
	}

	cdev = device_create(demo_rcu_class, NULL, dev, NULL, "demo_rcu");
	if (IS_ERR(cdev)) {
		retval = PTR_ERR(cdev);
		pr_err("cannot create the Device\n");
		goto err_device;
	}

	pr_info("Created demo device %d\n", demo_rcu_major);
	return 0;

err_device:
	class_destroy(demo_rcu_class);
err_class:
	cdev_del(&global.cdev);
err_add:
	unregister_chrdev_region(dev, 1);
err_region:
	return retval;
}

static void __exit demo_rcu_exit(void)
{
	kfree(global.cur_data);	/* why is this safe? */

	device_destroy(demo_rcu_class, MKDEV(demo_rcu_major, 0));
	cdev_del(&global.cdev);

	unregister_chrdev_region(MKDEV(demo_rcu_major, 0), 1);
	class_destroy(demo_rcu_class);
}

module_init(demo_rcu_init);
module_exit(demo_rcu_exit);

MODULE_LICENSE("GPL");
