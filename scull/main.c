/*
 * Minimal modern SCULL driver for Assignment 7
 * Compatible with Linux 6.8 (local) and 6.17 (Azure)
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

static int scull_major = SCULL_MAJOR;
static int scull_minor = 0;
static int scull_nr_devs = SCULL_NR_DEVS;

module_param(scull_major, int, 0);
module_param(scull_minor, int, 0);
module_param(scull_nr_devs, int, 0);

struct scull_dev *scull_devices;

/*
 * Trim device
 */
static int scull_trim(struct scull_dev *dev)
{
    kfree(dev->data);
    dev->data = NULL;
    dev->size = 0;
    return 0;
}


/*
 * Open
 */
static int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev;

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        scull_trim(dev);
    }
    return 0;
}

/*
 * Release
 */
static int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*
 * Read
 */
static ssize_t scull_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    ssize_t retval = 0;

    if (*f_pos >= dev->size)
        return 0;

    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    if (copy_to_user(buf, dev->data, count))
        return -EFAULT;

    *f_pos += count;
    retval = count;
    return retval;
}

/*
 * Write
 */
static ssize_t scull_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;

    kfree(dev->data);
    dev->data = kmalloc(count, GFP_KERNEL);
    if (!dev->data)
        return -ENOMEM;

    if (copy_from_user(dev->data, buf, count))
        return -EFAULT;

    dev->size = count;
    *f_pos += count;
    return count;
}

/*
 * File operations
 */
static const struct file_operations scull_fops = {
    .owner   = THIS_MODULE,
    .open    = scull_open,
    .release = scull_release,
    .read    = scull_read,
    .write   = scull_write,
};

/*
 * /proc entry
 */
static int scull_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "SCULL devices: %d\n", scull_nr_devs);
    return 0;
}

static int scull_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, scull_proc_show, NULL);
}

static const struct proc_ops scull_proc_ops = {
    .proc_open    = scull_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Init
 */
static int __init scull_init(void)
{
    int result, i;
    dev_t dev = 0;

    if (scull_major) {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
    }

    if (result < 0)
        return result;

    scull_devices = kzalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devices)
        return -ENOMEM;

    for (i = 0; i < scull_nr_devs; i++) {
        cdev_init(&scull_devices[i].cdev, &scull_fops);
        scull_devices[i].cdev.owner = THIS_MODULE;
        cdev_add(&scull_devices[i].cdev, MKDEV(scull_major, scull_minor + i), 1);
    }

    proc_create("scull", 0, NULL, &scull_proc_ops);

    return 0;
}

/*
 * Cleanup
 */
static void __exit scull_cleanup(void)
{
    int i;

    remove_proc_entry("scull", NULL);

    for (i = 0; i < scull_nr_devs; i++) {
        cdev_del(&scull_devices[i].cdev);
        scull_trim(&scull_devices[i]);
    }

    unregister_chrdev_region(MKDEV(scull_major, scull_minor), scull_nr_devs);
    kfree(scull_devices);
}

module_init(scull_init);
module_exit(scull_cleanup);
