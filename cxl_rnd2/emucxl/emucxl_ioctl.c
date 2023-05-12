#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>


/* START: For NUMA NODE*/
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/types.h>

#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/io.h>
#include <asm-generic/errno.h>
#include <asm-generic/errno-base.h>
/* END: For NUMA NODE*/

#include "emucxl_ioctl.h"

#define FIRST_MINOR 0
#define MINOR_CNT 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
// static int status = 1, dignity = 3, ego = 5;
static int* my_array;

static int my_open(struct inode *i, struct file *f)
{
	return 0;
}
static int my_close(struct inode *i, struct file *f)
{
	return 0;
}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int my_ioctl(struct inode *i, struct file *f, unsigned int cmd, unsigned long arg)
#else
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
	emucxl_arg_t _arguments ;
	int _size = 0;
	int _numa_node = 0;
	void* _address = NULL;
    phys_addr_t phys_addr;
    pid_t pid;
	int ret;
	unsigned long pfn;
	struct vm_area_struct *vma;
	emucxl_memory_t _memory;

	// query_arg_t q;
	switch (cmd)
	{
		case EMUCXL_ALLOCATE_MEMORY:
			if (copy_from_user(&_arguments, (emucxl_arg_t *)arg, sizeof(emucxl_arg_t)))
			{
				return -EACCES;
			}
			printk(KERN_INFO "EMUCXL_ALLOCATE_MEMORY: size = %d, numa_node = %d\n", _arguments.size, _arguments.numa_node);
			_address = kmalloc_node(_arguments.size, GFP_USER | __GFP_ZERO, _arguments.numa_node);
			my_array = (int*)_address;
			pid = task_pid_nr(current);
    		phys_addr = virt_to_phys((void *)my_array);
			if (!phys_addr) {
				pr_alert("Error: Virtual Address 0x%p of PID: %d does not exist... \n", (void*)my_array, pid);
				return -1;
			}
			pr_info("PID: %d \t Virtual Address: 0x%px \t Physical Address: 0x%llx \n", pid, (void*)my_array, phys_addr);

			// // Map the memory into the virtual address space of the user process
			// _address = mmap(NULL, _arguments.size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE, -1, 0);
			// if (_address == MAP_FAILED) {
			// 	kfree(_address);
			// 	return -ENOMEM;
			// }
			
    		pfn = virt_to_phys(_address) >> PAGE_SHIFT;
			vma = find_vma(current->mm, (unsigned long)_address);
			pr_info("PID: %d \t Virtual Address: 0x%px \t Physical Address: 0x%llx \n", pid, (void*)_address, phys_addr);

			printk(KERN_INFO "Reached here\n");
			_arguments.address = _address;
			_arguments.ret = ret;
			if (copy_to_user((emucxl_arg_t *)arg, &_arguments, sizeof(emucxl_arg_t)))
			{
				return -EACCES;
			}
			pr_info("Virtual Address: 0x%px \n", (void*)_address);
			
			break;
		case EMUCXL_FREE_MEMORY:
			if (copy_from_user(&_arguments, (emucxl_arg_t *)arg, sizeof(emucxl_arg_t)))
			{
				return -EACCES;
			}
			if(!_arguments.address)
			{
				return -EINVAL;
			}
			else
			{
				kfree(_arguments.address);
			}
			break;

		case EMUCXL_LOAD :
			if(copy_from_user(&_memory, (emucxl_memory_t *)arg, sizeof(emucxl_memory_t)))
			{
				return -EACCES;
			}

			// get the value of the address
			_memory.value = _memory.address[_memory.index];
			if (copy_to_user((emucxl_memory_t *)arg, &_memory, sizeof(emucxl_memory_t)))
			{
				return -EACCES;
			}
			break;
		case EMUCXL_STORE :
			if(copy_from_user(&_memory, (emucxl_memory_t *)arg, sizeof(emucxl_memory_t)))
			{
				return -EACCES;
			}

			// set the value of the address
			_memory.address[_memory.index] = _memory.value;
			if (copy_to_user((emucxl_memory_t *)arg, &_memory, sizeof(emucxl_memory_t)))
			{
				return -EACCES;
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static struct file_operations query_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = my_ioctl
#else
	.unlocked_ioctl = my_ioctl
#endif
};

static int __init emucxl_ioctl_init(void)
{
	int ret;
	struct device *dev_ret;


	if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "emucxl_ioctl")) < 0)
	{
		return ret;
	}

	cdev_init(&c_dev, &query_fops);

	if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
	{
		return ret;
	}
	
	if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(cl);
	}
	if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "emucxl")))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

	return 0;
}

static void __exit emucxl_ioctl_exit(void)
{
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(emucxl_ioctl_init);
module_exit(emucxl_ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raja Gond <rajagond@cse.iitb.ac.in>");
MODULE_DESCRIPTION("Basic Emucxl Library");
