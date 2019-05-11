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
#define CLK_KEY 0x3693
#define FLAGS (O_CREAT | O_EXCL)
#define PERMS (mode_t) (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define MAX 12
#define MAXACTIVE 4

#define FRAMELEN 32

/* STRUCTURES */
struct iClock {
	unsigned long seco;		// whole second val
	unsigned long nano;		//  nano offset val
};

struct pageTable {
	int name;			// name of page as int
	int refByte;			// 8 bit(byte)logical address up to 128, use as size
	int dirtyBit;			// int representing if page is dirty (0|1)
	int isRead;			// int representing read or write (0|1) 1 = read
	int address;			// the logical address of the page
};

struct shObj {
	int pComplete;			// number of complete processes
	int memSize;			// max memory allocation   256k
	int frames[FRAMELEN];		// free frame vector (convert int to binary for logical address)
	int pagesRequested;		// number of pages requested
	int refBytes[FRAMELEN];		// refbytes in frames
	bool hasQueue;			// signal declaring whether a queue exists
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
bool framesFull();
int sumRefBytes();
int shiftBitFrames(int * array, int size);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
void sigintHandler(int sig_num);
/**************/

int main(int argc, char * argv[]){
	printf("\t\t--> OSS START <--\n\t\tParent PID: %d\n\n",getpid());	
	/* READ COMMAND LINE ARGS */
	int max = ( argv[1] ) ? atol(argv[1]) : MAX;
	int maxActive = ( argv[1] && argv[2] ) ? atol(argv[2]) : (MAXACTIVE < max) ? MAXACTIVE : max;

	/* SEMAPHORE */
	if(getnamed(sema, &semaphore, 1) == -1){
		perror("Failed to create named semaphore");
		return 1;}

	/* ESTABLISH DATA STRUCTURE IN MEMORY SEGMENT */
	if((shm_0 = shmget(SHM_KEY, SHMSZ, IPC_CREAT | 0666)) < 0){
 		perror("OSS: Shared memory create shm: shmget()");
		exit(1);}
	if((shm = shmat(shm_0, NULL, 0)) == (void*) -1){
		perror("OSS: Shared memory attach shm: shmat()");
		exit(1);}
	/* ESTABLISH CLOCK IN MEMORY SEGMENT */
	if((shm_1 = shmget(CLK_KEY, SHMSZ, IPC_CREAT | 0666)) < 0){
 		perror("OSS: Shared memory create clock: shmget()");
		exit(1);}
	if((ptime = shmat(shm_1, NULL, 0)) == (void*) -1){
		perror("OSS: Shared memory attach clock: shmat()");
		exit(1);}
	/* INIT VARIABLES */
	int pid, status, pActive;
	ptime->nano = 0;
	ptime->seco = 0;	
	shm->pComplete = 0;
	shm->memSize = 0;
	shm->pagesRequested = 0;
	int faultOccurence = 0;
	printf("memSize = %d Max= %d Max-Active= %d\n", shm->memSize, max, maxActive);

	while(shm->pComplete < max){
		signal(SIGINT, sigintHandler);
		sem_wait(semaphore);
		shm->memSize = sumRefBytes();
		sem_post(semaphore);
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

		/* PAGE FAULT ADJUSTER: SHIFT BITS EVERY 3 REQUESTS */
		if(shm->hasQueue){
			printf("OSS: PAGE FAULT HANDLER EXECUTED (%d/256)\n", shm->memSize);
			shiftBitFrames(shm->refBytes, FRAMELEN);
			sem_wait(semaphore);
			shm->memSize = sumRefBytes();
			sem_post(semaphore);
			/* IF FRAMES ARE FULL: REPLACE A FRAME */
			if(framesFull()){
				int min = 9999;
				int mini = 0;
				for(int i = 0; i < FRAMELEN; i++){
					if(min > shm->frames[i]){
						min = shm->frames[i];
						mini = i;
					}
				}
				shm->frames[mini] = 0;
			}

			shm->hasQueue = false;
			unsigned long time = ptime->seco*(unsigned long)1e9 + ptime->nano;
			time += 5e8;
			ptime->seco = time/(unsigned long)1e9;
			ptime->nano = time%(unsigned long)1e9;
			faultOccurence++;

		}

		/* CATCH EXIT SIGNAL OF COMPLETED CHILDREN */
		while(waitpid(-1, &status, WNOHANG) > 0){	
			//printf("OSS: Parent recognized child completed at %lu:%lu\n", ptime->seco, ptime->nano);
			pActive--;
			shm->pComplete++;
		}	
	}	

	/* WAIT FOR ALL CHILDREN TO COMPELTE */
	while((pid = wait(&pid)) > 0);
	printf("OSS: There were %d page requests made\n", shm->pagesRequested);
	printf("OSS: There were %d page fault handler events\n", faultOccurence);
	printf("OSS: Deallocating and quitting..\n");
	/* DEALLOCATE */
	sem_unlink("SEMA6");
	if(shmdt(shm) == -1){
		perror("Shared memory detach: shmdt()");
		exit(1);}
	if(shmctl(shm_0, IPC_RMID, 0) == -1){
		perror("Shared memory remove: shmctl()");
		exit(1);}
	if(shmdt(ptime) == -1){
		perror("OSS: Shared memory detach: shmdt()");
		exit(1);}
	if(shmctl(shm_1, IPC_RMID, 0) == -1){
		perror("OSS: Shared memory remove: shmctl()");
		exit(1);}
	return 0;
}

/* FUNCTIONS */
bool framesFull(){
	for(int i = 0; i < FRAMELEN; i++){
		if(shm->frames[i] > 0)
			return false;
	}
	return false;
}

int sumRefBytes(){
	/* GET THE SUM OFF ALL REFBYTES */
	int tempM = 0;
	for(int i = 0; i < FRAMELEN; i++){
		tempM += shm->refBytes[i];
	}
	return tempM;
}

int shiftBitFrames(int * array, int size){
	/* SHIFT THE BITS TO THE RIGHT OF THE INT ARRAY */
	for(int i = 0; i < size; i++){
		if(array[i] > 0){
			array[i] = array[i] >> 1;
		}
	}
	return 0;
}

int getRandomNumber(int low, int high){
	/* get random number within range */
	int num;
	for ( int i = 0; i < 2; i++ ){
		num = (rand() % (high - low + 1)) + low;
	}
	return num;
}

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
	/* DEALLOCATE */
	sem_unlink("SEMA6");
	if(shmdt(shm) == -1){
		perror("Shared memory detach: shmdt()");
		exit(1);}
	if(shmctl(shm_0, IPC_RMID, 0) == -1){
		perror("Shared memory remove: shmctl()");
		exit(1);}
	if(shmdt(ptime) == -1){
		perror("OSS: Shared memory detach: shmdt()");
		exit(1);}
	if(shmctl(shm_1, IPC_RMID, 0) == -1){
		perror("OSS: Shared memory remove: shmctl()");
		exit(1);}
	kill(0,SIGTERM);
	exit(0);
}
