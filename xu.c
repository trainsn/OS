#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define CAR_NUMBER 20 /*通过路口的车辆总数*/

void *east(void *arg); /*东车线程*/
void *south(void *arg); /*南车线程*/
void *west(void *arg); /*西车线程*/
void *north(void *arg); /*北车线程*/

pthread_t tid[CAR_NUMBER]; /*每辆车的线程号*/
pthread_attr_t attr[CAR_NUMBER]; /*对应tid的attr*/
int tidnumber[CAR_NUMBER]; /*每个线程被创建的序号*/

/*0,1,2,3分别代表东南西北四个方向*/
int count[4]; /*每个路口等待的车辆数*/
int arrivalnumber[4]; /*每辆车从arrivalnumber获取在waitqueue中的位置*/
bool waitqueue[4][CAR_NUMBER]; /*四个方向的等待队列，true代表当前车辆为队列第一车*/
bool goalong; /*某个方向的车离开后向其左侧的车发出的指示*/
int go; /*firstgo的状态，-1代表未指定，0,1,2,3分别代表四个方向出发优先*/
pthread_mutex_t mutex; /*互斥锁*/
pthread_cond_t firstleave[4]; /*某个方向队列里的第一辆车离开的条件变量*/
pthread_cond_t firstgo; /*出发条件变量*/

int main() {
	int i, err, j;
	char array[20];

	/*初始化*/
	go = -1; /*-1代表优先出发方向未指定*/
	pthread_cond_init(&firstgo, NULL);
	pthread_mutex_init(&mutex, NULL);
	goalong = false; /*初始*/
	for (i = 0; i<4; i++)
	{
		count[i] = 0; /*每个方向的初始车辆数为0*/
		arrivalnumber[i] = -1; /*初始值为-1，因为第一辆车在waitqueue中的下标将是0*/
		waitqueue[i][0] = true; /*每个方向第一辆到达的车直接成为队列第一车*/
		pthread_cond_init(&firstleave[i], NULL);
	}
	for (i = 0; i<CAR_NUMBER; i++)
	{
		tidnumber[i] = i + 1;
	}
	for (i = 1; i<CAR_NUMBER; i++)
	{	/*每个方向后面到达的车都需要等待*/
		waitqueue[0][i] = false;
		waitqueue[1][i] = false;
		waitqueue[2][i] = false;
		waitqueue[3][i] = false;
	}

	for (i = 0; i<CAR_NUMBER; i++)
	{
		pthread_attr_init(&attr[i]); /*初始化attr*/
	}
	
	scanf("%s",array);
	i=0;
	for(j=0;array[j]!='\0';j++)
	{
			
		switch (array[j]) /*随机确定i车的方向*/
		{
		/*创建i车线程*/
		case 'E':
			err = pthread_create(&tid[i], &attr[i], east, &tidnumber[i]);
			break;
		case 'S':
			err = pthread_create(&tid[i], &attr[i], south, &tidnumber[i]);
			break;
		case 'W':
			err = pthread_create(&tid[i], &attr[i], west, &tidnumber[i]);
			break;
		case 'N':
			err = pthread_create(&tid[i], &attr[i], north, &tidnumber[i]);
			break;
		}
		if (err != 0)
		{
			printf("创建第%d个线程失败!\n", i + 1);
			return 1;
		}
		usleep(1);
		i++;
	}

	/*等待所有线程结束*/
	for (i = 0; i<j; i++)
		pthread_join(tid[i], NULL);

	printf("END\n");
	return 0;
}

