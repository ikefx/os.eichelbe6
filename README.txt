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
			* Default values for these arguments are provided if these argv[]s are not provided

		
		> PAGE FAULT HANDER:
			OSS has a page fault handler that handles page faults.  Page faults occur when there is not enough
			memory for the next page requests.

			When a page fault occurs, OSS recognizes such from a signal from the child.  To handle the page fault,
			the OSS shifts the bits of every 'in use' page reference byte to the right.  This reduces the memory
			required of the currently in-use addresses.  The lowest value page is replaced with the new page.
			Repeats until the new page can fit in the Free Frame Vector array.
			
			OSS also executes page fault handler every 10 requests to prevent a fault occurence

		> The logical clock is wrapped in semaphore protection and increments on page fault handling
		

	USER
		> Process size is random
		> At random times requests a frame for a new page table, if memory space and frame available, occupies the available frame and adds to 
		  memory the size of the page.  Capacity = 256k memory and thre are 32 frame positions.  
