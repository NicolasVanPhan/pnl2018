#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "taskmonitor.h"

MODULE_DESCRIPTION("A process monitor");
MODULE_AUTHOR("Maxime Lorrillere <maxime.lorrillere@lip6.fr>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");

unsigned short target = 1; /* default pid to monitor */
unsigned frequency = 1; /* sampling frequency */

module_param(target, ushort, 0400);
module_param(frequency, uint, 0600);

struct task_monitor {
	struct pid *pid;
};

struct task_monitor *tm;

struct task_struct *monitor_thread;

struct task_sample {
	cputime_t utime;
	cputime_t stime;
};


/* -------------------------------------------------------------------------- */
/* ---------- Exercice 6 : Adding a ioctl interface ------------------------- */
/* -------------------------------------------------------------------------- */

bool get_sample(struct task_monitor *tm, struct task_sample *sample);
int monitor_fn(void *data);
int monitor_pid(pid_t pid);

static int major;

long	taskmonitor_ioctl (struct file *f, unsigned int cmd, unsigned long arg)
{
	char mystr[IOC_MAXLEN];
	struct task_sample ts;
	struct task_monitor *old_tm;
	switch (cmd) {
		case GET_SAMPLE :
			if (!get_sample(tm, &ts))
				return 0;
			scnprintf(mystr, IOC_MAXLEN, "%d; user %ld; sys %ld",
			pid_nr(tm->pid), ts.utime, ts.stime);
			copy_to_user((char*)arg, mystr, IOC_MAXLEN);
			return 0;
			break;
		case TASKMON_START :
			if (monitor_thread > 0)
				break;
			monitor_thread = kthread_run(monitor_fn, NULL,
				"monitor_fn");
			break;
		case TASKMON_STOP :
			if (monitor_thread <= 0)
				break;
			kthread_stop(monitor_thread);
			monitor_thread = NULL;
			break;
		case TASKMON_SET_PID :
			old_tm = tm;
			if (monitor_pid(arg) == 0) {
				put_pid(old_tm->pid);
				kfree(old_tm);
			}
			break;
		default :
			return -ENOTTY;
			break;
	}
	return 0;
}

static struct file_operations fops_taskmonitor = {
	.unlocked_ioctl = taskmonitor_ioctl 
	};


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

bool get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	struct task_struct *task;
	bool alive = false;

	task = get_pid_task(tm->pid, PIDTYPE_PID);
	if (!task) {
		pr_err("can't find task for pid %u\n", pid_nr(tm->pid));
		goto out;
	}

	task_lock(task);
	alive = pid_alive(task);
	if (alive)
		task_cputime(task, &sample->utime, &sample->stime);
	task_unlock(task);
	put_task_struct(task);
out:
	return alive;
}

void print_sample(struct task_monitor *tm)
{
	struct task_sample ts;
	pid_t pid = pid_nr(tm->pid);
	bool alive;

	alive = get_sample(tm, &ts);

	if (!alive)
		pr_err("%hd is dead\n",	pid);
	else
		pr_info("%hd usr %lu sys %lu\n", pid, ts.utime, ts.stime);
}

int monitor_fn(void *data)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (schedule_timeout(max(HZ/frequency, 1U)))
			return -EINTR;

		print_sample(tm);
	}
	return 0;
}

int monitor_pid(pid_t pid)
{
	struct pid *p = find_get_pid(pid);

	if (!p) {
		pr_err("pid %hu not found\n", pid);
		return -ESRCH;
	}
	tm = kmalloc(sizeof(*tm), GFP_KERNEL);
	tm->pid = p;

	return 0;
}

static int monitor_init(void)
{
	int err = monitor_pid(target);

	if (err)
		return err;

	/*
	monitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if (IS_ERR(monitor_thread)) {
		err = PTR_ERR(monitor_thread);
		goto abort;
	}
	*/

	major = register_chrdev(0, IOC_DEVNAME, &fops_taskmonitor);
	pr_info("TaskMonitorIoctl : Module loaded\n");
	pr_info("TaskMonitorIoctl : major = %d\n", major);

	pr_info("Monitoring module loaded\n");
	return 0;

abort:
	put_pid(tm->pid);
	kfree(tm);
	return err;
}

static void monitor_exit(void)
{
	if (monitor_thread)
		kthread_stop(monitor_thread);

	put_pid(tm->pid);
	kfree(tm);

	unregister_chrdev(major, IOC_DEVNAME);
	pr_info("TaskMonitorIoctl : Module unloaded\n");

	pr_info("Monitoring module unloaded\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
