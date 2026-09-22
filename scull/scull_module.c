#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("patricereneeemery");
MODULE_DESCRIPTION("Minimal SCULL stub for Azure test harness");

static int __init scull_init(void)
{
    pr_info("scull: minimal stub loaded\n");
    return 0;
}

static void __exit scull_exit(void)
{
    pr_info("scull: minimal stub unloaded\n");
}

module_init(scull_init);
module_exit(scull_exit);
