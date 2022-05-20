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
static struct task_struct *demo_thread1;
static struct task_struct *demo_thread2;

static int thread_function(void *arg)
{
	const char *my_name = arg;

	while (!kthread_should_stop()) {
		spin_lock(&demo_spinlock);
		demo_global_variable++;
		pr_info("spin_demo %s %lu\n", my_name, demo_global_variable);
		spin_unlock(&demo_spinlock);

		msleep(1000);
	}
	return 0;
}

/* this device does nothing */
static const struct file_operations fops = {
	.owner  = THIS_MODULE,
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

	demo_thread1 = kthread_run(thread_function, "thread1", "spin_demo1");
	if (demo_thread1) {
		pr_info("Kthread1 Created Successfully...\n");
	} else {
		pr_err("Cannot create kthread1\n");
		goto r_device;
	}

	demo_thread2 = kthread_run(thread_function, "thread2", "spin_demo2");
	if (demo_thread2) {
		pr_info("Kthread2 Created Successfully...\n");
	} else {
		pr_err("Cannot create kthread2\n");
		goto r_device;
	}

	pr_info("Inserted spin_demo\n");
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
	pr_info("Remove spin_demo\n");
}

module_init(demo_spin_init);
module_exit(demo_spin_exit);

MODULE_LICENSE("GPL");
