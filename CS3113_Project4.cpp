//alan vo 
//ou spring 2025 
//operating systems - project 4
//simulates segment tables and processes occupying non-contiguous memory

#include <iostream>
#include <queue>
#include <string>
#include <list> 
using namespace std;

int cpuClock = 0; //global variable to keep track of the CPU clock

//process metadata, containing segment table
struct PCB {
	int processID; //unique identifier for the process
	int state; // tracks current status of the process: NEW, READY, RUNNING, or TERMINATED.
	int programCounter; //index of the next instruction to be executed within the process's logical memory
	int instructionBase; //starting address of the instructions in the logical memory
	int dataBase; //starting address of data segment within the logical memory.
	int memoryLimit; //the total size of logical memory allocated to the process
	int cpuCyclesUsed; //accumulates the total CPU cycles consumed by the process during its execution
	int registerValue; //simulated register used to store intermediate values during load and store operations
	int maxMemoryNeeded; //maximum memory required by the process as defined in the input file
	int mainMemoryBase; //denotes the starting address in main memory where the process, including its PCB and logical memory, is loaded
	int numInstructions; //number of instructions associated with the process
	vector <int> encodedInstructions; //stores encoded opcode instructions for the process. I was inspired by chatGPT to use this as a way to store the opcodes.
	vector <int> data; //stores the data associated with each process (iterations, cycles, values, and addresses)
	vector <int> logicalMemory; //contains segment table entries, instructions, and data with logical addressing
};

//linked list nodes represent memory blocks in main memory 
struct memoryBlock {
	int processID; //ID of the process occupying the block (-1 if free)
	int startingAddress; //memory location in mainMemory vector where block begins
	int size; //size of memory block accounting for 10 pcb fields and segment table
};

//procedure for checking whether there are processes in IOWaitingQueue that are ready to be transferred back to readyQueue
void checkIOWaitingQueue(queue<int>& IOWaitingQueue, queue<int>& readyQueue, vector <int>& mainMemory) {
	//create temporary queue to store processes that will be pushed back to the waiting queue (processes that have not completed IO operation)
	queue<int> temporary;

	//if IOWaitingQueue contains multiple processes, we must check ALL processes
	while (!IOWaitingQueue.empty()) {
		int timeProcessEntered, mainMemoryBase, dataBasePosition, printCycles, processID; //variables storing information about processes in IOWaitingQueue

		timeProcessEntered = IOWaitingQueue.front(); //get cpuClock of when process entered the IOWaitingQueue
		IOWaitingQueue.pop(); //remove element
		mainMemoryBase = IOWaitingQueue.front(); //get starting location of process in mainMemory
		IOWaitingQueue.pop(); //remove element
		dataBasePosition = IOWaitingQueue.front(); //get location of current data in process's logical memory
		IOWaitingQueue.pop(); //remove element
		printCycles = IOWaitingQueue.front(); //get number of CPU cycles required for the process to complete the print IO operation
		IOWaitingQueue.pop(); //remove element
		processID = IOWaitingQueue.front();
		IOWaitingQueue.pop();
		/*As Clark said, it is assumed that the time a process spends in I/O waiting is the time it is taking to print.
		In other words, the print is happening simultaneously while some other process executes*/

		//check to see if process has been in IOWaitingQueue for enough CPU cycles (completed print operation)
		if (cpuClock - timeProcessEntered >= printCycles) {
			readyQueue.push(mainMemoryBase); //push starting location of process in mainMemory to the back of readyQueue
			readyQueue.push(dataBasePosition); //push location of current data to the back of readyQueue
			cout << "print" << endl; //print statement occurs after process has waited long enough in IOWaitingQueue
			cout << "Process " << processID << " completed I/O and is moved to the ReadyQueue." << endl;
		}
		else {
			//otherwise, if processes are still waiting for the print operation, push them back to the temporary queue in the same order (FIFO)
			temporary.push(timeProcessEntered); //push timeProcessEntered to temporary queue
			temporary.push(mainMemoryBase); //push mainMemoryBase back to temporary queue
			temporary.push(dataBasePosition); //push dataBaseIndex back to temporary queue
			temporary.push(printCycles); //push number of cpu cycles back to temporary queue
			temporary.push(processID);
		}
	}
	//push unready processes back to IOWaitingQueue
	while (!temporary.empty()) {
		int processInfo = temporary.front(); //important variables related to processes in IOWaitingQueue
		temporary.pop(); //remove element
		IOWaitingQueue.push(processInfo); //push back to IOWaitingQueue
	}
}

