#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/sched.h>        /* Needed for current pointer */
#include <linux/fs.h>	        /* Needed for file_operation, inode, etc */
#include <linux/ioctl.h>		/* Needed ... duh ! */
#include <linux/delay.h>		/* Needed for msleep */
#include "ktest.h"

MODULE_AUTHOR("zeta");
MODULE_LICENSE("GPL");

static char* owner = "root"; 

module_param(owner, charp, S_IRUGO);

/* 
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */
long felk_ioctl(
		 struct file *file,			/* ditto */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)
{
	/* 
	 * Switch according to the ioctl called 
	 */
	switch (ioctl_num) {
	case IOCTL_FELK_HELLO:
		printk(KERN_DEBUG "Hello my dear owner %s !\n", owner);
		printk(KERN_DEBUG "Process ID=%d, cmd=%s\n", current->pid, current->comm);
		break;
	}
	
	return 0;
}

/* 
 * This structure will hold the functions to be called
 * when a process does something to the device we
 * created. Since a pointer to this structure is kept in
 * the devices table, it can't be local to
 * init_module. NULL is for unimplemented functions. 
 */
struct file_operations felk_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = felk_ioctl,
};

static int __init felk_start(void)
{
	int ret_val;

	printk(KERN_DEBUG "Loading %s for %s...\n", DEVICE_NAME, owner);
	printk(KERN_DEBUG "Process ID=%d, cmd=%s\n", current->pid, current->comm);

	/* 
	 * Register the character device (atleast try) 
	 */
	ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &felk_fops);

	/* 
	 * Negative values signify an error 
	 */
	if (ret_val < 0) {
		printk(KERN_ALERT "Can't register %s device.\n", DEVICE_NAME);
		return ret_val;
	}

	printk(KERN_INFO "Device felk registered with major number %d.\n", MAJOR_NUM);
	printk(KERN_INFO "Create device node like this: mknod %s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);

	return 0;
}

static void __exit felk_end(void)
{
	printk(KERN_DEBUG "Unloading felk ...\n");

	/* 
	 * Unregister the device 
	 */
	unregister_chrdev(MAJOR_NUM, DEVICE_NAME);

}

module_init(felk_start);
module_exit(felk_end);
