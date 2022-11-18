// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo usage of reader/writer lock
 */

/* include module name in all messages */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>

static struct global_data {
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

	schedule_delayed_work(&global.dwork, HZ);
}

static ssize_t demo_rwlock_read(struct file *filp, char __user *buf, size_t count,
				loff_t *off)
{
	char tmp[64];
	loff_t pos;
	ssize_t ret;
	int len;

	read_lock(&global.lock);
	len = snprintf(tmp, sizeof(tmp), "%lu\n", global.counter);
	read_unlock(&global.lock);

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
demo_rwlock_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	unsigned long val;
	int ret;

	ret = kstrtoul_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;
	
	write_lock(&global.lock);
	global.counter = val;
	write_unlock(&global.lock);

	return len;
}

/* this device does nothing */
static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = demo_rwlock_read,
	.write  = demo_rwlock_write,
};

static struct miscdevice demo_miscdev = {
	.name = "demo_rwlock",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init demo_rwlock_init(void)
{
	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot initialize misc dev\n");
		return -1;
	}
	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);

	rwlock_init(&global.lock);
	INIT_DELAYED_WORK(&global.dwork, counter_tick);
	schedule_delayed_work(&global.dwork, HZ);

	return 0;
}

static void __exit demo_rwlock_exit(void)
{
	pr_info("module exit\n");
	
	cancel_delayed_work_sync(&global.dwork);
	misc_deregister(&demo_miscdev);
}

module_init(demo_rwlock_init);
module_exit(demo_rwlock_exit);

MODULE_LICENSE("GPL");
