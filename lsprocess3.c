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
    int nr_total;   //��������
    int nr_running, nr_interruptible, nr_uninterruptible;//�������У����жϽ������������жϽ�����
    int nr_stopped; //����ִֹ�еĽ�����
    int nr_traced,nr_zombie,nr_dead;    //�������ӵĽ���������ʬ������������������
    int nr_unknown;//δ֪������

    struct task_struct *p = &init_task;
    char name[TASK_COMM_LEN],pname[TASK_COMM_LEN];
        
    //����ֵ
    nr_total = 0;
    nr_running = nr_interruptible = nr_uninterruptible = 0;
    nr_stopped = nr_unknown = 0;
    nr_traced = nr_zombie = nr_dead = 0;
    printk("********************************************************************\n");

    for_each_process(p) //ѭ���������н���
    {
        nr_total++; //�ܽ�����+1
        get_task_comm(name, p); //��ȡ��ǰ���̵�����
        if (p->parent)          //�����ǰ�����и����̣���ô��ȡ�����̵����֣���Ȼ��None
            get_task_comm(pname, p->parent);
        else
            strcpy(pname, "None");
        //��ϵͳ��־���������ǰ���̵����֣�ID��״̬���Լ������̵�����
        printk("name:%-20spid:%d state:%ld parent:%-20s\n", name, p->pid, p->state, pname);

        switch (p->exit_state)//�˳�״̬ͳ��
        {
        case EXIT_ZOMBIE:   //�˳�״̬�ǽ���״̬
            nr_zombie++;    //��������+1
            break;
        case TASK_DEAD:     //�˳�״̬������״̬ 
            nr_dead++;      //����״̬+1
            break;
        default:
            break;
        }
        if (p->exit_state)  //�ж��˳�״̬������Ϊ0����������
            continue;       //ֱ����ת����һ������

        switch (p->state)   //����״̬ͳ��
        {
        case TASK_RUNNING:  //�������л��ھ�������run_queue��׼�����еĽ��̣�ʵ�ʲ�����̵���
            nr_running++;
            break;
        case TASK_INTERRUPTIBLE:    //���ڵȴ������еĽ��̣�����Դ��Чʱ���ѣ�Ҳ������������ͨ���ź�(singal)��ʱ�жϻ��Ѻ�����������
            nr_interruptible++;
            break;
        case TASK_UNINTERRUPTIBLE://���ڵȴ������еĽ��̣�����Դ��Чʱ���ѣ���������������ͨ���źţ�signal����ʱ�жϻ���
            nr_uninterruptible++;
            break;
        case TASK_STOPPED:  //���̱���ͣ��ͨ���������̵��źŲ��ܻ���
            nr_stopped++;
            break;
        case TASK_TRACED:   //���̵�ִ�б�debugger������ͣ
            nr_traced++;
            break;   
        default:    //������δ֪������+1
            nr_unknown++;
            break;
        }              
    }

    //���ͳ����Ϣ
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
