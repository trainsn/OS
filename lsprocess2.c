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
    struct task_struct *p = &init_task;
    char name[TASK_COMM_LEN],pname[TASK_COMM_LEN];
    unsigned long dirtyCount,testCount;
    int  i, j, k, l;
    struct page *pag;
    struct mm_struct *mm; 
        
    printk("********************************************************************\n");

    for_each_process(p) //ѭ���������н���
    {
         get_task_comm(name, p); //��ȡ��ǰ���̵�����
        if (p->parent)          //�����ǰ�����и����̣���ô��ȡ�����̵����֣���Ȼ��None
            get_task_comm(pname, p->parent);
        else
            strcpy(pname, "None");
        //��ϵͳ��־���������ǰ���̵����֣�ID��״̬���Լ������̵�����
        printk("name:%-20spid:%d state:%ld parent:%-20s\n", name, p->pid, p->state, pname);

    
        dirtyCount = 0;
        testCount=0;
        
        mm = p->mm;
        //printk("%d\n", PTRS_PER_PGD);
        for (i = 0; i < PTRS_PER_PGD; ++i)
        {
            pgd_t *pgd = mm->pgd + i;
            if (pgd_none(*pgd) || pgd_bad(*pgd))
                continue;
            for (k = 0; k < PTRS_PER_PMD; ++k)
            {
                pmd_t *pmd = (pmd_t *)pgd_page_vaddr(*pgd) + k;
                if (pmd_none(*pmd) || pmd_bad(*pmd))
                     continue;
                for (l = 0; l < PTRS_PER_PTE; ++l)
                {
                    pte_t *pte = (pte_t *)pmd_page_vaddr(*pmd) + l;
                    if (!pte || pte_none(*pte))
                        continue;
                    pag = pte_page(*pte);
                        //if (test_bit(PG_dirty, p->flags))
                    testCount++;
                    if (PageDirty(pag))
                        dirtyCount++;
                }
            }
        }
        printk("nr_dirtied:%d\n",p->nr_dirtied);
        printk("dirty pages of current process:%lu\n",testCount);
    }
    return;
}

static int lsprocess(void *data){
    set_user_nice(current, 19); //���õ�ǰ���̵�NICEֵ�����͵�ǰ���̵����ȼ�
    set_current_state(TASK_INTERRUPTIBLE); //ʹ��ǰ����˯�ߣ��ҿɱ�����
    while (!kthread_should_stop()){
        __set_current_state(TASK_RUNNING);//����ǰ������Ϊ����״̬
        printk(KERN_INFO "let's begin now!\n");
        show_process();
        set_current_state(TASK_INTERRUPTIBLE); //ʹ��ǰ����˯�ߣ��ҿɱ�����
        schedule_timeout(60 * HZ); //������ʱ����״̬(60s)
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