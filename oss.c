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

#define SHM_KEY 0x3963

#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define MAX 3
#define MAXACTIVE 2
#define FRAMESIZE 8
#define BITLENGTH 8 * 8

/* STRUCTURES */
struct iClock {
	unsigned long seco;		// whole second val
	unsigned long nano;		//  nano offset val
};

struct pageT {
	int dirtybit;			//              0 or 1
	int logicalAddress;		// logical address int
};

struct shObj {
	int pActive;			// number of active processes
	int pTotal;			// total number of active processes
	int pComplete;			// number of complete processes
	int frameT[8];
	int frames[BITLENGTH];
	int memSize;			// max memory allocation   256k
};

/* PROTOTYPES */
int getnamed(char *name, sem_t **sem, int val);
void waitHandler(int sig);

/* GLOBALS */
char  * sema = "SEMA6";
sem_t * semaphore;

struct iClock * clockp;
size_t CLOCKSIZE = sizeof(struct iClock);

int shm_0;
struct shObj * shm;
size_t SHMSZ = sizeof(struct shObj);

int main(int argc, char * argv[]){
	printf("\t\t--> OSS START <--\n\t\tParent PID: %d\n\n",getpid());	
	/* READ COMMAND LINE ARGS */
	int max = ( argv[1] ) ? atol(argv[1]) : MAX;
	int maxActive = ( argv[1] && argv[2] ) ? atol(argv[2]) : (MAXACTIVE < max) ? MAXACTIVE : max;

	/* SEMAPHORE */
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;}
	
	/* INIT VARIABLES */
	int pid, status, pActive;
	clockp = malloc(CLOCKSIZE);
	clockp->seco = 100;
	clockp->nano = 100e9;

	/* ESTABLISH SHM SEGMENT */
	if((shm_0 = shmget(SHM_KEY, SHMSZ, IPC_CREAT | 0666)) < 0){
 		perror("Shared memory create: shmget()");
		exit(1);}
	if((shm = shmat(shm_0, NULL, 0)) == (void*) -1){
		perror("Shared memory attach: shmat()");
		exit(1);}
	shm->pActive   = 0;
	shm->pComplete = 0;
	shm->memSize = 256;
	printf("BIT STREAM:\n");
	for(int i = 0; i < BITLENGTH; i++){
		shm->frames[i] = 0;
		printf("%d", shm->frames[i]);
	}
	printf("\n");
	printf("%lu %lu\n", clockp->seco, clockp->nano);
	printf("Change second int to 1:\n");
	shm->frames[8*2] = 1;
	printf("BIT STREAM:\n");
	for(int i = 0; i < BITLENGTH; i++){
		printf("%d", shm->frames[i]);
	}
	printf("\n");
	printf("memSize = %d Max= %d Max-Active= %d\n", shm->memSize, max, maxActive);
	while(shm->pComplete < max){

		/* PRODUCE UP TO MAX-ACTIVE PROCESSES AT ONE TIME */
		for(int i = pActive; i < maxActive; i++){
			pActive++;
			if(shm->pComplete + i < max){
				if((pid = fork()) == 0){			 
					char spTotal[(int)((ceil(log10(pActive))+1)*sizeof(char))];
					sprintf(spTotal, "%d", pActive);
					char * args[] = {"./user", spTotal, '\0'};
					execvp("./user", args);
				}
			}
		}

		/* CATCH EXIT SIGNAL OF COMPLETED CHILDREN */
		while(waitpid(-1, &status, WNOHANG) > 0){	
			printf("OSS: Parent recognized child completed\n");
			pActive--;
			shm->pComplete++;
		}	
	}	

	/* WAIT FOR ALL CHILDREN TO COMPELTE */
	while((pid = wait(&pid)) > 0);

	/* DEALLOCATE */
	sem_unlink("SEMA6");
	if(shmdt(shm) == -1){
		perror("Shared memory detach: shmdt()");
		exit(1);}
	if(shmctl(shm_0, IPC_RMID, 0) == -1){
		perror("Shared memory remove: shmctl()");
		exit(1);
	}
	free(clockp);
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

void waitHandler(int sig){
	/* WAIT FOR CHILD */
	//pid_t pid;
	//pid = wait(NULL);
	//printf("Something happened\n");
}

void sigintHandler(int sig_num){
	/* CTRL C KILL */
	signal(SIGINT, sigintHandler);
	printf("\nTerminating all...\n");
	sem_unlink(sema);
	kill(0,SIGTERM);
	exit(0);
}
