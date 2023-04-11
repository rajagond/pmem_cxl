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


MODULE_AUTHOR("Raja");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("NUMA-aware Kernel Module");

#define ARRAY_SZ 10
#define NUMA_NODE 0  // Specify the NUMA node here
#define MODULE_NAME "mem_access"
#define METHOD 2

static int* my_array;

static int __init my_module_init(void)
{
    // Initialize the array with values
    int i;
    phys_addr_t phys_addr;
    pid_t pid;
    printk(KERN_INFO "Module %s inserted with NODE = %d...\n", MODULE_NAME, NUMA_NODE);

    /*
    ** Allocate memory on the specific NUMA node
    * vmalloc() is not NUMA-aware
    * vmalloc_node() is NUMA-aware
    * vzalloc_node() is NUMA-aware and recommended
    * kmalloc_node() and kzalloc_node() is NUMA-aware
    * GFP : Get Free Pages
    * Using GFP_KERNEL means that kmalloc can put the current process to sleep waiting for a page when called in low-memory situations.
    * Memory Zeroing: The memory returned by kzalloc is guaranteed to be initialized with zeros, which can be useful in situations where 
    * it is important to have initialized memory, such as when dealing with sensitive data or when allocating memory for data structures 
    * that require zero-initialized memory.
    * vmalloc vs kmalloc
    * 
    * Memory size:  vmalloc_node can allocate larger memory blocks compared to kmalloc_node. vmalloc_node can allocate memory blocks that
    *  are larger than the size of a single page
    * 
    * The kmalloc() & vmalloc() functions are a simple interface for obtaining kernel memory in byte-sized chunks.
    * The kmalloc() function guarantees that the pages are physically contiguous (and virtually contiguous).
    * The vmalloc() function works in a similar fashion to kmalloc(), except it allocates memory that is only virtually
    *  contiguous and not necessarily physically contiguous.
    */ 
    // Choose method of allocation based on variable
    switch (METHOD)
    {
    case 1:
        /* code */
        my_array = (int*)vmalloc_node(ARRAY_SZ * sizeof(int), NUMA_NODE);
        break;
    case 2:
        /* code */
        my_array = (int*)kmalloc_node(ARRAY_SZ * sizeof(int), GFP_KERNEL, NUMA_NODE);
        break;
    case 3:
        /* code */
        my_array = (int*)kzalloc_node(ARRAY_SZ * sizeof(int), GFP_KERNEL, NUMA_NODE);
        break;
    default:
        my_array = (int*)vzalloc_node(ARRAY_SZ * sizeof(int), NUMA_NODE);
        break;
    }
    // resize
    // local - remote allocation
    // transfer data from local to remote
    if (!my_array) {
        printk(KERN_ERR "Failed to allocate memory on NUMA node %d\n", NUMA_NODE);
        return -ENOMEM;
    }

    // check v2p mapping
    pid = task_pid_nr(current);
    phys_addr = virt_to_phys((void *)my_array);
    if (!phys_addr) {
        pr_alert("Error: Virtual Address 0x%p of PID: %d does not exist... \n", (void*)my_array, pid);
        return -1;
    }
    pr_info("PID: %d \t Virtual Address: 0x%px \t Physical Address: 0x%lld \n", pid, (void*)my_array, phys_addr);

    for (i = 0; i < ARRAY_SZ; i++) {
        my_array[i] = i * 2;
    }

    //Print the array values
    printk(KERN_INFO "Array values:\n");
    for (i = 0; i < ARRAY_SZ; i++) {
        printk(KERN_INFO "my_array[%d] = %d and address is 0x%px\n", i, my_array[i], &my_array[i]);
    }

        // Print the array values
    // printk(KERN_INFO "Array values:\n");
    // for (i = 0; i < ARRAY_SZ; i++) {
    //     printk(KERN_INFO "my_array[%d] = %d and address is 0x%llx\n", i, my_array[i], (long long unsigned int)&my_array[i]);
    // }
    return 0;
}

static void __exit my_module_exit(void)
{
    pr_info("Exited from module %s\n", MODULE_NAME);

    // Free the allocated memory
    if (my_array && (METHOD == 3 || METHOD == 2)) {
        kfree(my_array);
    }
    else{
        if(my_array)
        {
            vfree(my_array);
        }
    }
}

module_init(my_module_init);
module_exit(my_module_exit);