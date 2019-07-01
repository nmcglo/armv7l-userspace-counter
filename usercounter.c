/**
 * @file    usercounter.c
 * @author  Neil McGlohon
 * @date    1 July 2019
 * @version 0.2
 * @brief  A loadable kernel to enable userspace access to the cycle counter
*/
#include <linux/module.h>     /* Needed by all modules */
#include <linux/kernel.h>     /* Needed for KERN_INFO */
#include <linux/init.h>       /* Needed for the macros */
 
#define PERF_DEF_OPTS (1 | 16)

///< The license type -- this affects runtime behavior
MODULE_LICENSE("GPL");
 
///< The author -- visible when you use modinfo
MODULE_AUTHOR("Neil McGlohon");
 
///< The description -- see modinfo
MODULE_DESCRIPTION("Enables userspace access to cycle counter");
 
///< The version of the module
MODULE_VERSION("0.2");
 
static void enable_ccnt_read(void* data)
{
    asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(1));
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(PERF_DEF_OPTS));
    asm volatile("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
}

static void disable_ccnt_read(void* data)
{
    asm volatile("mcr p15, 0, %0, c9, c12, 0" :: "r"(0));
    asm volatile("mcr p15, 0, %0, c9, c12, 2" :: "r"(0x8000000f));
    asm volatile("mcr p15, 0, %0, c9, c14, 0" :: "r"(0));
}

static int __init usercounter_start(void)
{
    printk(KERN_INFO "Loading usercounter module...\n");

    on_each_cpu(enable_ccnt_read, NULL, 1);

    printk(KERN_INFO "user counters enabled\n");
    return 0;
}
 
static void __exit usercounter_end(void)
{
    printk(KERN_INFO "Unloading usercounter module...\n");
    on_each_cpu(disable_ccnt_read, NULL, 1);
    printk(KERN_INFO "user counters disbled\n");

}
 
module_init(usercounter_start);
module_exit(usercounter_end);
