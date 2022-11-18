// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo using seqlock to read a value
 */

/* include module name in all messages */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/seqlock.h>

static struct global_data {
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
		pr_info("%s(): seq=%u len=%d\n", __func__, seq, len);
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

	pr_info("Called %s len=%zu\n", __func__, len);
	ret = kstrtoull_from_user(buf, len, 10, &val);
	if (ret < 0) {
		pr_err("kstrtoull ret %d\n", ret);
		return ret;
	}
	
	pr_info("seqlock write value=%llu\n", val);
	write_seqlock(&global.lock);
	global.counter = val;
	write_sequnlock(&global.lock);

	return len;
}

static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = demo_seqlock_read,
	.write  = demo_seqlock_write,
};

static struct miscdevice demo_miscdev = {
	.name = "demo_seqlock",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init demo_seqlock_init(void)
{
	seqlock_init(&global.lock);

	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot initialize misc dev\n");
		return -1;
	}
	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);
	return 0;
}

static void __exit demo_seqlock_exit(void)
{
	misc_deregister(&demo_miscdev);
}

module_init(demo_seqlock_init);
module_exit(demo_seqlock_exit);

MODULE_LICENSE("GPL");
