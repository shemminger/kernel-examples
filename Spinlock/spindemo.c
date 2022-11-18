// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Demo of spin lock
 */

/* include module name in all messages */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

DEFINE_SPINLOCK(demo_spinlock);

static unsigned long demo_global_variable = 0;

static struct task_struct *demo_thread1;
static struct task_struct *demo_thread2;

static int thread_function(void *arg)
{
	const char *my_name = arg;

	while (!kthread_should_stop()) {
		spin_lock(&demo_spinlock);
		demo_global_variable++;
		pr_info("%s = %lu\n", my_name, demo_global_variable);
		spin_unlock(&demo_spinlock);

		msleep(1000);
	}
	return 0;
}

static int __init demo_spin_init(void)
{
	demo_thread1 = kthread_run(thread_function, "thread1", "spin_demo1");
	if (demo_thread1 == NULL) {
		pr_err("Cannot create kthread1\n");
		return -1;
	}

	demo_thread2 = kthread_run(thread_function, "thread2", "spin_demo2");
	if (demo_thread2 == NULL) {
		pr_err("Cannot create kthread2\n");
		kthread_stop(demo_thread1);
		return -1;
	}

	pr_info("module inserted\n");
	return 0;

}

static void __exit demo_spin_exit(void)
{
	pr_info("module exit\n");
	
	kthread_stop(demo_thread1);
	kthread_stop(demo_thread2);
}

module_init(demo_spin_init);
module_exit(demo_spin_exit);

MODULE_LICENSE("GPL");
