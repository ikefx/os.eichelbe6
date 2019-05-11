PROJECT 06 README
NEIL EICHELBERGER

files:

OSS

USER

README

CRITERIA:

	OSS
		> OSS accepts 2 argument parameters: ie to execute:
		
			$./oss 5 3
				> the first argument is the maximum number of processes to create/complete before 
				successful OSS termination

				> the second argument is the maximum number of processes allowed to run simultaneously
				** this value cannot be greater than 18 **
			Default values for these arguments are leveraged if these argv[]s are not provided

		

		> When a memory reference is made to a frame byte, set its most
		significant bit to 1

		> When `x` (user defined) amount of memory references are made to a specific frame
		shift the byte one bit to the right (setting left-most to zero)
		> When the frame vector fills, replace the lowest value frame byte
		> Free Frame Vector local to OSS, place in shared memory
		> Free Frame Vector is a 32 integer array, is 0 or 1
		> Once Free Vector is full, do replacement algorith
			> Max is 256k
			> 8 bits per frame
		> Allocate memory for shared data structures: ie page tables
			page table uses fixed size arrays 
			each child will require less than 32k memory, each page 1k
		> Use semaphore on the logical clock
		> Assume max memory of 256k
		

	USER
		> Process size is random
		> At random times requests a frame for a new page table, if memory space and frame available, occupies the available frame and adds to 
		  memory the size of the page.  Capacity = 256k memory and thre are 32 frame positions.  
