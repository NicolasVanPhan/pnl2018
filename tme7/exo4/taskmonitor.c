#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/shrinker.h>
#include <linux/mempool.h>

#include "taskmonitor.h"

MODULE_DESCRIPTION("A process monitor");
MODULE_AUTHOR("Maxime Lorrillere <maxime.lorrillere@lip6.fr>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");

unsigned short target = 1; /* default pid to monitor */
unsigned frequency = 1; /* sampling frequency */

module_param(target, ushort, 0400);
module_param(frequency, uint, 0600);

struct task_struct *monitor_thread;

struct task_monitor *tm;

/* -------------------------------------------------------------------------- */
/* ---------- TME7 Exercice 4 : Mempool ------------------------------------- */
/* -------------------------------------------------------------------------- */

mempool_t	*ts_mempool;

/* -------------------------------------------------------------------------- */
/* ---------- TME7 Exercice 3 : Slab ---------------------------------------- */
/* -------------------------------------------------------------------------- */

struct kmem_cache	*task_sample_pool;

/* -------------------------------------------------------------------------- */
/* ---------- TME7 Exercice 2 : Shrinker ------------------------------------ */
/* -------------------------------------------------------------------------- */

struct shrinker taskmon_shrinker = {
	.count_objects = taskmon_count,
	.scan_objects = taskmon_scan,
	.seeks = DEFAULT_SEEKS,
	.batch = 0,
	.flags = 0};

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
		//kfree(pts);		
		//kmem_cache_free(task_sample_pool, pts);	 /* TME7 EXO3 */
		mempool_free(pts, ts_mempool);			 /* TME7 EXO4 */
		tm->nsamples--;
		freed++;
	}
	mutex_unlock(&tm->lock);
	return freed;
}

/* -------------------------------------------------------------------------- */
/* ---------- TME6 Exercice 5 : Adding a sysfs interface -------------------- */
/* -------------------------------------------------------------------------- */

static struct kobj_attribute tm_attr = __ATTR_RW(tm_attr);

static ssize_t tm_attr_show (struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct task_sample *pts;
	const int maxlen = 20;
	ssize_t	cnt;

	/* TME7 - Exercice 1 */
	cnt = 0;
	cnt += scnprintf(buf + cnt, maxlen, "Nb samples : %d\n", tm->nsamples);
	list_for_each_entry(pts, &tm->phantom.list, list) {
		if (cnt >= PAGE_SIZE - maxlen)
			break;
		cnt += scnprintf(buf + cnt, maxlen, "%d; usr %ld; sys %ld\n",
			target,	pts->utime, pts->stime);
	}
	return cnt;
}

static ssize_t tm_attr_store (struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
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

/* -------------------------------------------------------------------------- */
/* ---------- TME7 Exercice 1 : Saving samples ------------------------------ */
/* -------------------------------------------------------------------------- */

void save_sample(struct task_monitor *tm)
{
	struct task_sample *ts;
	bool alive;

	//ts = kmalloc(sizeof(struct task_sample), GFP_KERNEL);
	//ts = kmem_cache_alloc(task_sample_pool, GFP_KERNEL);   /* TME7 EXO3 */
	ts = mempool_alloc(ts_mempool, GFP_KERNEL);              /* TME7 EXO4 */
	alive = get_sample(tm, ts);

	if (alive) {
		mutex_lock(&tm->lock);
		list_add_tail(&ts->list, &tm->phantom.list);
		tm->nsamples++;
		mutex_unlock(&tm->lock);
	} else {
		//kfree(ts);
		/* TME7 - Exercice 3 */
		//kmem_cache_free(task_sample_pool, ts);
		/* TME7 - Exercice 4 */
		mempool_free(ts, ts_mempool);
	}
}

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

		//print_sample(tm);
		save_sample(tm);				 /* TME7 EXO1 */
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
	INIT_LIST_HEAD(&tm->phantom.list);			 /* TME7 EXO1 */
	mutex_init(&tm->lock);					 /* TME7 EXO1 */
	tm->nsamples = 0;					 /* TME7 EXO1 */

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

	/* TME6 - Exercice 5 */
	sysfs_create_file(kernel_kobj, &(tm_attr.attr));
	/* TME7 - Exercice 2 */
	register_shrinker(&taskmon_shrinker);
	/* TME7 - Exercice 3 */
	task_sample_pool = KMEM_CACHE(task_sample, 0);
	/* TME7 - Exercice 4 */
	ts_mempool = mempool_create_slab_pool(100, task_sample_pool);
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
		//kfree(pts);		
		/* TME7 - Exercice 3 */
		//kmem_cache_free(task_sample_pool, pts);
		/* TME7 - Exercice 4 */
		mempool_free(pts, ts_mempool);
	}
	mutex_unlock(&tm->lock);

	put_pid(tm->pid);
	kfree(tm);

	/* TME6 - Exercice 5 */
	sysfs_remove_file(kernel_kobj, &(tm_attr.attr));
	/* TME7 - Exercice 2 */
	unregister_shrinker(&taskmon_shrinker);
	/* TME7 - Exercice 3 */
	kmem_cache_destroy(task_sample_pool);
	/* TME7 - Exercice 4 */
	mempool_destroy(ts_mempool);
	pr_info("Monitoring module unloaded\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