/*This procedure copies the logical contents of a process (including its segment table,
 PCB fields, instructions, and data) into the allocated segments as listed in the
segment table. The segment table is assumed to begin at logical address 0, and the logical layout is
linear.*/
void copyProcessToMemory(PCB& process, vector<int>& mainMemory, int totalLogicalSize) {
	vector <int> processLogicalMemory = process.logicalMemory; //get logical memory vector of each process from its PCB
	int segmentTableSize = processLogicalMemory.at(0); //2 times number of segments
	int numSegments = segmentTableSize / 2;
	int logicalIndex = 0;
	for (int i = 0; i < numSegments; i++) {
		int start = processLogicalMemory[1 + 2 * i]; // physical starting address of segment i
		int size = processLogicalMemory[1 + 2 * i + 1]; // size of segment i
		for (int j = 0; j < size && logicalIndex < totalLogicalSize; j++) {
			mainMemory.at(start + j) = processLogicalMemory[logicalIndex];
			logicalIndex++;
		}
	}
	if (logicalIndex > totalLogicalSize) {
		cout << "Error: not enough space in allocated segments to hold process." << endl;
	}
}

//function that translates a logical address to physical address using segment table
int translateLogicalToPhysical(int logicalAddress, vector<int> segmentTable) {
	int segmentTableSize = segmentTable.at(0);
	int numSegments = segmentTableSize / 2;
	int remaining = logicalAddress;
	for (int i = 0; i < numSegments; i++) {
		int start = segmentTable[1 + 2 * i];
		int length = segmentTable[1 + 2 * i + 1];
		if (remaining < length) {
			return start + remaining;
		}
		else {
			remaining -= length;
		}
	}
	cout << "Memory violation: address " << logicalAddress << " out of bounds." << endl;
	return -1;
}

//procedure performing memory coalescing (combining adjacent free memory blocks)
void triggerMemoryCoalescing(list<memoryBlock>& memoryTracker) {
	//scan linked list checking if current and next/adjacent node are both free (for each memory block node in list)
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//if current node and next node are both free, then perform memory coalescing 
		while (next(it, 1) != memoryTracker.end() && it->processID == -1 && next(it, 1)->processID == -1) {
			it->size += next(it, 1)->size; //current node becomes merged free memory block accounting for pcb fields
			next(it, 1) = memoryTracker.erase(next(it, 1)); //delete next node and have it point to the node after the deleted node
		}
	}
}

//procedure for handling job termination and marking mainMemory vector locations as free (-1)
void releaseMemoryBlock(list<memoryBlock>& memoryTracker, int processID, vector<int>& mainMemory) {
	//find memory block that was occupied by process by iterating through linked list 
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//after finding memory block occupied by the process, update memoryBlock's process ID to -1 (free)
		if (it->processID == processID) {
			int endingAddress = (it->startingAddress + it->size) - 1; //ending address of process, accounting for pcb fields and vector indexing 
			it->processID = -1; //update memoryBlock process id to indicate it is now free (-1)

			//free main memory vector locations (set all to -1) that process occupied
			for (int startingAddress = it->startingAddress; startingAddress <= endingAddress; startingAddress++) {
				mainMemory.at(startingAddress) = -1;//update main memory vector locations to -1 to indicate free memory
			}
		}
	}
	cout << "Process " << processID << " terminated and freed memory blocks." << endl;
}

