#include <linux/sched.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/linkage.h>
#include <linux/page-flags.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/pagevec.h>


MODULE_LICENSE("GPL");

static struct task_struct *tesk;

static void show_process(void)
{
    struct task_struct *task = &init_task;        
    printk("********************************************************************\n");

    pgd_t *pgd;
    pud_t *pud;
    pte_t* pte;
    pmd_t* pmd;
    struct page* page;
    struct mm_struct* mm;
    struct vm_area_struct *vma = 0;
    unsigned long vpage,virt;
    int AddressCount=0;
    for_each_process(task) //循环遍历所有进程
    {
        if (task->mm && task->mm->mmap)
            for (vma = task->mm->mmap; vma; vma = vma->vm_next)
                for (vpage = vma->vm_start; vpage < vma->vm_end; vpage += PAGE_SIZE)
                {
                    mm = task->mm;
                    virt = vpage;
                    pgd = pgd_offset(mm, virt);
                    if (pgd_none(*pgd) || pgd_bad(*pgd))
                        continue;
                    pud = pud_offset(pgd, virt);
                    if (pud_none(*pud) || pud_bad(*pud))
                        continue;
                    pmd = pmd_offset(pud, virt);
                    if (pmd_none(*pmd) || pmd_bad(*pmd))
                        continue;
                    if (!(pte = pte_offset_map(pmd, virt)))
                        continue;
                    if (!(page = pte_page(*pte)))
                        continue;
                    AddressCount++;
                }                   
    }
    printk("Addresscount=%d\n",AddressCount);
    return;
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