
#ifndef H_TASKMONITOR
#define H_TASKMONITOR

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

/* -------- Prototypes ------------------------------------------------------ */
unsigned long taskmon_count(struct shrinker* s, struct shrink_control *sc);
unsigned long taskmon_scan(struct shrinker* s, struct shrink_control *sc);
static ssize_t tm_attr_show (struct kobject *kobj,
	struct kobj_attribute *attr, char *buf);
static ssize_t tm_attr_store (struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count);

bool get_sample(struct task_monitor *tm, struct task_sample *sample);
void print_sample(struct task_monitor *tm);

int monitor_fn(void *data);
int monitor_pid(pid_t pid);
static int monitor_init(void);
static void monitor_exit(void);


#endif