//function returns a queue containing iterators to sufficient memory blocks used in process allocation (I got the idea to use a queue from Cory)
queue<list<memoryBlock>::iterator> checkAvailableMemory(list<memoryBlock>& memoryTracker, int processSize, int processID) {
	int remainingSize = processSize; //remaining size that we still need to allocate for process 
	bool spaceForTable = false; //indicates if we were able to find a contiguous block with size >= 13 for segment table
	queue<list<memoryBlock>::iterator> segmentQueue; //store iterators pointing to memory blocks that can be used for allocation 
	list<memoryBlock>::iterator firstSegment; //stores iterator to first block with size >= 13 that can hold the segment table (inspired by Clark) 

	//first, check for contiguous block >= 13 for segment table
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		if (it->processID == -1 && it->size >= 13) {
			spaceForTable = true;
			firstSegment = it; //get first segment with block of size at least 13 (containing the segment table)
			break; //after finding this segment we can break
		}
	}
	//if no contiguous segment of size >= 13 for segment table was found
	if (spaceForTable == false) {
		cout << "Process " << processID << " could not be loaded due to insufficient contiguous space for segment table." << endl;
		return segmentQueue; //return empty queue, indicating no segments could be allocated 
	}
	else {
		//attempt to allocate process segments starting from the first segment (idea from Clark)
		while (firstSegment != memoryTracker.end() && segmentQueue.size() < 6 && remainingSize > 0) {
			if (firstSegment->processID == -1) {
				//if block size greater than or equal to process size, we have enough space
				if (firstSegment->size >= remainingSize) {
					remainingSize -= firstSegment->size; //end loop because process can fit perfectly into block
					segmentQueue.push(firstSegment); //add iterator of block to queue for later use
					return segmentQueue; //we have our allocated block!
				}
				//if block size smaller than process size, keep checking to see if there is sufficient size for process within 6 segments
				else {
					remainingSize -= firstSegment->size; //update remaining process size still needed for allocation
					segmentQueue.push(firstSegment); //add segment to queue 
				}
			}
			++firstSegment; //move on to next memory block
		}
		//after getting out of loop, if we still have memory to allocate for process, it means there was not enough space for process 
		if (remainingSize > 0) {
			cout << "Insufficient memory for Process " << processID << ". Attempting memory coalescing." << endl;
			cout << "Process " << processID << " waiting in NewJobQueue due to insufficient memory." << endl;
			//clear the queue to indicate no segments can be allocated
			while (!segmentQueue.empty()) {
				segmentQueue.pop();
			}
			return segmentQueue; //return empty queue (no segments can be allocated)
		}
		return segmentQueue; //if we reach down here, we can return the sufficient memory blocks for process allocation
	}
}


//procedure allocates segments after finding enough memory
void allocateMemoryBlocks(list<memoryBlock>& memoryTracker, int processID, int processSizeNeeded, PCB& process, queue<list<memoryBlock>::iterator>& segmentQueue) {
	memoryBlock nextFreeBlock; //node representing next free block of memory
	int freeMemorySize; //original free memory block size used in splitting
	int segmentTableSize, segmentPhysicalStart, segmentSize; //used to add to process's logical memory vector in PCB
	int remainingProcessSize = processSizeNeeded; //total memory needed for process
	cout << "Process " << processID << " loaded with segment table stored at physical address " << segmentQueue.front()->startingAddress << endl;

	//using queue of iterators, begin allocating in the same order we found them (FIFO) 
	while (!segmentQueue.empty() && remainingProcessSize > 0) {
		list<memoryBlock>::iterator block = segmentQueue.front(); //first block in front of queue
		freeMemorySize = block->size; //get original free size of memory block
		block->processID = processID; //block now occupied by this process

		//if block size less than or equal to process size needed, we use whole block
		if (block->size <= remainingProcessSize) {
			remainingProcessSize -= block->size; //update remaining memory needed for process
		}
		//if memory block is bigger than process size, we need to split current node and insert a new free block node 
		else {
			block->size = remainingProcessSize; //assign memory block the size of process
			//create a new free memory block and insert it right after in the linked list
			nextFreeBlock.processID = -1; //indicate it is free
			nextFreeBlock.startingAddress = block->startingAddress + remainingProcessSize; //starting address of next free memory block
			nextFreeBlock.size = freeMemorySize - remainingProcessSize; //new free block size (original size - memory block occupied by process)
			memoryTracker.insert(next(block, 1), nextFreeBlock);//insert new free block node after occupied memory block
			processSizeNeeded -= block->size; //update so we can end loop
		}
		//add segment table information to process's logical memory vector 
		segmentPhysicalStart = block->startingAddress; //starting address of segment in physical memory 
		segmentSize = block->size; //size of segment 
		(process.logicalMemory).push_back(segmentPhysicalStart);
		(process.logicalMemory).push_back(segmentSize);
		segmentQueue.pop(); //pop so we can move on to next segment in queue
	}
	//after allocating the segments, add segment table size at beginning of process's logical memory vector
	segmentTableSize = process.logicalMemory.size(); //get size of segment table (number of entries so we can store in segmentTableSize variable)
	(process.logicalMemory).insert(process.logicalMemory.begin(), segmentTableSize);//insert segment table size into index 0 of logical memory vector
}


