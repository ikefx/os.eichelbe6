/* NEIL EICHELBERGER
 * cs4760 assignment 6
 * OSS file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FRAMESIZE 8
#define BITLENGTH 8 * 8
/* STRUCTURES */
struct iClock {
	unsigned long seco;
	unsigned long nano;
};

struct shObj {
	int frames[BITLENGTH];
};

/* PROTOTYPES */
int getnamed(char *name, sem_t **sem, int val);

/* GLOBALS */
char  * sema = "SEMA6";
sem_t * semaphore;

struct iClock * clockp;
size_t CLOCKSIZE = sizeof(struct iClock);

struct shObj * shmp;
size_t SHMSZ = sizeof(struct shObj);

int main(int argc, char * argv[]){
	printf("\t\t--> OSS START <--\n\t\tParent PID: %d\n\n",getpid());	
	/* SEMAPHORE */
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;
	}
	/* INIT VARIABLES */
	clockp = malloc(CLOCKSIZE);
	clockp->seco = 100;
	clockp->nano = 100e9;
	printf("%lu %lu\n", clockp->seco, clockp->nano);

	shmp = malloc(SHMSZ);
	printf("BIT STREAM:\n");
	for(int i = 0; i < BITLENGTH; i++){
		shmp->frames[i] = 0;
		printf("%d", shmp->frames[i]);
	}
	printf("\n");

	printf("Change second int to 1:\n");
	usleep(10000);
	shmp->frames[8*2] = 1;

	printf("BIT STREAM:\n");
	for(int i = 0; i < BITLENGTH; i++){
		printf("%d", shmp->frames[i]);
	}
	printf("\n");

	/* DEALLOCATE */
	sem_unlink("SEMA6");
	free(clockp);
	free(shmp);
	return 0;
}

/* FUNCTIONS */
int getnamed(char *name, sem_t **sem, int val){
	/* FUNCTION TO ACCESS NAMED SEMAPHORE | CREATING IF IT DOESNT EXIST */
	while(((*sem = sem_open(name, FLAGS , PERMS , val)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED) return 0;
	if(errno != EEXIST) return -1;
	while(((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR));
	if(*sem != SEM_FAILED) return 0;
	return -1;	
}

void sigintHandler(int sig_num){
	/* CTRL C KILL */
	signal(SIGINT, sigintHandler);
	printf("\nTerminating all...\n");
	sem_unlink(sema);
	kill(0,SIGTERM);
	exit(0);
}
