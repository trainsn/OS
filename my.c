#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define CAR_NUMBER 20 /*通过路口的车辆总数*/
#define UNDIFINDED -1
#define EAST 0
#define SOUTH 1
#define WEST 2
#define NORTH 3
#define RIGHT(X) (X+3)%4
#define LEFT(X) (X+1)%4

void *CarEast(void *arg); /*东车线程*/
void *CarSouth(void *arg); /*南车线程*/
void *CarWest(void *arg); /*西车线程*/
void *CarNorth(void *arg); /*北车线程*/

pthread_t tid[CAR_NUMBER];//car's thread number
pthread_attr_t attr[CAR_NUMBER];//the corresponding attr
int tidnumber[CAR_NUMBER];

bool waitQueue[4][CAR_NUMBER];	//the queue for all cars
int arriveNum[4];	//the car 
int count[4];
int go; //which direction is the current privileged dir
bool goAlong;	//when car in one dir leaves, it gives one signal to its left car 
char* name[4] = { "East", "South", "West", "North" };

pthread_mutex_t mutex;
pthread_cond_t firstLeave[4];
pthread_cond_t firstGo;

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
		case 'E':
			err = pthread_create(&tid[i], &attr[i], CarEast, &tidnumber[i]);
			break;
		case 'S':
			err = pthread_create(&tid[i], &attr[i], CarSouth, &tidnumber[i]);
			break;
		case 'W':
			err = pthread_create(&tid[i], &attr[i], CarWest, &tidnumber[i]);
			break;
		case 'N':
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