//procedure for managing jobs being dynamically loaded into available memory 
void loadJobsToMemory(queue<PCB>& NewJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, list<memoryBlock>& memoryTracker) {
	triggerMemoryCoalescing(memoryTracker); //perform memory coalescing before searching for space
	//while there is available memory, keep loading processes 
	while (!NewJobQueue.empty()) {
		PCB process = NewJobQueue.front(); //get current process from NewJobQueue
		int processID = process.processID; //get process id of current process (used in linked list to allocate memory)  
		int processSize = process.memoryLimit + 23; //size that process occupies in main memory (accounting for 10 pcb fields and segment table)
		queue <list<memoryBlock>::iterator> allocatedSegmentQueue; //create a queue storing iterators to blocks in the linked list 
		allocatedSegmentQueue = checkAvailableMemory(memoryTracker, processSize, processID); //call function to see if segments can be allocated to process

		//if process can fit within 6 segments, allocate memory 
		if (allocatedSegmentQueue.size() > 0 && allocatedSegmentQueue.size() <= 6) {
			allocateMemoryBlocks(memoryTracker, processID, processSize, process, allocatedSegmentQueue); //allocate memory blocks using queue
			int N = process.logicalMemory.at(0) + 1; //segment table size + 1 (get process id in logical memory )
			process.mainMemoryBase = process.logicalMemory.at(1); //physical starting address of segment 0 
			process.instructionBase = N + 10; //logical index in process's memory marking starting address of instructions
			process.dataBase = process.instructionBase + process.numInstructions; //instructionBase + numInstructions

			//add 10 pcb fields to process's logical memory vector
			process.logicalMemory.push_back(processID);
			process.logicalMemory.push_back(process.state);
			process.logicalMemory.push_back(process.programCounter);
			process.logicalMemory.push_back(process.instructionBase);
			process.logicalMemory.push_back(process.dataBase);
			process.logicalMemory.push_back(process.memoryLimit);
			process.logicalMemory.push_back(process.cpuCyclesUsed);
			process.logicalMemory.push_back(process.registerValue);
			process.logicalMemory.push_back(process.maxMemoryNeeded);
			process.logicalMemory.push_back(process.mainMemoryBase);
			//add opcode instructions
			for (int i = 0; i < process.encodedInstructions.size(); i++) {
				process.logicalMemory.push_back(process.encodedInstructions.at(i));
			}
			//add data 
			for (int j = 0; j < process.data.size(); j++) {
				process.logicalMemory.push_back(process.data.at(j));
			}
			//add remaining logical memory of process (-1)
			for (int k = process.logicalMemory.size(); k < processSize; k++) {
				process.logicalMemory.push_back(-1);
			}

			//copy logical contents of process into allocated segments in main memory 
			copyProcessToMemory(process, mainMemory, processSize);

			//push process to readyQueue
			process.state = 2;//set process state to READY
			readyQueue.push(process.mainMemoryBase); //push the physical starting address of process to the ready queue for CPU execution 
			readyQueue.push(process.dataBase); //also push the logical address of database, which will be translated later
			NewJobQueue.pop(); //pop newJobQueue and get next process	
		}
		else {
			break;//otherwise break out of loop and trigger memory coalescing
		}
	}
}

