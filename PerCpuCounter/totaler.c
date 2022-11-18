// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo of totaling with percpu counter
 */

/* include module name in all messages */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/percpu_counter.h>
#include <linux/uaccess.h>

static struct global_data {
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

static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = totaler_read,
	.write  = totaler_write,
};

static struct miscdevice demo_miscdev = {
	.name = "demo_percpu",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init totaler_init(void)
{
	if (percpu_counter_init(&global.pcpu, 0, GFP_KERNEL) != 0) {
		pr_err("Cannot init counter\n");
		return -1;
	}

	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot register miscdev\n");
		return -1;
	}

	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);
	return 0;
}

static void __exit totaler_exit(void)
{
	misc_deregister(&demo_miscdev);
	percpu_counter_destroy(&global.pcpu);
}

module_init(totaler_init);
module_exit(totaler_exit);

MODULE_LICENSE("GPL");
