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

static struct class *demo_atomic_class;
static int demo_atomic_major;

static struct global_data {
	struct cdev cdev;
	atomic_t refcnt;
	atomic_long_t counter;
	struct delayed_work dwork;
} global;

/* Called from delayed work every second to increment counter. */
static void counter_tick(struct work_struct *work)
{
	atomic_long_inc(&global.counter);
	schedule_delayed_work(&global.dwork, HZ);
}

static ssize_t
demo_atomic_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	char tmp[64];
	loff_t pos;
	ssize_t ret;
	int len;

	len = snprintf(tmp, sizeof(tmp), "%ld\n",
		       atomic_long_read(&global.counter));

	pos = *off;
	if (pos >= len)
		return 0; /* read past eof */

	ret = min_t(ssize_t, count, len - pos);
	if (copy_to_user(buf, tmp + pos, ret))
		return -EFAULT;

	*off += ret;
	return ret;
}

static ssize_t
demo_atomic_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	long val;
	int ret;

	ret = kstrtol_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;
	
	atomic_long_set(&global.counter, val);

	return len;
}

static int demo_atomic_open(struct inode *inode, struct file *file)
{
	if (atomic_inc_return(&global.refcnt) == 1) {
		pr_info("starting work\n");
		schedule_delayed_work(&global.dwork, HZ);
	}

	return 0;
}

static int demo_atomic_release(struct inode *inode, struct file *file)
{
	if (atomic_dec_and_test(&global.refcnt)) {
		pr_info("stopping timer\n");
		cancel_delayed_work(&global.dwork);
	}

	return 0;
}


static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = demo_atomic_read,
	.write  = demo_atomic_write,
	.open   = demo_atomic_open,
	.release = demo_atomic_release,
};

static int __init demo_atomic_init(void)
{
	struct device *cdev;
	int retval;
	dev_t dev;
	
	retval = alloc_chrdev_region(&dev, 0, 1, "demo_atomic");
	if (retval < 0) {
		pr_err("Cannot allocate major number\n");
		goto err_region;
	}
	demo_atomic_major = MAJOR(dev);

	atomic_long_set(&global.counter, 0);
	atomic_set(&global.refcnt, 0);

	INIT_DELAYED_WORK(&global.dwork, counter_tick);
	cdev_init(&global.cdev, &fops);

	retval = cdev_add(&global.cdev, dev, 1);
	if (retval < 0) {
		pr_err("Cannot add the device to the system\n");
		goto err_add;
	}

	demo_atomic_class = class_create(THIS_MODULE, "demo_atomic");
	if (IS_ERR(demo_atomic_class)) {
		retval = PTR_ERR(demo_atomic_class);
		pr_err("cannot create the struct class\n");
		goto err_class;
	}

	cdev = device_create(demo_atomic_class, NULL, dev, NULL, "demo_atomic");
	if (IS_ERR(cdev)) {
		retval = PTR_ERR(cdev);
		pr_err("cannot create the Device\n");
		goto err_device;
	}

	pr_info("Created demo device %d\n", demo_atomic_major);
	return 0;

err_device:
	class_destroy(demo_atomic_class);
err_class:
	cdev_del(&global.cdev);
err_add:
	unregister_chrdev_region(dev, 1);
err_region:
	return retval;
}

static void __exit demo_atomic_exit(void)
{
	device_destroy(demo_atomic_class, MKDEV(demo_atomic_major, 0));
	cdev_del(&global.cdev);

	unregister_chrdev_region(MKDEV(demo_atomic_major, 0), 1);
	class_destroy(demo_atomic_class);

	pr_info("Device Driver Remove...Done!!\n");
}

module_init(demo_atomic_init);
module_exit(demo_atomic_exit);

MODULE_LICENSE("GPL");
