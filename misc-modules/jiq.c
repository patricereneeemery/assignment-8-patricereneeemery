/*
 * jiq.c -- the just-in-queue module
 *
 * Updated for modern kernels (proc_ops + timer_shutdown)
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/interrupt.h>
#include <linux/timer.h>

MODULE_LICENSE("Dual BSD/GPL");

static long delay = 1;
module_param(delay, long, 0);

#define LIMIT   (PAGE_SIZE - 128)

static DECLARE_WAIT_QUEUE_HEAD(jiq_wait);

struct clientdata {
        struct work_struct jiq_work;
        struct delayed_work jiq_delayed_work;
        struct timer_list jiq_timer;
        struct tasklet_struct jiq_tasklet;
        struct seq_file *m;
        int len;
        unsigned long jiffies;
        long delay;
};

static struct clientdata jiq_data;

/*
 * Printing helper
 */
static int jiq_print(struct clientdata *data)
{
        int len = data->len;
        struct seq_file *m = data->m;
        unsigned long j = jiffies;

        if (len > LIMIT) {
                wake_up_interruptible(&jiq_wait);
                return 0;
        }

        if (len == 0) {
                seq_puts(m, "    time  delta preempt   pid cpu command\n");
                len = m->count;
        } else {
                len = 0;
        }

        seq_printf(m, "%9li  %4li     %3i %5i %3i %s\n",
                   j, j - data->jiffies,
                   preempt_count(), current->pid,
                   smp_processor_id(), current->comm);

        len += m->count;
        data->len += len;
        data->jiffies = j;

        return 1;
}

/*
 * Workqueue versions
 */
static void jiq_print_wq(struct work_struct *work)
{
        struct clientdata *data =
                container_of(work, struct clientdata, jiq_work);

        if (jiq_print(data))
                schedule_work(&jiq_data.jiq_work);
}

static void jiq_print_wq_delayed(struct work_struct *work)
{
        struct clientdata *data =
                container_of(work, struct clientdata, jiq_delayed_work.work);

        if (jiq_print(data))
                schedule_delayed_work(&jiq_data.jiq_delayed_work, data->delay);
}

static int jiq_read_wq_show(struct seq_file *m, void *v)
{
        DEFINE_WAIT(wait);

        jiq_data.len = 0;
        jiq_data.m = m;
        jiq_data.jiffies = jiffies;
        jiq_data.delay = 0;

        prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
        schedule_work(&jiq_data.jiq_work);
        schedule();
        finish_wait(&jiq_wait, &wait);

        return 0;
}

static int jiq_read_wq_open(struct inode *inode, struct file *file)
{
        return single_open(file, jiq_read_wq_show, NULL);
}

static const struct proc_ops jiq_read_wq_pops = {
        .proc_open      = jiq_read_wq_open,
        .proc_read      = seq_read,
        .proc_lseek     = seq_lseek,
        .proc_release   = single_release,
};

/*
 * Delayed workqueue
 */
static int jiq_read_wq_delayed_show(struct seq_file *m, void *v)
{
        DEFINE_WAIT(wait);

        jiq_data.len = 0;
        jiq_data.m = m;
        jiq_data.jiffies = jiffies;
        jiq_data.delay = delay;

        prepare_to_wait(&jiq_wait, &wait, TASK_INTERRUPTIBLE);
        schedule_delayed_work(&jiq_data.jiq_delayed_work, delay);
        schedule();
        finish_wait(&jiq_wait, &wait);

        return 0;
}

static int jiq_read_wq_delayed_open(struct inode *inode, struct file *file)
{
        return single_open(file, jiq_read_wq_delayed_show, NULL);
}

static const struct proc_ops jiq_read_wq_delayed_pops = {
        .proc_open      = jiq_read_wq_delayed_open,
        .proc_read      = seq_read,
        .proc_lseek     = seq_lseek,
        .proc_release   = single_release,
};

/*
 * Tasklet version
 */
static void jiq_print_tasklet(unsigned long ptr)
{
        struct clientdata *data = (struct clientdata *)ptr;

        if (jiq_print(data))
                tasklet_schedule(&data->jiq_tasklet);
}

static int jiq_read_tasklet_show(struct seq_file *m, void *v)
{
        jiq_data.len = 0;
        jiq_data.m = m;
        jiq_data.jiffies = jiffies;

        tasklet_schedule(&jiq_data.jiq_tasklet);
        wait_event_interruptible(jiq_wait, 0);

        return 0;
}

static int jiq_read_tasklet_open(struct inode *inode, struct file *file)
{
        return single_open(file, jiq_read_tasklet_show, NULL);
}

static const struct proc_ops jiq_read_tasklet_pops = {
        .proc_open      = jiq_read_tasklet_open,
        .proc_read      = seq_read,
        .proc_lseek     = seq_lseek,
        .proc_release   = single_release,
};

/*
 * Timer version
 */
static void jiq_timedout(struct timer_list *t)
{
        struct clientdata *data =
                container_of(t, struct clientdata, jiq_timer);

        jiq_print(data);
        wake_up_interruptible(&jiq_wait);
}

static int jiq_read_run_timer_show(struct seq_file *m, void *v)
{
        jiq_data.len = 0;
        jiq_data.m = m;
        jiq_data.jiffies = jiffies;

        timer_setup(&jiq_data.jiq_timer, jiq_timedout, 0);
        jiq_data.jiq_timer.expires = jiffies + HZ;

        jiq_print(&jiq_data);
        add_timer(&jiq_data.jiq_timer);

        wait_event_interruptible(jiq_wait, 0);

        timer_shutdown(&jiq_data.jiq_timer);

        return 0;
}

static int jiq_read_run_timer_open(struct inode *inode, struct file *file)
{
        return single_open(file, jiq_read_run_timer_show, NULL);
}

static const struct proc_ops jiq_read_run_timer_pops = {
        .proc_open      = jiq_read_run_timer_open,
        .proc_read      = seq_read,
        .proc_lseek     = seq_lseek,
        .proc_release   = single_release,
};

/*
 * Init / cleanup
 */
static int __init jiq_init(void)
{
        INIT_WORK(&jiq_data.jiq_work, jiq_print_wq);
        INIT_DELAYED_WORK(&jiq_data.jiq_delayed_work, jiq_print_wq_delayed);
        tasklet_init(&jiq_data.jiq_tasklet, jiq_print_tasklet,
                     (unsigned long)&jiq_data);

        proc_create("jiqwq", 0, NULL, &jiq_read_wq_pops);
        proc_create("jiqwqdelay", 0, NULL, &jiq_read_wq_delayed_pops);
        proc_create("jitimer", 0, NULL, &jiq_read_run_timer_pops);
        proc_create("jiqtasklet", 0, NULL, &jiq_read_tasklet_pops);

        return 0;
}

static void __exit jiq_cleanup(void)
{
        remove_proc_entry("jiqwq", NULL);
        remove_proc_entry("jiqwqdelay", NULL);
        remove_proc_entry("jitimer", NULL);
        remove_proc_entry("jiqtasklet", NULL);
}

module_init(jiq_init);
module_exit(jiq_cleanup);
