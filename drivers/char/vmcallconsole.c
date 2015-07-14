/*
 * vmcall console.
 */

#include <linux/module.h>
#include <linux/init.h>

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/console.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/compat.h>

static void vmcall_console_write (struct console *co, const char *s, unsigned n);

static ssize_t vmcc_write(struct file * file, const char __user * buf,
		        size_t count, loff_t *ppos)
{
	char kbuf[32];
	size_t copy_size = count;
	if (copy_size > sizeof(kbuf))
		copy_size = sizeof(kbuf);

	if (copy_from_user (kbuf, buf, copy_size)) {
		return -EFAULT;
	}
	vmcall_console_write((void *)0, kbuf, copy_size);
 	return copy_size;
}

/* there is nothing to go wrong here. */
static int vmcc_open(struct inode * inode, struct file * file)
{
	return 0;
}

static int vmcc_release(struct inode * inode, struct file * file)
{
	return 0;
}

static const struct file_operations vmcc_fops = {
	.owner		= THIS_MODULE,
	.write		= vmcc_write,
	.open		= vmcc_open,
	.release	= vmcc_release,
	//.read		= vmcc_read,
	.llseek		= noop_llseek,
};

static void vmcall_console_write (struct console *co, const char *s, unsigned n)
{
	unsigned int c;
	while ((c = *s++) != '\0' && n-- > 0) {
		__asm__  __volatile__("movl %0, %%edi\nvmcall\n" :  : "m"(c));
	}
}

static struct console vmcall_cons = {
	.name		= "vmcall",
	.write		= vmcall_console_write,
	.flags		= CON_PRINTBUFFER,
};

static int vmcc_register(void)
{
	printk("--------------------------> vmcc_register\n");
	if (register_chrdev (LP_MAJOR, "vmcc", &vmcc_fops)) {
		printk (KERN_ERR ">>>>>>>>>>>>>>>>>>>>>>>>>>>vmcc: unable to get major %d\n", LP_MAJOR);
		return -EIO;
	}
	register_console(&vmcall_cons);
	printk("->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Registered the vmcall console\n");
	return 0;
}

module_init(vmcc_register);

MODULE_LICENSE("GPL");
