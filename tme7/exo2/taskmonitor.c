#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/shrinker.h>

MODULE_DESCRIPTION("A process monitor");
MODULE_AUTHOR("Maxime Lorrillere <maxime.lorrillere@lip6.fr>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");

unsigned short target = 1; /* default pid to monitor */
unsigned frequency = 100; /* sampling frequency */

module_param(target, ushort, 0400);
module_param(frequency, uint, 0600);

struct task_struct *monitor_thread;

struct task_sample {
	struct list_head	list;
	cputime_t		utime;
	cputime_t		stime;
};

struct task_monitor {
	struct pid		*pid;
	struct task_sample	phantom;
	struct mutex		lock;
	int			nsamples;
};

struct task_monitor *tm;

/* -------- Prototypes ------------------------------------------------------ */
static ssize_t taskmonitor_attr_show (struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf);
static ssize_t taskmonitor_attr_store (struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count);
bool get_sample(struct task_monitor *tm, struct task_sample *sample);
void print_sample(struct task_monitor *tm);
int monitor_fn(void *data);
int monitor_pid(pid_t pid);
static int monitor_init(void);
static void monitor_exit(void);

/* -------------------------------------------------------------------------- */
/* ---------- TME7 Exercice 2 : Shrinker ------------------------------------ */
/* -------------------------------------------------------------------------- */

unsigned long taskmon_count(struct shrinker* s, struct shrink_control *sc)
{
	int	count;

	mutex_lock(&tm->lock);
	count = tm->nsamples;
	mutex_unlock(&tm->lock);
	return count;
}

unsigned long taskmon_scan(struct shrinker* s, struct shrink_control *sc)
{
	struct task_sample	*pts;
	struct list_head	*plh;
	int			nr_to_scan;
	long			freed;

	freed = 0;
	nr_to_scan = sc->nr_to_scan;
	mutex_lock(&tm->lock);
	while (!list_empty(&tm->phantom.list)) {
		if (freed >= nr_to_scan)
			break;
		plh = tm->phantom.list.next;
		pts = list_entry(plh, struct task_sample, list);
		list_del(plh);
		kfree(pts);		
		tm->nsamples--;
		freed++;
	}
	mutex_unlock(&tm->lock);
	return freed;
}

struct shrinker taskmon_shrinker = {
	.count_objects = taskmon_count,
	.scan_objects = taskmon_scan,
	.seeks = DEFAULT_SEEKS,
	.batch = 0,
	.flags = 0};

/* -------------------------------------------------------------------------- */
/* ---------- TME6 Exercice 5 : Adding a sysfs interface -------------------- */
/* -------------------------------------------------------------------------- */


static ssize_t taskmonitor_attr_show (struct kobject *kobj,
	struct kobj_attribute *attr,
	char *buf)
{
	struct task_sample *pts;
	const int maxlen = 20;
	ssize_t	count;

	/* TME7 - Exercice 1 */
	count = 0;
	count += scnprintf(buf + count, maxlen, "Nb samples : %d\n", tm->nsamples);
	list_for_each_entry(pts, &tm->phantom.list, list) {
		if (count >= PAGE_SIZE - maxlen)
			break;
		count += scnprintf(buf + count, maxlen, "%d; usr %ld; sys %ld\n",
			target,	pts->utime, pts->stime);
	}
	return count;
}

static ssize_t taskmonitor_attr_store (struct kobject *kobj,
	struct kobj_attribute *attr,
	const char *buf,
	size_t count)
{
	/* If input string == "start" and no thread is running */
	if (!strcmp(buf, "start") && monitor_thread <= 0) {
		monitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	}

	/* If input string == "stop" and there's a thread running */
	if (!strcmp(buf, "stop") && monitor_thread > 0) {
		kthread_stop(monitor_thread);
		monitor_thread = NULL;
	}

	return count;
}

static struct kobj_attribute taskmonitor_attr = __ATTR_RW(taskmonitor_attr);

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

/* TME7 : Exercice 1 */
void save_sample(struct task_monitor *tm)
{
	struct task_sample *ts;
	//pid_t pid = pid_nr(tm->pid);
	bool alive;

	ts = kmalloc(sizeof(struct task_sample), GFP_KERNEL);
	alive = get_sample(tm, ts);

	if (alive) {
		mutex_lock(&tm->lock);
		list_add_tail(&ts->list, &tm->phantom.list);
		tm->nsamples++;
		mutex_unlock(&tm->lock);
	} else {
		kfree(ts);
	}
}

int monitor_fn(void *data)
{
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (schedule_timeout(max(HZ/frequency, 1U)))
			return -EINTR;

		//print_sample(tm);
		save_sample(tm);
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
	/* TME7 : Exercice 1 */
	INIT_LIST_HEAD(&tm->phantom.list);
	mutex_init(&tm->lock);
	tm->nsamples = 0;

	return 0;
}

static int monitor_init(void)
{
	int err = monitor_pid(target);

	if (err)
		return err;

	monitor_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if (IS_ERR(monitor_thread)) {
		err = PTR_ERR(monitor_thread);
		goto abort;
	}

	sysfs_create_file(kernel_kobj, &(taskmonitor_attr.attr));
	register_shrinker(&taskmon_shrinker);
	pr_info("Monitoring module loaded\n");
	return 0;

abort:
	put_pid(tm->pid);
	kfree(tm);
	return err;
}

static void monitor_exit(void)
{
	struct task_sample *pts;
	struct list_head   *plh;

	if (monitor_thread)
		kthread_stop(monitor_thread);

	/* TME7 - Exercice 1 */
	mutex_lock(&tm->lock);
	while (!list_empty(&tm->phantom.list)) {
		plh = tm->phantom.list.next;
		pts = list_entry(plh, struct task_sample, list);
		list_del(plh);
		kfree(pts);		
	}
	mutex_unlock(&tm->lock);
	put_pid(tm->pid);
	kfree(tm);

	unregister_shrinker(&taskmon_shrinker);
	sysfs_remove_file(kernel_kobj, &(taskmonitor_attr.attr));
	pr_info("Monitoring module unloaded\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
