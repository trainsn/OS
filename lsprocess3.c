#include<linux/sched.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/kthread.h>
#include<linux/jiffies.h>
#include<linux/err.h>

MODULE_LICENSE("GPL");

static struct task_struct *tesk;

static void show_process(void)
{
    int nr_total;   //进程总数
    int nr_running, nr_interruptible, nr_uninterruptible;//进程运行，可中断进程数，不可中断进程数
    int nr_stopped; //已终止执行的进程数
    int nr_traced,nr_zombie,nr_dead;    //正被监视的进程数，僵尸进程数，死亡进程数
    int nr_unknown;//未知进程数

    struct task_struct *p = &init_task;
    char name[TASK_COMM_LEN],pname[TASK_COMM_LEN];
        
    //赋初值
    nr_total = 0;
    nr_running = nr_interruptible = nr_uninterruptible = 0;
    nr_stopped = nr_unknown = 0;
    nr_traced = nr_zombie = nr_dead = 0;
    printk("********************************************************************\n");

    for_each_process(p) //循环遍历所有进程
    {
        nr_total++; //总进程数+1
        get_task_comm(name, p); //获取当前进程的名字
        if (p->parent)          //如果当前进程有父进程，那么获取父进程的名字，不然赋None
            get_task_comm(pname, p->parent);
        else
            strcpy(pname, "None");
        //在系统日志当中输出当前进程的名字，ID，状态，以及父进程的名字
        printk("name:%-20spid:%d state:%ld parent:%-20s\n", name, p->pid, p->state, pname);

        switch (p->exit_state)//退出状态统计
        {
        case EXIT_ZOMBIE:   //退出状态是僵死状态
            nr_zombie++;    //僵死进程+1
            break;
        case TASK_DEAD:     //退出状态是死亡状态 
            nr_dead++;      //死亡状态+1
            break;
        default:
            break;
        }
        if (p->exit_state)  //判断退出状态，若不为0，则不在运行
            continue;       //直接跳转到下一个进程

        switch (p->state)   //运行状态统计
        {
        case TASK_RUNNING:  //正在运行或在就绪队列run_queue中准备运行的进程，实际参与进程调度
            nr_running++;
            break;
        case TASK_INTERRUPTIBLE:    //处于等待队列中的进程，待资源有效时唤醒，也可由其它进程通过信号(singal)或定时中断唤醒后进入就绪队列
            nr_interruptible++;
            break;
        case TASK_UNINTERRUPTIBLE://处于等待队列中的进程，待资源有效时唤醒，不可由其它进程通过信号（signal）或定时中断唤醒
            nr_uninterruptible++;
            break;
        case TASK_STOPPED:  //进程被暂停，通过其它进程的信号才能唤醒
            nr_stopped++;
            break;
        case TASK_TRACED:   //进程的执行被debugger程序暂停
            nr_traced++;
            break;   
        default:    //其它，未知进程数+1
            nr_unknown++;
            break;
        }              
    }

    //输出统计信息
    printk("total tasks:              %4d\n", nr_total);
    printk("TASK_RUNNING:             %4d\n", nr_running);
    printk("TASK_INTERRUPTIBLE:       %4d\n", nr_interruptible);
    printk("TASK_UNINTERRUPTIBLE:     %4d\n", nr_uninterruptible);
    printk("TASK_STOPPED:             %4d\n", nr_stopped);
    printk("TASK_TRACED:              %4d\n", nr_traced);
    printk("TASK_ZOBIME:              %4d\n", nr_zombie);
    printk("TASK_DEAD:                %4d\n", nr_dead);
    printk("unknown state:            %4d\n", nr_unknown);
    printk("********************************************************************\n");

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
