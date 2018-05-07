/**
 * @file    usercounter.c
 * @author  Neil McGlohon
 * @date    6 May 2018
 * @version 0.1
 * @brief  A loadable kernel to enable userspace access to the cycle counter
*/
#include <linux/module.h>     /* Needed by all modules */
#include <linux/kernel.h>     /* Needed for KERN_INFO */
#include <linux/init.h>       /* Needed for the macros */
 
///< The license type -- this affects runtime behavior
MODULE_LICENSE("GPL");
 
///< The author -- visible when you use modinfo
MODULE_AUTHOR("Neil McGlohon");
 
///< The description -- see modinfo
MODULE_DESCRIPTION("Enables userspace access to cycle counter");
 
///< The version of the module
MODULE_VERSION("0.1");
 
static void enable_ccnt_read(void* data)
{
  // WRITE PMUSERENR = 1
  asm volatile ("MCR p15, 0, %0, C9, C14, 0\n\t" :: "r"(1));
  asm volatile ("MCR p15, 0, %0, C9, C14, 2\n\t" :: "r"(0x8000000f));
}

static void disable_ccnt_read(void* data)
{
  // WRITE PMUSERENR = 1
  asm volatile ("MCR p15, 0, %0, C9, C14, 0\n\t" :: "r"(0));
  asm volatile ("MCR p15, 0, %0, C9, C14, 2\n\t" :: "r"(0));
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
