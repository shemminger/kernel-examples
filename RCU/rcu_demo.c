// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo usage of RCU to read a counter
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
#include <linux/rcupdate.h>

struct save_value {
	struct rcu_head rcu;
	char str[];
};

static struct save_value __rcu *global;

static ssize_t
demo_rcu_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
	struct save_value *data;
	loff_t pos;
	ssize_t ret;
	size_t len;

	rcu_read_lock();
	data = rcu_dereference(global);
	if (data == NULL) {
		ret = -EINVAL;	/* no value has been set */
	} else {
		len = strlen(data->str);
		ret = min_t(ssize_t, count, len - pos);
		if (copy_to_user(buf, data->str + pos, ret))
			ret = -EFAULT;
		else
			*off += ret;
	}
	rcu_read_unlock();
	return ret;
}

static ssize_t
demo_rcu_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	struct save_value *data;
	int ret;

	data = kmalloc(sizeof(*data) + len, GFP_USER);
	if (data == NULL)
		return -ENOMEM;

	ret = strncpy_from_user(data->str, buf, len);
	if (ret < 0)
		return ret;

	/* replace original pointer with new value */
	data = xchg(&global, data);

	/* Free old one after grace period */
	if (data != NULL)
		kfree_rcu(data, rcu);

	return len;
}

static const struct file_operations demo_fops = {
	.owner  = THIS_MODULE,
	.read   = demo_rcu_read,
	.write  = demo_rcu_write,
};

static struct miscdevice demo_miscdev = {
	.name = "demo_percpu",
	.minor = MISC_DYNAMIC_MINOR,
	.fops = &demo_fops,
};

static int __init demo_rcu_init(void)
{
	RCU_INIT_POINTER(global, NULL);

	if (misc_register(&demo_miscdev) != 0) {
		pr_err("Cannot register miscdev\n");
		return -1;
	}

	pr_info("node %d:%d\n", MISC_MAJOR, demo_miscdev.minor);
	return 0;
}

static void __exit demo_rcu_exit(void)
{
	misc_deregister(&demo_miscdev);
	
	kfree(global);	/* why is this safe? */
}

module_init(demo_rcu_init);
module_exit(demo_rcu_exit);

MODULE_LICENSE("GPL");
