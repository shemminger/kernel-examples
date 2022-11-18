// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo usage of atomic for a counter
 */

/* include module name in all messages */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>

static struct global_data {
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

static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = demo_atomic_read,
	.write  = demo_atomic_write,
	.open   = demo_atomic_open,
	.release = demo_atomic_release,
};


static struct miscdevice demo_miscdev = {
	.name = "demo_atomic",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init demo_atomic_init(void)
{
	atomic_long_set(&global.counter, 0);
	atomic_set(&global.refcnt, 0);
	INIT_DELAYED_WORK(&global.dwork, counter_tick);

	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot initialize misc dev\n");
		return -1;
	}

	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);
	return 0;

}

static void __exit demo_atomic_exit(void)
{
	cancel_delayed_work_sync(&global.dwork);
	misc_deregister(&demo_miscdev);
}

module_init(demo_atomic_init);
module_exit(demo_atomic_exit);

MODULE_LICENSE("GPL");
