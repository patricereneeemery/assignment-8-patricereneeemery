/*
 * Minimal SCULL pipe for Assignment 7
 * Modern kernel compatible (Linux 6.8 / 6.17)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "scull.h"

MODULE_LICENSE("Dual BSD/GPL");

#define PIPE_SIZE 4096

struct scull_pipe {
    char *buffer;
    size_t rp;   /* read pointer */
    size_t wp;   /* write pointer */
    struct cdev cdev;
};

static struct scull_pipe *pipe_dev;

/*
 * Read
 */
static ssize_t scull_pipe_read(struct file *filp, char __user *buf,
                               size_t count, loff_t *f_pos)
{
    struct scull_pipe *dev = filp->private_data;
    size_t available;

    if (dev->rp == dev->wp)
        return 0; /* empty */

    available = dev->wp - dev->rp;
    if (count > available)
        count = available;

    if (copy_to_user(buf, dev->buffer + dev->rp, count))
        return -EFAULT;

    dev->rp += count;
    return count;
}

/*
 * Write
 */
static ssize_t scull_pipe_write(struct file *filp, const char __user *buf,
                                size_t count, loff_t *f_pos)
{
    struct scull_pipe *dev = filp->private_data;
    size_t space = PIPE_SIZE - dev->wp;

    if (count > space)
        count = space;

    if (copy_from_user(dev->buffer + dev->wp, buf, count))
        return -EFAULT;

    dev->wp += count;
    return count;
}

/*
 * Open
 */
static int scull_pipe_open(struct inode *inode, struct file *filp)
{
    filp->private_data = pipe_dev;
    return 0;
}

/*
 * Release
 */
static int scull_pipe_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations scull_pipe_fops = {
    .owner   = THIS_MODULE,
    .open    = scull_pipe_open,
    .release = scull_pipe_release,
    .read    = scull_pipe_read,
    .write   = scull_pipe_write,
};

/*
 * /proc entry
 */
static int scull_pipe_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "SCULL pipe: %zu bytes used\n",
               pipe_dev->wp - pipe_dev->rp);
    return 0;
}

static int scull_pipe_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, scull_pipe_proc_show, NULL);
}

static const struct proc_ops scull_pipe_proc_ops = {
    .proc_open    = scull_pipe_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Init
 */
static int __init scull_pipe_init(void)
{
    dev_t dev;
    int result;

    result = alloc_chrdev_region(&dev, 0, 1, "scullpipe");
    if (result < 0)
        return result;

    pipe_dev = kzalloc(sizeof(struct scull_pipe), GFP_KERNEL);
    if (!pipe_dev)
        return -ENOMEM;

    pipe_dev->buffer = kzalloc(PIPE_SIZE, GFP_KERNEL);
    if (!pipe_dev->buffer)
        return -ENOMEM;

    cdev_init(&pipe_dev->cdev, &scull_pipe_fops);
    pipe_dev->cdev.owner = THIS_MODULE;
    cdev_add(&pipe_dev->cdev, dev, 1);

    proc_create("scullpipe", 0, NULL, &scull_pipe_proc_ops);

    return 0;
}

/*
 * Cleanup
 */
static void __exit scull_pipe_cleanup(void)
{
    remove_proc_entry("scullpipe", NULL);
    cdev_del(&pipe_dev->cdev);
    kfree(pipe_dev->buffer);
    kfree(pipe_dev);
}

module_init(scull_pipe_init);
module_exit(scull_pipe_cleanup);
