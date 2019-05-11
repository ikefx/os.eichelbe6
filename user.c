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
	int name;			// name of page as an int
	int refByte;			// int representing 8 bit(byte) logical address
	int dirtyBit;			// int representing if page is dirty (0|1)
	int isRead;			// is the page a read or write? 1 = read
	int address;			// simualted logical address
};

struct shObj {
	int pComplete;			// number of complete processes
	int memSize;			// max memory allocation   256k
	int frames[FRAMELEN];		// free frame vector (convert int to binary for logical address)
	int pagesRequested;		// number of pages requested
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
void printPageInfo(struct pageTable * table);
int getRandomNumber(int low, int high);
int getnamed(char *name, sem_t **sem, int val);
pid_t r_wait(int * stat_loc);
/**************/

int main(int argc, char * argv[]){
	srand(getpid());
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
	/* INIT VARIABLES */
	struct pageTable * childPage;	
	int pageRequestsLocal = 0;
	bool requesting = false;
	printf("\t\tSTART USER: I am the child and my pid is %s:%d\n", argv[1], getpid());	
	while(1){

		usleep(100000);
		
		if(getRandomNumber(0,100) < 20){		
			/* REQUEST A FRAME FOR A PAGE */	
			pageRequestsLocal++;
			shm->pagesRequested++;
			childPage = malloc(sizeof(struct pageTable));
			childPage->name = shm->pagesRequested;
			childPage->refByte = getRandomNumber(1,128);
			childPage->dirtyBit = 0;
			childPage->isRead = ( getRandomNumber(0,100) > 50 ) ? 1 : 0;
			childPage->address = getRandomNumber(10000, 99999);
			printf("\t\t\tUSER: Child %s:%d Requesting a frame for Page %d\n", argv[1], getpid(), childPage->name);
			printPageInfo(childPage);
	
			bool noFrameAvail = false;
			for(int i = 0; i < FRAMELEN; i++){
				if(shm->frames[i] == 0){
					printf("\t\tPlacing page in frame %d\n", i);
					shm->frames[i] = 1;
					shm->memSize += childPage->refByte;
					noFrameAvail = false;
					break;
				}
				noFrameAvail = true;
			}
			while(noFrameAvail){
				printf("frames full\n");
				printf("there were %d requests, and memory space is %d\n", shm->pagesRequested, shm->memSize);
				usleep(100000);
			}
			usleep(10000);
		}
		
		if(pageRequestsLocal > 0 && getRandomNumber(0,100) < 20){
			/* COMPLETE CHILD */
			printf("\t\tEND USER: I am finished. Child %s:%d\n", argv[1], getpid());
			ptime->seco += 1;
			ptime->nano += 5e8;
			break;
		}
	
	}
	
	if(r_wait(NULL) == -1){
		return 1;}
	if(sem_close(semaphore) < 0){
		perror("sem_close() error in child");}
	/* DETACH FROM SEGMENT */
	if(shmdt(shm) == -1){
		perror("DETACHING SHARED MEMORY: shmdt()");
		return 1;}	
	if(shmdt(ptime) == -1){
		perror("DETACHING SHARED MEMORY: shmdt()");
		return 1;}	
	free(childPage);
	return 0;
}

/* FUNCTIONS */

void printPageInfo(struct pageTable * table){
	/* PRINT TABLE INFO */
	printf("-------------------------------------------------------\n");
	printf(" %6s | %8s | %8s | %8s | %8s |\n", "NAME", "REFBYTE", "DIRTY", "R|W", "ADDRESS");
	printf(" %6d | %8d | %8d | %8s | %8d |\n", table->name, table->refByte, table->dirtyBit, (table->isRead == 1) ? "READ" : "WRITE", table->address);
	printf("-------------------------------------------------------\n");
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

pid_t r_wait(int * stat_loc){
	/* a function that restarts wait if interrupted by a signal */
	int retval;
	while(((retval = wait(stat_loc)) == -1) && (errno == EINTR));
	return retval;
}
