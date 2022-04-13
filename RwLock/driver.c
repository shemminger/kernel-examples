// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo of bad usage of spinlock
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
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>

static struct class *demo_class;
static int demo_major;

static struct global_data {
	struct cdev cdev;
	unsigned long counter;

	rwlock_t lock;

	struct delayed_work dwork;
} global;

/* Called from delayed work every second to increment counter. */
static void counter_tick(struct work_struct *work)
{
	write_lock(&global.lock);
	++global.counter;
	write_unlock(&global.lock);

	schedule_delayed_work(&global.dwork, HZ); /* or msecs_to_jiffies(1000)) */
}

static ssize_t demo_read(struct file *filp, char __user *buf, size_t count,
			 loff_t *off)
{
	char tmp[64];
	int len;

	read_lock(&global.lock);
	len = snprintf(tmp, sizeof(tmp), "%lu\n", global.counter);
	read_unlock(&global.lock);

	count = min_t(size_t, count, len);
	
	/* XXX doesn't do partial read */
	if (copy_to_user(buf, tmp, count))
		return -EFAULT;

	return count;
}

static ssize_t demo_write(struct file *filp, const char __user *buf, size_t len,
			  loff_t *off)
{
	unsigned long val;
	int ret;

	ret = kstrtoul_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;
	
	write_lock(&global.lock);
	global.counter = val;
	write_lock(&global.lock);

	return len;
}

static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = demo_read,
	.write  = demo_write,
};

static int __init demo_driver_init(void)
{
	struct device *cdev;
	int retval;
	dev_t dev;
	
	retval = alloc_chrdev_region(&dev, 0, 1, "demo_dev");
	if (retval < 0) {
		pr_err("Cannot allocate major number\n");
		goto err_region;
	}
	demo_major = MAJOR(dev);

	rwlock_init(&global.lock);
	INIT_DELAYED_WORK(&global.dwork, counter_tick);
	cdev_init(&global.cdev, &fops);

	retval = cdev_add(&global.cdev, dev, 1);
	if (retval < 0) {
		pr_err("Cannot add the device to the system\n");
		goto err_add;
	}

	demo_class = class_create(THIS_MODULE, "counter_demo");
	if (IS_ERR(demo_class)) {
		retval = PTR_ERR(demo_class);
		pr_err("Cannot create the struct class\n");
		goto err_class;
	}

	cdev = device_create(demo_class, NULL, dev, NULL, "demo_device");
	if (IS_ERR(cdev)) {
		retval = PTR_ERR(cdev);
		pr_info("Cannot create the Device\n");
		goto err_device;
	}

	pr_info("Created demo device %d\n", demo_major);
	return 0;

err_device:
	class_destroy(demo_class);
err_class:
	cdev_del(&global.cdev);
err_add:
	unregister_chrdev_region(dev, 1);
err_region:
	return retval;
}

static void __exit demo_driver_exit(void)
{
	device_destroy(demo_class, MKDEV(demo_major, 0));
	cdev_del(&global.cdev);

	unregister_chrdev_region(MKDEV(demo_major, 0), 1);
	class_destroy(demo_class);

	pr_info("Device Driver Remove...Done!!\n");
}

module_init(demo_driver_init);
module_exit(demo_driver_exit);

MODULE_LICENSE("GPL");
