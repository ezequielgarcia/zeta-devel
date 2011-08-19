#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

#define THREAD_COUNT			100
#define STACK_SIZE              (10*1024*1024)
#define TASK_ARRAY_SIZE 		(STACK_SIZE-(1024*10))

int broadcast_stop = 0;

void * task()
{
   int i=0;
   char array[TASK_ARRAY_SIZE];

	for (i=0; i < TASK_ARRAY_SIZE; i++)
    	array[i]=0x55;

	while (!broadcast_stop) 
		sleep(1);

	printf("done\n");
	return 0;
}


int main()
{
	size_t				 i;
	int                  result=0;
	size_t               stacksize=0;
	pthread_attr_t       attr;
	pthread_t            tid[THREAD_COUNT];

	result = pthread_attr_init(&attr);

	stacksize = STACK_SIZE;
	result = pthread_attr_setstacksize(&attr, stacksize);
	if(result)
		printf("%s\n", strerror(result));

	result = pthread_attr_getstacksize(&attr, &stacksize);
	printf("Tasks stack size = %d\n", stacksize);

	for (i=0; i<THREAD_COUNT; i++) {

		result = pthread_create(&tid[i], &attr, task, (void*)NULL);
		if(result)
			printf("%s\n", strerror(result));

		printf("Task %d created \n", i);
	}

	pthread_attr_destroy(&attr);

	printf("Done, press enter to stop em all\n");
	getchar();

	broadcast_stop = 1;

	for (i=0; i<THREAD_COUNT; i++) {

		if(tid[i] != 0)
			pthread_join( tid[i], NULL);
	}

	return 0;
}

