// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo using wait queue
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
#include <linux/wait.h>

static struct global_data {
	struct wait_queue_head wq;
	atomic_long_t counter;
} global;

struct user_data {
	long lastread;
};

static bool
counter_changed(long *counter_ret)
{
	long value = atomic_long_read(&global.counter);

	if (value == *counter_ret)
		return false;

	*counter_ret = value;
	return true;
}

static ssize_t
demo_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	struct user_data *ud = filp->private_data;
	char tmp[64];
	loff_t pos;
	ssize_t ret;
	int len;

	if (wait_event_interruptible(global.wq, counter_changed(&ud->lastread)))
		return -EINTR;

	len = snprintf(tmp, sizeof(tmp), "%ld\n", ud->lastread);
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
demo_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	long val;
	int ret;

	ret = kstrtol_from_user(buf, len, 10, &val);
	if (ret < 0)
		return ret;

	atomic_long_set(&global.counter, val);
	wake_up_interruptible(&global.wq);

	return len;
}

static int demo_open(struct inode *inode, struct file *filp)
{
	struct user_data *ud;

	ud = kmalloc(sizeof(*ud), GFP_USER);
	if (ud == NULL)
		return -ENOMEM;

	ud->lastread = atomic_long_read(&global.counter);
	filp->private_data = ud;
	return 0;
}

static int demo_release(struct inode *inode, struct file *filp)
{
	struct user_data *ud = filp->private_data;

	kfree(ud);
	return 0;
}

static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = demo_read,
	.write  = demo_write,
	.open   = demo_open,
	.release = demo_release,
};

static struct miscdevice demo_miscdev = {
	.name = "demo_wait",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init demo_init(void)
{

	atomic_long_set(&global.counter, 0);
	init_waitqueue_head(&global.wq);

	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot initialize misc dev\n");
		return -1;
	}

	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);
	return 0;
}

static void __exit demo_exit(void)
{
	misc_deregister(&demo_miscdev);
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
