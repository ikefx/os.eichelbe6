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

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define MAX 1
#define FRAMESIZE 8
#define BITLENGTH 8 * 8

/* STRUCTURES */
struct shObj {
	int pActive;			// number of active processes
	int pTotal;			// total number of active processes
	int pComplete;			// number of complete processes
	int frameT[8];
	int frames[BITLENGTH];
	int memSize;			// max memory allocation   256k
};

/* GLOBALS */
int shm_0;
struct shObj * shm;
size_t SHMSZ = sizeof(struct shObj);

int main(int argc, char * argv[]){

	/* ESTABLISH SHM SEGMENT */
	if((shm_0 = shmget(SHM_KEY, SHMSZ, 0666)) < 0){
		perror("USER: Shared memory create: shmget()");
		exit(1);}
	if((shm = shmat(shm_0, NULL, 0)) == (void*) -1){
		perror("USER: Shared memory attach: shmat()");
		exit(1);}
	
	printf("\t\tSTART USER: I am the child and my pid is %s:%d\n", argv[1], getpid());	

	usleep(1000000);

	printf("\t\tEND USER: I am finished. Child %s:%d\n", argv[1], getpid());

	/* DETACH FROM SEGMENT */
	if(shmdt(shm) == -1){
		perror("DETACHING SHARED MEMORY: shmdt()");
		return 1;}	

	return 0;
}

