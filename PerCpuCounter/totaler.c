// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo of totaling with percpu counter
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/percpu_counter.h>
#include <linux/uaccess.h>

static struct class *totaler_class;
static int totaler_major;

static struct global_data {
	struct cdev cdev;
	struct percpu_counter pcpu;
} global;

static ssize_t
totaler_read(struct file *filp, char __user *buf, size_t count,
	     loff_t *off)
{
	char tmp[64];
	loff_t pos;
	ssize_t ret;
	int len;
	s64 total;

	total = percpu_counter_sum(&global.pcpu);
	
	len = snprintf(tmp, sizeof(tmp), "%ld\n", (long)total);

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
totaler_write(struct file *filp, const char __user *buf, size_t len,
			  loff_t *off)
{
	int ret;
	long val;

	ret = kstrtol_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;
	
	percpu_counter_add(&global.pcpu, val);

	return len;
}

static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = totaler_read,
	.write  = totaler_write,
};

static int __init totaler_init(void)
{
	struct device *cdev;
	int retval;
	dev_t dev;
	
	retval = alloc_chrdev_region(&dev, 0, 1, "totaler");
	if (retval < 0) {
		pr_err("Cannot allocate major number\n");
		goto err_region;
	}
	totaler_major = MAJOR(dev);

	retval = percpu_counter_init(&global.pcpu, 0, GFP_KERNEL);
	if (retval < 0) {
		pr_err("Cannot init counter\n");
		goto err_counter;
	}

	cdev_init(&global.cdev, &fops);
	retval = cdev_add(&global.cdev, dev, 1);
	if (retval < 0) {
		pr_err("Cannot add the device to the system\n");
		goto err_add;
	}

	totaler_class = class_create(THIS_MODULE, "atomic_demo");
	if (IS_ERR(totaler_class)) {
		retval = PTR_ERR(totaler_class);
		pr_err("cannot create the struct class\n");
		goto err_class;
	}

	cdev = device_create(totaler_class, NULL, dev, NULL, "totaler");
	if (IS_ERR(cdev)) {
		retval = PTR_ERR(cdev);
		pr_err("cannot create the Device\n");
		goto err_device;
	}

	pr_info("Created demo device %d\n", totaler_major);
	return 0;

err_device:
	class_destroy(totaler_class);
err_class:
	cdev_del(&global.cdev);
err_add:
	percpu_counter_destroy(&global.pcpu);
err_counter:
	unregister_chrdev_region(dev, 1);
err_region:
	return retval;
}

static void __exit totaler_exit(void)
{
	device_destroy(totaler_class, MKDEV(totaler_major, 0));
	cdev_del(&global.cdev);

	unregister_chrdev_region(MKDEV(totaler_major, 0), 1);
	class_destroy(totaler_class);
	percpu_counter_destroy(&global.pcpu);

	pr_info("Device Driver Remove...Done!!\n");
}

module_init(totaler_init);
module_exit(totaler_exit);

MODULE_LICENSE("GPL");