//procedure simulating the CPU executing processes
void executeCPU(int startAddress, vector<int>& mainMemory, int CPUAllocated, queue<int>& readyQueue, int dataPointer, int contextSwitch,
	queue<int>& IOWaitingQueue, vector<int>& startTimes, list <memoryBlock>& memoryTracker, queue<PCB>& NewJobQueue, int maxMemory) {

	int startTime, terminatedTime, executionTime; //store the time when process first began execution, terminated, and its total execution time
	int opcode; //used to execute decoded instructions (compute, print, store, load)
	int iterations, cpuCycles, value, address; //data variables associated with the operations

	//create segment table from main memory
	int segmentTableSize = mainMemory.at(startAddress); //using starting address of segment 0 (main memory base)
	int numSegments = segmentTableSize / 2;
	vector <int> segmentTable; //create a vector to store segment table used for address translation
	//populate vector with segment table info
	segmentTable.push_back(segmentTableSize);
	for (int i = 1; i <= segmentTableSize; i++) {
		segmentTable.push_back(mainMemory.at(startAddress + i));
	}
	int N = segmentTableSize + 1; //first PCB entry after segment table entries in logical memory

	//translate logical address to physical addresses using segment table
	//using the start address of process, go through mainMemory and assign PCB details to corresponding variables 
	int processID = mainMemory[translateLogicalToPhysical(N, segmentTable)];
	int state = mainMemory[translateLogicalToPhysical(N + 1, segmentTable)];
	int programCounter = mainMemory[translateLogicalToPhysical(N + 2, segmentTable)]; //will be updated in mainMemory as we traverse through instructions
	int instructionBase = mainMemory[translateLogicalToPhysical(N + 3, segmentTable)];
	int dataBase = mainMemory[translateLogicalToPhysical(N + 4, segmentTable)];
	int memoryLimit = mainMemory[translateLogicalToPhysical(N + 5, segmentTable)];
	int cpuCyclesUsed = mainMemory[translateLogicalToPhysical(N + 6, segmentTable)];
	int registerValue = mainMemory[translateLogicalToPhysical(N + 7, segmentTable)];
	int maxMemoryNeeded = mainMemory[translateLogicalToPhysical(N + 8, segmentTable)];
	int mainMemoryBase = mainMemory[translateLogicalToPhysical(N + 9, segmentTable)];

	cout << "Process " << processID << " has moved to Running." << endl; //print out what process is running 
	cpuClock += contextSwitch; //context switch happens when process moves from READY to RUNNING state

	//programCounter = 0, means it is the first time the process is executed
	if (programCounter == 0) {
		programCounter = instructionBase; //get location where first opcode of process is located
		startTime = cpuClock; //get the time when the process first entered the running state
		startTimes.insert(startTimes.begin() + processID, startTime); //insert the starting cpu clock of process in vector with its processID as index		
	}
	//otherwise programCounter (next opcode) is loaded from saved state of process in mainMemory

	int dataBaseIndex = dataPointer; //used to traverse through data section in main memory in process's logical memory 
	int dataValue = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)]; //get current data value of process using dataBaseIndex (used as end condition for while loop)
	int cpuCounter = 0; //to account for timeout interrupts

	/*If a process runs for more than CPUAllocated ticks without terminating or issuing an I/O
	operation (e.g., a print statement), it will be interrupted. The process is then moved to the back of the
	ReadyQueue, and a message is printed indicating a TimeOUT Interrupt.*/

	opcode = mainMemory[translateLogicalToPhysical(programCounter, segmentTable)]; //get first opcode of process using program counter
	//while process still has enough CPU ticks and there is still data in mainMemory, keep executing process
	while (programCounter < dataBase) {
		//use a switch to determine what instruction to execute
		switch (opcode) {
		case 1: //compute operation 
			cout << "compute" << endl; //output operation 
			//go to data base and get iterations and cycles
			iterations = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)];//get number of iterations 
			cpuCycles = mainMemory[translateLogicalToPhysical((dataBaseIndex + 1), segmentTable)];//get number of cpu cycles

			dataBaseIndex += 2; //update dataBaseIndex so we can move on to next data
			cpuCyclesUsed += cpuCycles; //update the number of cpu cycles to output later
			cpuCounter += cpuCycles; //update the number of cpu cycles used by the process so that we can check for timeout interrupts
			cpuClock += cpuCycles;//update number of cpu ticks consumed by instruction
			programCounter++; //update programCounter to move to next opcode
			break;
		case 2: //print instruction (i/o interrupt)
			cpuCycles = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)]; //read in number of cpu cycles useds

			dataBaseIndex++; //update dataBaseIndex so that we can move on to other data 
			cpuCyclesUsed += cpuCycles; //update number of cpu cycles
			cpuCounter += cpuCycles; //update the number of cpu cycles used by the process so that we can check for time out interrupts
			programCounter++; //update program counter to move to next opcode


			//save state of current process in mainMemory before moving it to IOWaitingQueue
			mainMemory[translateLogicalToPhysical(N + 2, segmentTable)] = programCounter; //update program counter so we can resume at next opcode (instruction) when process is reloaded
			mainMemory[translateLogicalToPhysical(N + 6, segmentTable)] = cpuCyclesUsed; //update cpuCyclesUsed in mainMemory
			mainMemory[translateLogicalToPhysical(N + 7, segmentTable)] = registerValue; //update registerValues in mainMemory

			//send process to IOWaitingQueue
			IOWaitingQueue.push(cpuClock); //record CPU clock time when process was moved to IOWaitingQueue 
			IOWaitingQueue.push(mainMemoryBase); //push starting address of process to IOWaitingQueue
			IOWaitingQueue.push(dataBaseIndex); //push interrupted process's current data pointer to back of IOWaitingQueue 
			IOWaitingQueue.push(cpuCycles); //push number of cpu cycles required to complete print operation in IOWaitingQueue 
			IOWaitingQueue.push(processID); //for output
			cout << "Process " << processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl; //issue io interrupt 

			//since we have io interrupt, check if there are any processes in IOWaitingQueue that need to be tranfered back to readyQueue
			if (!IOWaitingQueue.empty()) {
				checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
			}
			return; //exit this process (context switch) and execute next process in readyQueue
			break;
		case 3: //store instruction
			value = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)];//read in value 
			address = mainMemory[translateLogicalToPhysical(dataBaseIndex + 1, segmentTable)];//read in address

			//if the array location is within bounds say stored, otherwise say store error!
			if (address > memoryLimit) {
				cout << "store error!" << endl;
				registerValue = value;//update register value 

			}
			else {
				cout << "stored" << endl;
				cout << "Logical address " << address << " translated to physical address " << translateLogicalToPhysical(address, segmentTable) << " for Process " << processID << endl;
				registerValue = value; //update simulated register value  
				mainMemory[translateLogicalToPhysical(address, segmentTable)] = registerValue; //account for process's logical memory as shown by Xavier
			}
			dataBaseIndex += 2; //update dataBaseIndex to move on to other data
			cpuCyclesUsed++; //update cpu cycles - store takes 1 cpu time
			cpuCounter++; //update the number of cpu cycles used by the process so that we can check for interrupts
			cpuClock++; //update number of cpu ticks consumed by instruction
			programCounter++; //update programCounter to move to next opcode
			break;
		case 4: //load instruction
			address = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)]; //get address of where value is being retrieved
			//if array location is within bounds say loaded, otherwise say load error!
			if (address >= memoryLimit) {
				cout << "load error!" << endl;
			}
			else {
				cout << "loaded" << endl;
				cout << "Logical address " << address << " translated to physical address " << translateLogicalToPhysical(address, segmentTable) << " for Process " << processID << endl;
				registerValue = mainMemory[translateLogicalToPhysical(address, segmentTable)]; //account for process's logical memory as shown by Xavier

			}
			dataBaseIndex++;//update dataBaseIndex so we can move to other data
			cpuCyclesUsed++;//update cpu cycles - load takes 1 cpu time 
			cpuCounter++; //update the number of cpu cycles used by the process so that we can check for interrupts
			cpuClock++; //update number of cpu ticks consumed by instruction
			programCounter++; //update programCounter to move to next opcode
			break;
		default:
			break;
		}

		//If a process runs for more than CPUAllocated ticks without terminating or issuing an I/O operation (print), it will be interrupted
		if (cpuCounter >= CPUAllocated) {
			//if process is about to terminate do not interrupt (data value of -1 means there is no more data in data segment)
			if (mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)] == -1) {
				break;
			}
			else {
				cout << "Process " << processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl; //output message

				//update the PCB fields in mainMemory to save state of process
				mainMemory[translateLogicalToPhysical(N + 2, segmentTable)] = programCounter;//update program counter so we can resume at next opcode when process is reloaded
				mainMemory[translateLogicalToPhysical(N + 6, segmentTable)] = cpuCyclesUsed;//update cpuCyclesUsed in mainMemory
				mainMemory[translateLogicalToPhysical(N + 7, segmentTable)] = registerValue;//update registerValues in mainMemory

				readyQueue.push(mainMemoryBase); //push starting address of process to back of readyQueue
				readyQueue.push(dataBaseIndex); //push interrupted process's data pointer to back of ready queue after storing its state

				//The IOWaitingQueue is checked for job transfers every time an interrupt occurs in the CPU	
				if (!IOWaitingQueue.empty()) {
					checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
				}
				return; //exit this process and execute the next process in the readyQueue
			}

		}
		opcode = mainMemory[translateLogicalToPhysical(programCounter, segmentTable)]; //get next opcode in the process using programCounter
		dataValue = mainMemory[translateLogicalToPhysical(dataBaseIndex, segmentTable)]; //get next data value
	}

	startTime = startTimes.at(processID); //get starting time of process from vector using its processID as index
	terminatedTime = cpuClock; //get current cpu time when process terminated 
	executionTime = terminatedTime - startTime; //calculate total execution time
	programCounter = instructionBase - 1; //once the process is done executing, reset the programCounter to the address directly before instructionBase

	//Once a job is completed by the CPU output the details of the process
	cout << "Process ID: " << processID << endl;
	cout << "State: TERMINATED" << endl;//print out state
	cout << "Program Counter: " << programCounter << endl;
	cout << "Instruction Base: " << instructionBase << endl;
	cout << "Data Base: " << dataBase << endl;
	cout << "Memory Limit: " << memoryLimit << endl;
	cout << "CPU Cycles Used: " << cpuCyclesUsed << endl;
	cout << "Register Value: " << registerValue << endl;
	cout << "Max Memory Needed: " << maxMemoryNeeded << endl;
	cout << "Main Memory Base: " << mainMemoryBase << endl;
	cout << "Total CPU Cycles Consumed: " << executionTime << endl;
	cout << "Process " << processID << " terminated. Entered running state at: " << startTime << ". " << "Terminated at: " << terminatedTime << ". Total Execution Time: " << executionTime << "." << endl;
	releaseMemoryBlock(memoryTracker, processID, mainMemory);//process releases memory block it occupied in main memory and linked list nodes are updated

	//check if there are still processes in NewJobQueue waiting to be loaded into memory
	if (!NewJobQueue.empty()) {
		PCB process = NewJobQueue.front(); //get waiting process
		int processID = process.processID; //process id of current process used in linked list  
		int processSize = process.memoryLimit + 23; //number of vector locations that the process occupies, including 10 pcb fields and segment table entries
		loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, memoryTracker);
	}
	//check if there are any processes in waiting queue that need to be tranfered to readyQueue
	if (!IOWaitingQueue.empty()) {
		checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
	}
}

