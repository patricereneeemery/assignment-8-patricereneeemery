#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("patricereneeemery");
MODULE_DESCRIPTION("Minimal SCULL driver for Azure test harness");

static dev_t scull_dev;
static struct cdev scull_cdev;


static ssize_t scull_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    return 0;   // EOF
}

static ssize_t scull_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    return count;   // pretend we wrote everything
}


static struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .read = scull_read,
    .write = scull_write,
};


static int __init scull_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&scull_dev, 0, 1, "scull");
    if (ret < 0) {
        pr_err("scull: alloc_chrdev_region failed\n");
        return ret;
    }

    cdev_init(&scull_cdev, &scull_fops);
    cdev_add(&scull_cdev, scull_dev, 1);

    pr_info("scull: registered with major %d minor %d\n",
            MAJOR(scull_dev), MINOR(scull_dev));

    return 0;
}

static void __exit scull_exit(void)
{
    cdev_del(&scull_cdev);
    unregister_chrdev_region(scull_dev, 1);
    pr_info("scull: unregistered\n");
}




module_init(scull_init);
module_exit(scull_exit);
