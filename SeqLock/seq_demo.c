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
#include <linux/seqlock.h>

static struct class *demo_seqlock_class;
static int demo_seqlock_major;

static struct global_data {
	struct cdev cdev;
	seqlock_t lock;
	unsigned long long counter;
} global;

static ssize_t
demo_seqlock_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	unsigned seq;
	char tmp[64];
	loff_t pos;
	ssize_t ret;
	int len;

	do {
		seq = read_seqbegin(&global.lock);
		len = snprintf(tmp, sizeof(tmp),
			       "%llu\n", global.counter);
	} while (read_seqretry(&global.lock, seq));

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
demo_seqlock_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	unsigned long long val;
	int ret;

	ret = kstrtoull_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;
	
	write_seqlock(&global.lock);
	global.counter = val;
	write_sequnlock(&global.lock);

	return len;
}

static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = demo_seqlock_read,
	.write  = demo_seqlock_write,
};

static int __init demo_seqlock_init(void)
{
	struct device *cdev;
	int retval;
	dev_t dev;
	
	retval = alloc_chrdev_region(&dev, 0, 1, "demo_seqlock");
	if (retval < 0) {
		pr_err("Cannot allocate major number\n");
		goto err_region;
	}
	demo_seqlock_major = MAJOR(dev);

	seqlock_init(&global.lock);

	cdev_init(&global.cdev, &fops);

	retval = cdev_add(&global.cdev, dev, 1);
	if (retval < 0) {
		pr_err("Cannot add the device to the system\n");
		goto err_add;
	}

	demo_seqlock_class = class_create(THIS_MODULE, "demo_seqlock");
	if (IS_ERR(demo_seqlock_class)) {
		retval = PTR_ERR(demo_seqlock_class);
		pr_err("cannot create the struct class\n");
		goto err_class;
	}

	cdev = device_create(demo_seqlock_class, NULL, dev, NULL, "demo_seqlock");
	if (IS_ERR(cdev)) {
		retval = PTR_ERR(cdev);
		pr_err("cannot create the Device\n");
		goto err_device;
	}

	pr_info("Created demo device %d\n", demo_seqlock_major);
	return 0;

err_device:
	class_destroy(demo_seqlock_class);
err_class:
	cdev_del(&global.cdev);
err_add:
	unregister_chrdev_region(dev, 1);
err_region:
	return retval;
}

static void __exit demo_seqlock_exit(void)
{
	device_destroy(demo_seqlock_class, MKDEV(demo_seqlock_major, 0));
	cdev_del(&global.cdev);

	unregister_chrdev_region(MKDEV(demo_seqlock_major, 0), 1);
	class_destroy(demo_seqlock_class);

	pr_info("Device Driver Remove...Done!!\n");
}

module_init(demo_seqlock_init);
module_exit(demo_seqlock_exit);

MODULE_LICENSE("GPL");