int main() {
	int maxMemory, numProcesses; //maximum size of main memory & total number of processes
	queue<PCB> NewJobQueue; //stores segment table, instructions, and data 
	queue<int> readyQueue; //stores the starting address of each process in main memory and data pointer
	queue<int> IOWaitingQueue; //queue to store information about processes waiting for I/O operations to complete
	PCB newProcess; //used to store information for each process added to the NewJobQueue

	//Implement input parsing
	int processID, maxMemoryNeeded, numInstructions; //PCB variables
	int CPUAllocated; //specifies the number of CPU ticks a process can execute before it gives TimeOUT interrupt
	int contextSwitch; //specifies the number of CPU ticks required to perform a context switch (move a job from readyQueue to running state)

	cin >> maxMemory; //read in the first number specifying the maximum size of main memory
	vector <int> mainMemory(maxMemory, -1); //dynamically create vector with max memory size and default values of -1
	list<memoryBlock> memoryBlockTracker; //main memory is tracked using linked list, with each node representing a memory block

	//create a memoryBlock node that represents single large free block of memory initially
	memoryBlock initialMemoryBlock;
	initialMemoryBlock.processID = -1; //initially, there is no process in main memory vector
	initialMemoryBlock.startingAddress = 0; //the vector location where this block begins is at index 0, since nothing is stored yet
	initialMemoryBlock.size = maxMemory; //since we have the whole block free at the beginning 
	memoryBlockTracker.push_front(initialMemoryBlock);//add this node at the beginning (front) of the linked list

	cin >> CPUAllocated; //read in number of ticks that each process can execute before timing out 
	cin >> contextSwitch; //read in number of ticks required to perform a context switch
	cin >> numProcesses; //read in total number of processes
	vector<int> startingTimes(numProcesses, 0); //vector to store the cpu clock time each process when they first get executed

	//populate newJobQueue
	for (int i = 0; i < numProcesses; i++) {
		cin >> processID; //get processID 
		cin >> maxMemoryNeeded; //get max memory needed for the process
		cin >> numInstructions; //get number of instructions in process

		//fill out 10 PCB data fields
		newProcess.processID = processID; //assign processID to PCB variable 
		newProcess.state = 1; //set initial state of process to NEW
		newProcess.programCounter = 0;//set program counter to 0
		newProcess.instructionBase = 0; //array index where instructions start in main memory (initially set to 0 and update when loading into main memory)
		newProcess.dataBase = numInstructions + newProcess.instructionBase; //number of instructions + mainMemoryBase
		newProcess.memoryLimit = maxMemoryNeeded; //assign the amount of memory needed by the process as read in from the input file
		newProcess.cpuCyclesUsed = 0; //initially set to 0 and update later as the CPU executes the instructions
		newProcess.registerValue = 0; //initially set to 0 for the register value and update later when CPU executes
		newProcess.maxMemoryNeeded = maxMemoryNeeded; // assign amount of memory required for the process to PCB variable 
		newProcess.mainMemoryBase = 0; //initially set this to 0 and update later after loading into memory
		newProcess.numInstructions = numInstructions; //assign numInstructions to use later in determining dataBase 

		int opcode, iterations, cpuCycles, value, address; //opcode and data variables
		vector <int> instructions; //used to store the instruction opcodes of each process
		vector <int> data; //used to store the data (iterations, cycles, addresses, etc)
		cin >> opcode; //read the next integer to see first opcode

		//use for loop to store instructions and data into their respective vectors
		for (int i = 0; i < numInstructions; i++) {
			//switch to determine how many numbers to read 
			switch (opcode) {
			case 1:
				//compute instruction, so we have to read two more numbers after 
				cin >> iterations;
				cin >> cpuCycles;
				//store opcode data in instruction vector
				instructions.push_back(opcode); //store in instructions vector
				data.push_back(iterations); //store in data vector
				data.push_back(cpuCycles); //store in data vector
				break;
			case 2:
				//print instruction so just read one number after 
				cin >> cpuCycles;
				//store opcode and data in their vectors
				instructions.push_back(opcode);
				data.push_back(cpuCycles);
				break;
			case 3:
				//store instruction so we have to read two more numbers after 
				cin >> value;
				cin >> address;
				//store opcode and data in their vectors
				instructions.push_back(opcode);
				data.push_back(value);
				data.push_back(address);
				break;
			case 4:
				//load instruction so just read one more number after 
				cin >> address;
				//store opcode and data in their respective vectors
				instructions.push_back(opcode);
				data.push_back(address);
				break;
			default:
				break;
			}
			//on the last instruction of each process, do not read the next element in the input file because that is the next process
			if (i < numInstructions - 1) {
				cin >> opcode; //read the next number to see what the next encoded instruction should be  
			}
			newProcess.encodedInstructions = instructions; //store opcode instructions into vector in PCB
			newProcess.data = data; //store data instructions into data vector in PCB
		}
		//populate NewJobQueue with process metadata
		NewJobQueue.push(newProcess);
	}
	//while there are processes in NewJobQueue, keep loading as much as possible if there is enough memory
	loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, memoryBlockTracker);

	//print out contents of main memory vector 
	for (int i = 0; i < mainMemory.size(); i++) {
		cout << i << " : " << mainMemory.at(i) << endl;
	}

	//readyQueue contains start addresses and data pointers w.r.t main memory for jobs
	//while there are still processes in NewJobQueue, readyQueue or IOWaitingQueue, keep loading and executing processes
	while (!readyQueue.empty() || !IOWaitingQueue.empty()) {
		//if ready queue is not empty, keep executing processes
		if (!readyQueue.empty()) {
			int startAddress = readyQueue.front(); //get starting address of process
			readyQueue.pop();
			int dataBaseIndex = readyQueue.front(); //get data pointer for process
			readyQueue.pop();

			//execute job, checking after each termination if there are waiting jobs in NewJobQueue
			executeCPU(startAddress, mainMemory, CPUAllocated, readyQueue, dataBaseIndex, contextSwitch, IOWaitingQueue, startingTimes,
				memoryBlockTracker, NewJobQueue, maxMemory);
		}
		//if ready queue IS empty, keep checking IOWaitingQueue for a process that has completed the IO operation
		else {
			//while readyQueue is empty, keep looking for processes in io waiting queue 
			while (readyQueue.empty()) {
				cpuClock += contextSwitch; //increment cpu clock when readyQueue is empty
				checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory); //check IOWaitingQueue
			}
		}
	}
	cpuClock += contextSwitch; //context switch out of the last process
	cout << "Total CPU time used: " << cpuClock << "." << endl; //print out the total number of CPU ticks consumed by all processes
	return 0;
}