void *CarEast(void *arg)
{
	int dir = EAST;
	int number, queIndex;

	pthread_mutex_lock(&mutex);
	number = *(int*)(arg);
	count[dir]++;

	queIndex = ++arriveNum[dir]; //get the position in the queue
	while (!waitQueue[dir][queIndex])//wait until the car becomes the first car 
	{
		pthread_cond_wait(&firstLeave[dir], &mutex);
	}
	printf("car %d from %s arrives at crossing\n", number, name[dir]);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (go != UNDIFINDED && go != dir)
	{
		pthread_cond_wait(&firstGo, &mutex);
	}
	go = dir;
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (1)
	{
		bool deadlock = true;
		for (int i = 0; i < 4; i++)
			if (i != dir && count[i] <= 0)
				deadlock = false;
		if (deadlock && goAlong == false)
		{
				go = NORTH;
				goAlong = false;
				pthread_cond_broadcast(&firstGo);
				while (go != dir)
				{
					pthread_cond_wait(&firstGo, &mutex);
				}
				continue;
		}
		else
			if (goAlong == false && count[RIGHT(dir)] > 0)//since north is in the right of east 
			{
			//printf("RIGHT= %d", RIGHT(dir));
			go = RIGHT(dir);
			pthread_cond_broadcast(&firstGo);
			while (go != dir)
			{
				pthread_cond_wait(&firstGo, &mutex);
			}
			continue;
			}
			else break;
	}

	//this car leaves
	count[dir]--;
	printf("car %d from %s leaving crossing\n", number, name[dir]);
	if (count[LEFT(dir)] != 0)//since south is in the left of east
	{
		go = LEFT(dir);	//car in the south is able to start 
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

void *CarSouth(void *arg)
{
	int dir = SOUTH;
	int number, queIndex;

	pthread_mutex_lock(&mutex);
	number = *(int*)(arg);
	count[dir]++;

	queIndex = ++arriveNum[dir]; //get the position in the queue
	while (!waitQueue[dir][queIndex])//wait until the car becomes the first car 
	{
		pthread_cond_wait(&firstLeave[dir], &mutex);
	}
	printf("car %d from %s arrives at crossing\n", number, name[dir]);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (go != UNDIFINDED && go != dir)
	{
		pthread_cond_wait(&firstGo, &mutex);
	}
	go = dir;
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (1)
	{
		bool deadlock = true;
		for (int i = 0; i < 4; i++)
			if (i != dir && count[i] <= 0)
				deadlock = false;
		if (deadlock && goAlong==false)
		{
			if (dir != NORTH)
			{
				go = NORTH;
				goAlong = false;
				pthread_cond_broadcast(&firstGo);
				while (go != dir)
				{
					pthread_cond_wait(&firstGo, &mutex);
				}
			}
			else {
				printf("DEADLOCK: car jam detected, signalling North to go\n");
				/*该车离开*/
				count[dir]--;
				printf("car %d from North leaving crossing\n", number);

				go = EAST;
				goAlong = true;
				pthread_cond_broadcast(&firstGo);

				//signal for the latter car in the queue 
				waitQueue[NORTH][queIndex + 1] = true;
				pthread_cond_broadcast(&firstLeave[NORTH]);
				pthread_mutex_unlock(&mutex);

				pthread_exit(0);
			}
			continue;
		}
		else
			if (goAlong == false && count[RIGHT(dir)] > 0)//since north is in the right of east 
			{
			go = RIGHT(dir);
			pthread_cond_broadcast(&firstGo);
			while (go != dir)
			{
				pthread_cond_wait(&firstGo, &mutex);
			}
			continue;
			}
			else break;
	}

	//this car leaves
	count[dir]--;
	printf("car %d from %s leaving crossing\n", number, name[dir]);
	if (count[LEFT(dir)] != 0)//since south is in the left of east
	{
		go = LEFT(dir);	//car in the south is able to start 
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

void *CarWest(void *arg)
{
	int dir = WEST;
	int number, queIndex;

	pthread_mutex_lock(&mutex);
	number = *(int*)(arg);
	count[dir]++;

	queIndex = ++arriveNum[dir]; //get the position in the queue
	while (!waitQueue[dir][queIndex])//wait until the car becomes the first car 
	{
		pthread_cond_wait(&firstLeave[dir], &mutex);
	}
	printf("car %d from %s arrives at crossing\n", number, name[dir]);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (go != UNDIFINDED && go != dir)
	{
		pthread_cond_wait(&firstGo, &mutex);
	}
	go = dir;
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (1)
	{
		bool deadlock = true;
		for (int i = 0; i < 4; i++)
			if (i != dir && count[i] <= 0)
				deadlock = false;
		if (deadlock && goAlong == false)
		{
			if (dir != NORTH)
			{
				go = NORTH;
				goAlong = false;
				pthread_cond_broadcast(&firstGo);
				while (go != dir)
				{
					pthread_cond_wait(&firstGo, &mutex);
				}
			}
			else {
				printf("DEADLOCK: car jam detected, signalling North to go\n");
				/*该车离开*/
				count[dir]--;
				printf("car %d from North leaving crossing\n", number);

				go = EAST;
				goAlong = true;
				pthread_cond_broadcast(&firstGo);

				//signal for the latter car in the queue 
				waitQueue[NORTH][queIndex + 1] = true;
				pthread_cond_broadcast(&firstLeave[NORTH]);
				pthread_mutex_unlock(&mutex);

				pthread_exit(0);
			}
			continue;
		}
		else
			if (goAlong == false && count[RIGHT(dir)] > 0)//since north is in the right of east 
			{
			go = RIGHT(dir);
			pthread_cond_broadcast(&firstGo);
			while (go != dir)
			{
				pthread_cond_wait(&firstGo, &mutex);
			}
			continue;
			}
			else break;
	}

	//this car leaves
	count[dir]--;
	printf("car %d from %s leaving crossing\n", number, name[dir]);
	if (count[LEFT(dir)] != 0)//since south is in the left of east
	{
		go = LEFT(dir);	//car in the south is able to start 
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

void *CarNorth(void *arg)
{
	int dir = NORTH;
	int number, queIndex;

	pthread_mutex_lock(&mutex);
	number = *(int*)(arg);
	count[dir]++;

	queIndex = ++arriveNum[dir]; //get the position in the queue
	while (!waitQueue[dir][queIndex])//wait until the car becomes the first car 
	{
		pthread_cond_wait(&firstLeave[dir], &mutex);
	}
	printf("car %d from %s arrives at crossing\n", number, name[dir]);
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (go != UNDIFINDED && go != dir)
	{
		pthread_cond_wait(&firstGo, &mutex);
	}
	go = dir;
	pthread_mutex_unlock(&mutex);
	usleep(1);

	pthread_mutex_lock(&mutex);
	while (1)
	{
		bool deadlock = true;
		for (int i = 0; i < 4; i++)
			if (i != dir && count[i] <= 0)
				deadlock = false;
		if (deadlock)
		{
			printf("DEADLOCK: car jam detected, signaling North to go\n");
				/*该车离开*/
			count[dir]--;
			printf("car %d from North leaving crossing\n", number);

			go = EAST;
			goAlong = true;
			pthread_cond_broadcast(&firstGo);

			//signal for the latter car in the queue 
			waitQueue[NORTH][queIndex + 1] = true;
			pthread_cond_broadcast(&firstLeave[NORTH]);
			pthread_mutex_unlock(&mutex);

			pthread_exit(0);
		}
		else
			if (goAlong == false && count[RIGHT(dir)] > 0)//since north is in the right of east 
			{
			go = RIGHT(dir);
			pthread_cond_broadcast(&firstGo);
			while (go != dir)
			{
				pthread_cond_wait(&firstGo, &mutex);
			}
			continue;
			}
			else break;
	}

	//this car leaves
	count[dir]--;
	printf("car %d from %s leaving crossing\n", number, name[dir]);
	if (count[LEFT(dir)] != 0)//since south is in the left of east
	{
		go = LEFT(dir);	//car in the south is able to start 
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