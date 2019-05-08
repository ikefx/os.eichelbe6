/* NEIL EICHELBERGER
 * cs4760 assignment 6
 * user.c file */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <stdbool.h>

#define SHM_KEY 0x3963
#define CLK_KEY 0x3693
#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define FRAMELEN 32

/* STRUCTURES */
struct iClock {
	unsigned long seco;		// whole second val
	unsigned long nano;		//  nano offset val
};

struct pageTable {
	int refByte;			// int representing 8 bit(byte) logical address
	int dirtyBit;			// int representing if page is dirty (0|1)
};

struct shObj {
	int pComplete;			// number of complete processes
	int memSize;			// max memory allocation   256k
	int frames[FRAMELEN];		// free frame vector (convert int to binary for logical address)
};
/**************/
/* GLOBALS ****/
char  * sema = "SEMA6";
sem_t * semaphore;
int shm_1;
struct iClock * ptime;
size_t CLOCKSIZE = sizeof(struct iClock);
int shm_0;
struct shObj * shm;
size_t SHMSZ = sizeof(struct shObj);
/**************/

/* PROTOTYPES */
int getnamed(char *name, sem_t **sem, int val);
/**************/

int main(int argc, char * argv[]){

	/* SEMAPHORE */
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;}

	/* ESTABLISH DATA STRUCTURE IN MEMORY SEGMENT */
	if((shm_0 = shmget(SHM_KEY, SHMSZ, 0666)) < 0){
 		perror("OSS: Shared memory create shm: shmget()");
		exit(1);}
	if((shm = shmat(shm_0, NULL, 0)) == (void*) -1){
		perror("OSS: Shared memory attach shm: shmat()");
		exit(1);}
	/* ESTABLISH CLOCK IN MEMORY SEGMENT */
	if((shm_1 = shmget(CLK_KEY, SHMSZ, 0666)) < 0){
 		perror("OSS: Shared memory create clock: shmget()");
		exit(1);}
	if((ptime = shmat(shm_1, NULL, 0)) == (void*) -1){
		perror("OSS: Shared memory attach clock: shmat()");
		exit(1);}
	
	printf("\t\tSTART USER: I am the child and my pid is %s:%d\n", argv[1], getpid());	
	usleep(1000000);
	printf("\t\tEND USER: I am finished. Child %s:%d\n", argv[1], getpid());
	ptime->seco += 1;
	ptime->nano += 5e8;

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
