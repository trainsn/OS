#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define CAR_NUMBER 100 
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

char* name[4] = { "East", "South", "West", "North" };

pthread_mutex_t dirMutex[4];	//互斥锁
pthread_mutex_t waitMutex[4];	
pthread_cond_t firstDir[4];		//某个方向成为可走方向的条件变量
pthread_cond_t queue[4];		//某个方向队列中第一辆车离开的条件变量

bool goAlong[4];	//true:某个方向可以直接离开，而不用考虑右边是否有车

struct Wait
{
	int front, rear;
	int num[CAR_NUMBER];//the waiting cars in every intersection 
	int count;	//	the count of waiting cars in every intersection 
	
	Wait()
	{
		front = rear = count = 0;
	}

	int pop()
	{
		count--;
		front = (front + 1) % CAR_NUMBER;
		return num[front];
	}

	void push(int id)
	{
		count++;
		num[rear] = id;
		rear = (rear + 1) % CAR_NUMBER;	
	}

	int getHead()
	{
		return num[front];
	}
};

Wait waitQueue[4];

int main(void *arg)
{
	int i,err, j;
	char array[20];

	//Initialize
	for (int i = 0; i < 4; i++)
	{
		pthread_cond_init(&firstDir[i], NULL);
		pthread_cond_init(&queue[i], NULL);
		pthread_mutex_init(&dirMutex[i], NULL);
		pthread_mutex_init(&waitMutex[i], NULL);
	}

	for (int i = 0; i < CAR_NUMBER; i++)
	{
		tidnumber[i] = i + 1;
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

	//arrive
	pthread_mutex_lock(&waitMutex[dir]);
	number = *(int*)(arg);	//获取车号

	waitQueue[dir].push(number);
	while (waitQueue[dir].getHead()!=number)
		pthread_cond_wait(&queue[dir],&waitMutex[dir]);
	printf("car %d from %s arrives at crossing\n", number,name[dir]);
	pthread_mutex_unlock(&waitMutex[dir]);

	pthread_mutex_lock(&dirMutex[dir]);
	while (1)
	{
		//cross		
		bool isDeadlock;
		isDeadlock = true;
		
		for (int i = 0; i < 4; i++)
		{
			if (waitQueue[i].count == 0 || goAlong[i])
				isDeadlock = false;
		}	
// 		for (int i = 0; i < 4; i++)
// 			printf("%s %d ", name[i], goAlong[i]);
// 		printf("\n");
				
		if (isDeadlock)
		{
			printf("DEADLOCK: car jam detected, signalling North to go\n");
			if (dir == NORTH)
			{
				for (int i = 0; i < 4; i++)
					if (i != dir)
						goAlong[i] = false;
				goAlong[dir] = true;		
				
				break;
			}
		}

		while (waitQueue[RIGHT(dir)].count> 0 && !goAlong[dir])
			pthread_cond_wait(&firstDir[dir], &dirMutex[dir]);
		usleep(200000);
		break;	
	}
	
	//this car leaves
	waitQueue[dir].pop();
	goAlong[dir] = false;
	printf("car %d from %s leaving crossing\n", number,name[dir]);
	pthread_cond_broadcast(&queue[dir]);
	if (waitQueue[LEFT(dir)].count != 0)//如果此时左侧有车
	{
		for (int i = 0; i < 4; i++)
			if (i != LEFT(dir) && i!=RIGHT(dir))
				goAlong[i] = false;
		goAlong[LEFT(dir)] = true;
		pthread_cond_signal(&firstDir[LEFT(dir)]);
	}
	pthread_mutex_unlock(&dirMutex[dir]);

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