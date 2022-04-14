// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo of spin lock
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
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

DEFINE_SPINLOCK(demo_spinlock);

static unsigned long demo_global_variable = 0;

static dev_t dev;
static struct class *dev_class;
static struct cdev demo_cdev;

static int __init demo_spin_init(void);
static void __exit demo_spin_exit(void);

static struct task_struct *demo_thread1;
static struct task_struct *demo_thread2;

static int thread_function1(void *pv)
{
	while (!kthread_should_stop()) {
		spin_lock(&demo_spinlock);
		demo_global_variable++;
		pr_info("In %s %lu\n", __func__, demo_global_variable);
		spin_unlock(&demo_spinlock);

		msleep(1000);
	}
	return 0;
}

static int thread_function2(void *pv)
{
	while (!kthread_should_stop()) {
		spin_lock(&demo_spinlock);
		demo_global_variable++;

		pr_info("In %s Function2 %lu\n", __func__,
			demo_global_variable);
		spin_unlock(&demo_spinlock);
		msleep(1000);
	}
	return 0;
}

static int demo_open(struct inode *inode, struct file *file)
{
	pr_info("Device File Opened...!!!\n");
	return 0;
}

static int demo_release(struct inode *inode, struct file *file)
{
	pr_info("Device File Closed...!!!\n");
	return 0;
}

static ssize_t demo_read(struct file *filp, char __user *buf, size_t len,
			 loff_t *off)
{
	pr_info("Read function\n");
	return 0;
}

static ssize_t demo_write(struct file *filp, const char __user *buf, size_t len,
			  loff_t *off)
{
	pr_info("Write Function\n");
	return len;
}

static const struct file_operations fops = {
	.owner  = THIS_MODULE,
	.read   = demo_read,
	.write  = demo_write,
	.open   = demo_open,
	.release = demo_release,
};

static int __init demo_spin_init(void)
{
	if (alloc_chrdev_region(&dev, 0, 1, "spin_demo") < 0) {
		pr_err("Cannot allocate major number\n");
		return -1;
	}
	pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));

	cdev_init(&demo_cdev, &fops);

	if (cdev_add(&demo_cdev, dev, 1) < 0) {
		pr_err("Cannot add the device to the system\n");
		goto r_class;
	}

	dev_class = class_create(THIS_MODULE, "spin_demo");
	if (dev_class == NULL) {
		pr_err("Cannot create the struct class\n");
		goto r_class;
	}

	if (!device_create(dev_class, NULL, dev, NULL, "spin_demo")) {
		pr_info("Cannot create the Device\n");
		goto r_device;
	}

	demo_thread1 = kthread_run(thread_function1, NULL, "demo thread1");
	if (demo_thread1) {
		pr_info("Kthread1 Created Successfully...\n");
	} else {
		pr_err("Cannot create kthread1\n");
		goto r_device;
	}

	demo_thread2 = kthread_run(thread_function2, NULL, "demo thread2");
	if (demo_thread2) {
		pr_info("Kthread2 Created Successfully...\n");
	} else {
		pr_err("Cannot create kthread2\n");
		goto r_device;
	}

	pr_info("Device Driver Insert...Done!!!\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	cdev_del(&demo_cdev);
	return -1;
}

static void __exit demo_spin_exit(void)
{
	kthread_stop(demo_thread1);
	kthread_stop(demo_thread2);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&demo_cdev);
	unregister_chrdev_region(dev, 1);
	pr_info("Device Driver Remove...Done!!\n");
}

module_init(demo_spin_init);
module_exit(demo_spin_exit);

MODULE_LICENSE("GPL");