void *east(void *arg)
{
	int number, numinque; /*车号、在waitqueue中的位置号*/

	pthread_mutex_lock(&mutex);
	number = *(int *)arg; /*获取车号*/
	count[0]++; /*东向等待车辆加一*/
	
	arrivalnumber[0]++;
	numinque = arrivalnumber[0]; /*获取位置号*/
	/*等待成为当前队列第一车*/
	while(!waitqueue[0][numinque])
	{
		pthread_cond_wait(&firstleave[0], &mutex);
	}
	printf("car %d from East arrives at crossing\n", number);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (go != -1 && go != 0) /*优先方向未指定或当前方向为优先方向则可以开始判断*/
	{
		pthread_cond_wait(&firstgo, &mutex);
	}
	go = 0;
	
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (true)
	{
		if (count[1] > 0 && count[2] > 0 && count[3] > 0&&goalong==false)/*死锁*/
		{
			go = 3; /*北车先走*/
			goalong = false;
			pthread_cond_broadcast(&firstgo);
			while (go != 0) /*等待优先方向为当前方向*/
			{
				pthread_cond_wait(&firstgo, &mutex);
			}
			continue;
		}
		else /*没有死锁*/
			if (goalong == false && count[3] != 0) /*不能直接离开且右侧有车*/
			{
				go = 3; /*右侧车先走*/
				pthread_cond_broadcast(&firstgo);
				while (go != 0) /*等待优先方向为当前方向*/
				{
					pthread_cond_wait(&firstgo, &mutex);
				}
				continue;
			}
			else
				break;
	}
	
		
	/*该车离开*/
	count[0]--;
	printf("car %d from East leaving crossing\n", number);
	if (count[1] != 0) /*左侧有车*/
	{
		go = 1; /*左侧车可以出发*/
		goalong = true;
	}
	else
	{
		go = -1; /*取消优先级*/
		goalong = false;
	}
	pthread_cond_broadcast(&firstgo);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	/*向队列后面的车发出离开信号*/
	pthread_mutex_lock(&mutex);
	waitqueue[0][numinque + 1] = true;
	pthread_cond_broadcast(&firstleave[0]);
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}

void *south(void *arg)
{
	int number, numinque; /*车号、在waitqueue中的位置号*/

	pthread_mutex_lock(&mutex);
	number = *(int *)arg; /*获取车号*/
	count[1]++; /*南向等待车辆加一*/
	
	arrivalnumber[1]++;
	numinque = arrivalnumber[1]; /*获取位置号*/
	/*等待成为当前队列第一车*/
	while (!waitqueue[1][numinque])
	{
		pthread_cond_wait(&firstleave[1], &mutex);
	}
	printf("car %d from South arrives at crossing\n", number);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (go != -1 && go != 1) /*优先方向未指定或当前方向为优先方向则可以开始判断*/
	{
		pthread_cond_wait(&firstgo, &mutex);
	}
	go = 1;
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (true)
	{
		if (count[0] > 0 && count[2] > 0 && count[3] > 0&&goalong==false) /*死锁*/
		{
			go = 3; /*北车先走*/
			goalong = false;
			pthread_cond_broadcast(&firstgo);
			while (go != 1) /*等待优先方向为当前方向*/
			{
				pthread_cond_wait(&firstgo, &mutex);
			}
			continue;
		}
		else /*没有死锁*/
			if (goalong == false && count[0] != 0) /*不能直接离开且右侧有车*/
			{
				go = 0; /*右侧车先走*/
				pthread_cond_broadcast(&firstgo);
				while (go != 1) /*等待优先方向为当前方向*/
				{
					pthread_cond_wait(&firstgo, &mutex);
				}
				continue;
			}
			else
				break;
	}


	/*该车离开*/
	count[1]--;
	printf("car %d from South leaving crossing\n", number);
	if (count[2] != 0) /*左侧有车*/
	{
		go = 2; /*左侧车可以出发*/
		goalong = true;
	}
	else
	{
		go = -1; /*取消优先级*/
		goalong = false;
	}
	pthread_cond_broadcast(&firstgo);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	/*向队列后面的车发出离开信号*/
	pthread_mutex_lock(&mutex);
	waitqueue[1][numinque + 1] = true;
	pthread_cond_broadcast(&firstleave[1]);
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}

