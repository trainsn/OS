#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");

static struct task_struct *tesk;

int pte_callback(pte_t *pte, unsigned long addr, unsigned long next, struct mm_walk *walk)
{
	if (pte_dirty(*pte))
	{
		(*(unsigned long *)(walk->private))++;
		printk("Dirty PTE %lu\n", addr);
	}
	return 0;
}


static int show_process(void)
{
	struct task_struct *task;
	struct mm_struct *mm;
	unsigned long total_dirty = 0;
	unsigned long vma_dirty;
	struct mm_walk walk;
	int count=0;

	walk.pte_entry = pte_callback;
	walk.pmd_entry = NULL;
	walk.pte_hole = NULL;
	walk.hugetlb_entry = NULL;
	walk.test_walk = NULL;

	printk("**********************\n");
	//printk("pf of the system:%lu\n",pfcount);
	//printk("pf of current process:%lu\n",current->pf);    

	for (task = &init_task; (task = next_task(task)) != &init_task; )
	{
		count++;
		printk("process count = %d\n", count);
		total_dirty = 0;
		mm = task->mm;
		if (mm==NULL)
		{
			printk("Current process has no mmap\n");
			continue;
		}
		walk.mm=mm;
		for (walk.vma = mm->mmap; walk.vma != NULL; walk.vma = walk.vma->vm_next)
		{
			vma_dirty = 0;
			walk.private = &vma_dirty;
			walk_page_vma(walk.vma, &walk);
			//printk("VMA [%lx:%lx] has %lu dirty pages\n", walk.vma->vm_start, walk);
			total_dirty += vma_dirty;
		}
		printk("dirty page count = %lu\n", total_dirty);
	}
	return 0;
}

static int lsprocess(void *data){
    set_user_nice(current, 19); //设置当前进程的NICE值，降低当前进程的优先级
    set_current_state(TASK_INTERRUPTIBLE); //使当前进程睡眠，且可被唤醒
    while (!kthread_should_stop()){
        __set_current_state(TASK_RUNNING);//将当前进程设为运行状态
        printk(KERN_INFO "let's begin now!\n");
        show_process();
        set_current_state(TASK_INTERRUPTIBLE); //使当前进程睡眠，且可被唤醒
        schedule_timeout(60 * HZ); //进入延时唤醒状态(60s)
    }
    return 0;
}

static int lsprocess_init(void){
    tesk = kthread_run(lsprocess, NULL, "lsprocess");
    return 0;
}

static void __exit lsprocess_exit(void)
{
    printk(KERN_INFO "end!\n");
}

module_init(lsprocess_init);
module_exit(lsprocess_exit);

sudo mkinitramfs  4.6.0  -o  /boot/initrd.img-4.6.0