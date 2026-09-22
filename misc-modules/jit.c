/*
 * Minimal modern jit.c for Assignment 7
 * Compatible with Linux 6.8 and 6.17 kernels
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>

MODULE_LICENSE("Dual BSD/GPL");

/*
 * Simple jiffies display
 */
static int jit_fn_show(struct seq_file *m, void *v)
{
    seq_printf(m, "jiffies: %lu\n", jiffies);
    return 0;
}

static int jit_fn_open(struct inode *inode, struct file *file)
{
    return single_open(file, jit_fn_show, NULL);
}

static const struct proc_ops jit_fn_pops = {
    .proc_open    = jit_fn_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * current time
 */
static int jit_currentime_show(struct seq_file *m, void *v)
{
    struct timespec64 ts;
    ktime_get_real_ts64(&ts);
    seq_printf(m, "%lld.%09ld\n",
               (long long)ts.tv_sec,
               ts.tv_nsec);
    return 0;
}

static int jit_currentime_open(struct inode *inode, struct file *file)
{
    return single_open(file, jit_currentime_show, NULL);
}

static const struct proc_ops jit_currentime_pops = {
    .proc_open    = jit_currentime_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Module init/exit
 */
static int __init jit_init(void)
{
    proc_create("jit", 0, NULL, &jit_fn_pops);
    proc_create("currentime", 0, NULL, &jit_currentime_pops);
    return 0;
}

static void __exit jit_cleanup(void)
{
    remove_proc_entry("jit", NULL);
    remove_proc_entry("currentime", NULL);
}

module_init(jit_init);
module_exit(jit_cleanup);