void *west(void *arg)
{
	int number, numinque; /*车号、在waitqueue中的位置号*/

	pthread_mutex_lock(&mutex);
	number = *(int *)arg; /*获取车号*/
	count[2]++; /*西向等待车辆加一*/
	
	arrivalnumber[2]++;
	numinque = arrivalnumber[2]; /*获取位置号*/
	/*等待成为当前队列第一车*/
	while (!waitqueue[2][numinque])
	{
		pthread_cond_wait(&firstleave[2], &mutex);
	}
	printf("car %d from West arrives at crossing\n", number);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (go != -1 && go != 2) /*优先方向未指定或当前方向为优先方向则可以开始判断*/
	{
		pthread_cond_wait(&firstgo, &mutex);
	}
	go = 2;
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (true)
	{
		if (count[0] > 0 && count[1] > 0 && count[3] > 0&&goalong==false) /*死锁*/
		{
			go = 3; /*北车先走*/
			goalong = false;
			pthread_cond_broadcast(&firstgo);
			while (go != 2) /*等待优先方向为当前方向*/
			{
				pthread_cond_wait(&firstgo, &mutex);
			}
			continue;
		}
		else /*没有死锁*/
			if (goalong == false && count[1] != 0) /*不能直接离开且右侧有车*/
			{
				go = 1; /*右侧车先走*/
				pthread_cond_broadcast(&firstgo);
				while (go != 2) /*等待优先方向为当前方向*/
				{
					pthread_cond_wait(&firstgo, &mutex);
				}
				continue;
			}
			else
				break;
	}


	/*该车离开*/
	count[2]--;
	printf("car %d from West leaving crossing\n", number);
	if (count[3] != 0) /*左侧有车*/
	{
		go = 3; /*左侧车可以出发*/
		goalong = true;
	}
	else
	{
		go = -1; /*取消优先级*/
		goalong = false;
	}
	pthread_cond_broadcast(&firstgo);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	/*向队列后面的车发出离开信号*/
	pthread_mutex_lock(&mutex);
	waitqueue[2][numinque + 1] = true;
	pthread_cond_broadcast(&firstleave[2]);
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}

void *north(void *arg)
{
	int number, numinque; /*车号、在waitqueue中的位置号*/

	pthread_mutex_lock(&mutex);
	number = *(int *)arg; /*获取车号*/
	count[3]++; /*北向等待车辆加一*/
	
	arrivalnumber[3]++;
	numinque = arrivalnumber[3]; /*获取位置号*/
	/*等待成为当前队列第一车*/
	while (!waitqueue[3][numinque])
	{
		pthread_cond_wait(&firstleave[3], &mutex);
	}
	printf("car %d from North arrives at crossing\n", number);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (go != -1 && go != 3) /*优先方向未指定或当前方向为优先方向则可以开始判断*/
	{
		pthread_cond_wait(&firstgo, &mutex);
	}
	go = 3;
	pthread_mutex_unlock(&mutex);
	usleep(1);
	pthread_mutex_lock(&mutex);
	while (true)
	{
		if (count[0] > 0 && count[1] > 0 && count[2] > 0) /*死锁*/
		{
			printf("DEADLOCK: car jam detected, signalling North to go\n");
			/*该车离开*/
			count[3]--;
			printf("car %d from North leaving crossing\n", number);
			
				go = 0; /*东车先走*/
			goalong = true;
			pthread_cond_broadcast(&firstgo);

			/*向队列后面的车发出离开信号*/
			waitqueue[3][numinque + 1] = true;
			pthread_cond_broadcast(&firstleave[3]);
			pthread_mutex_unlock(&mutex);

			pthread_exit(0);
		}
		else /*没有死锁*/
			if (goalong == false && count[2] != 0) /*不能直接离开且右侧有车*/
			{
				go = 2; /*右侧车先走*/
				pthread_cond_broadcast(&firstgo);
				while (go != 3) /*等待优先方向为当前方向*/
				{
					pthread_cond_wait(&firstgo, &mutex);
				}
				continue;
			}
			else
				break;
	}

	/*该车离开*/
	count[3]--;
	printf("car %d from North leaving crossing\n", number);
	if (count[0] != 0) /*左侧有车*/
	{
		go = 0; /*左侧车可以出发*/
		goalong = true;
	}
	else
	{
		go = -1; /*取消优先级*/
		goalong = false;
	}
	pthread_cond_broadcast(&firstgo);
	pthread_mutex_unlock(&mutex);
	usleep(1);
	/*向队列后面的车发出离开信号*/
	pthread_mutex_lock(&mutex);
	waitqueue[3][numinque + 1] = true;
	pthread_cond_broadcast(&firstleave[3]);
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}
