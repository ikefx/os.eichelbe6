PROJECT 06 README
NEIL EICHELBERGER

files:

OSS

USER

README

CRITERIA:

	OSS
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
		> Fork multiple childs at random times
		> Logical clock
		> Allocate memory for shared data structures: ie page tables
			page table uses fixed size arrays 
			each child will require less than 32k memory, each page 1k
		> Use a delimter to note the end of each page table
		> Use semaphore on the logical clock
		> Assume max memory of 256k
		

	USER
		> Process size is random
		> Page Table is local to USER
