#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define CAR_NUMBER 20 
#define UNDIFINDED -1
#define EAST 0
#define SOUTH 1
#define WEST 2
#define NORTH 3
#define RIGHT(X) (X+3)%4
#define LEFT(X) (X+1)%4


void *CarEast(void *arg); 
void *CarSouth(void *arg); 
void *CarWest(void *arg); 
void *CarNorth(void *arg);

pthread_t tid[CAR_NUMBER];//car's thread number
pthread_attr_t attr[CAR_NUMBER];//the correspoding attr
int tidnumber[CAR_NUMBER]; //每个线程被创建的序号

bool waitQueue[4][CAR_NUMBER];	//the queue for all cars , true if this car is the first in this dir
int arriveNum[4];	//every car gets its position in waitQueue from the arriveNum 
int count[4];//the waiting cars in every intersection 
int go; //which direction is the current privileged dir
bool goAlong;	//when car in one dir leaves, it gives one signal to its left car 
char* name[4] = { "East", "South", "West", "North" };

pthread_mutex_t mutex;	//互斥锁
pthread_cond_t firstLeave[4];	//某个方向队列中第一辆车离开的条件变量
pthread_cond_t firstGo;		//出发条件变量

int main(void *arg)
{
	int i,err, j;
	char array[20];

	//Initialize
	go = -1;
	pthread_cond_init(&firstGo, NULL);
	pthread_mutex_init(&mutex, NULL);
	goAlong = false;
	for (int i = 0; i < 4; i++)
	{
		count[i] = 0;
		arriveNum[i]= - 1;
		waitQueue[i][0] = true;
		pthread_cond_init(&firstLeave[i], NULL);
	}

	for (int i = 0; i < CAR_NUMBER; i++)
	{
		tidnumber[i] = i + 1;
	}
	for (int i = 1; i < CAR_NUMBER; i++)
	{
		for (int j = 0; j < 4; j++)
			waitQueue[j][i] = false;
	}
	for (int i = 0; i < CAR_NUMBER; i++)
	{
		pthread_attr_init(&attr[i]);
	}

	scanf("%s", array);
	i = 0;
	for (j = 0; array[j] != '\0'; j++)
	{
		switch (array[j])
		{
		case 'e':
			err = pthread_create(&tid[i], &attr[i], CarEast, &tidnumber[i]);
			break;
		case 's':
			err = pthread_create(&tid[i], &attr[i], CarSouth, &tidnumber[i]);
			break;
		case 'w':
			err = pthread_create(&tid[i], &attr[i], CarWest, &tidnumber[i]);
			break;
		case 'n':
			err = pthread_create(&tid[i], &attr[i], CarNorth, &tidnumber[i]);
			break;
		}
		if (err)
		{
			printf("create the %dth thread failed", i + 1);
			return 1;
		}
		usleep(1);
		i++;
	}

	for (int i = 0; i < j; i++)
		pthread_join(tid[i], NULL);

	printf("END\n");
	return 0;
}

void Treat(int dir,void *arg)
{
	int number, queIndex;//车号和在waitQueue中的位置号

	pthread_mutex_lock(&mutex);
	number = *(int*)(arg);	//获取车号
	count[dir]++;	//此方向等待车辆加1

	queIndex = ++arriveNum[dir]; //get the position in the queue
	while (!waitQueue[dir][queIndex])//wait until the car becomes the first car 
	{
		pthread_cond_wait(&firstLeave[dir], &mutex);
	}	
	printf("car %d from %s arrives at crossing\n", number,name[dir]);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (go != UNDIFINDED && go != dir)//如果优先方向不是未指定或者当前方向不是优先方向那么陷在while循环中，不然就进入下一步
	{
		pthread_cond_wait(&firstGo, &mutex);
	}
	go = dir;
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (1)
	{
		//开始判断死锁
		bool deadlock = true;
		for (int i = 0; i < 4; i++)
			if (i != dir && count[i] == 0)
				deadlock = false;

		if (deadlock && !goAlong)
		{
			if (dir != NORTH)//如果此时处理的方向不是北方
			{
				go = NORTH;//指定此时北车先走
				goAlong = false;
				pthread_cond_broadcast(&firstGo);
				while (go != dir)//等待优先方向为当前方向
				{
					pthread_cond_wait(&firstGo, &mutex);
				}
			}
			else {
				printf("DEADLOCK: car jam detected, signalling North to go\n");
				break;
			}
			continue;
		}
		else//如果没有死锁
			if (goAlong == false && count[RIGHT(dir)] > 0)//如果不能离开而且右侧有车
			{
			go = RIGHT(dir);//指定右侧车先走
			pthread_cond_broadcast(&firstGo);
			while (go != dir)//等待当前方向为优先方向
			{
				pthread_cond_wait(&firstGo, &mutex);
			}
			continue;
			}
			else break;
	}

	//this car leaves
	count[dir]--;
	//printf("%d\n", count[dir]);
	printf("car %d from %s leaving crossing\n", number,name[dir]);
	if (count[LEFT(dir)] != 0)//如果此时左侧有车
	{
		go = LEFT(dir);	//左侧车可以出发 
		goAlong = true;
	}
	else
	{
		go = UNDIFINDED;	//cancel the privilege
		goAlong = false;
	}
	pthread_cond_broadcast(&firstGo);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	//signal for the latter car in the queue 
	pthread_mutex_lock(&mutex);
	waitQueue[dir][queIndex + 1] = true;
	pthread_cond_broadcast(&firstLeave[dir]);
	pthread_mutex_unlock(&mutex);

	pthread_exit(0);
}

void *CarEast(void *arg)
{
	Treat(EAST, arg);
}

void *CarSouth(void *arg)
{
	Treat(SOUTH, arg);
}

void *CarWest(void *arg)
{
	Treat(WEST, arg);
}

void *CarNorth(void *arg)
{
	Treat(NORTH, arg);
}