//alan vo 
//ou spring 2025 
//operating systems - project 3
//simulates jobs being dynamically loaded into available memory, removed after completion, and memory coalescing using linked lists.

#include <iostream>
#include <queue>
#include <string>
#include <list> 
using namespace std;

int cpuClock = 0; //global variable to keep track of the CPU clock

//process metadata
struct PCB {
	//10 PCB fields
	int processID; //unique identifier for the process
	string state; // tracks current status of the process: NEW, READY, RUNNING, or TERMINATED.
	int programCounter; //index of the next instruction to be executed within the process's logical memory
	int instructionBase; //starting address of the instructions in the logical memory
	int dataBase; //starting address of data segment within the logical memory.
	int memoryLimit; //the total size of logical memory allocated to the process
	int cpuCyclesUsed; //accumulates the total CPU cycles consumed by the process during its execution
	int registerValue; //simulated register used to store intermediate values during load and store operations
	int maxMemoryNeeded; //maximum memory required by the process as defined in the input file
	int mainMemoryBase; //denotes the starting address in main memory where the process, including its PCB and logical memory, is loaded

	//additional PCB fields used to display output 
	int numInstructions; //number of instructions associated with the process
	vector <int> encodedInstructions; //stores encoded opcode instructions for the process. I was inspired by chatGPT to use this as a way to store the encoded instructions
	vector <int> data; //stores the data associated with each process (iterations, cycles, values, and addresses)
};

//linked list nodes represent memory blocks in main memory 
struct memoryBlock {
	int processID; //ID of the process occupying the block (-1 if free)
	int startingAddress; //memory location in mainMemory vector where block begins
	int size; //size of memory block accounting for 10 pcb fields
};

//procedure for checking whether there are processes in IOWaitingQueue that are ready to be transferred back to readyQueue
void checkIOWaitingQueue(queue<int>& IOWaitingQueue, queue<int>& readyQueue, vector <int>& mainMemory) {
	//create temporary queue to store processes that will be pushed back to the waiting queue (processes that have not completed IO operation)
	queue<int> temporary;

	//if IOWaitingQueue contains multiple processes, we must check ALL processes
	while (!IOWaitingQueue.empty()) {
		int timeProcessEntered, mainMemoryBase, dataBasePosition, printCycles; //variables storing information about processes in IOWaitingQueue

		timeProcessEntered = IOWaitingQueue.front(); //get cpuClock of when process entered the IOWaitingQueue
		IOWaitingQueue.pop(); //remove element
		mainMemoryBase = IOWaitingQueue.front(); //get starting location of process in mainMemory
		IOWaitingQueue.pop(); //remove element
		dataBasePosition = IOWaitingQueue.front(); //get location of current data in process's logical memory
		IOWaitingQueue.pop(); //remove element
		printCycles = IOWaitingQueue.front(); //get number of CPU cycles required for the process to complete the print IO operation
		IOWaitingQueue.pop(); //remove element

		/*As Clark said, it is assumed that the time a process spends in I/O waiting is the time it is taking to print.
		In other words, the print is happening simultaneously while some other process executes*/

		//check to see if process has been in IOWaitingQueue for enough CPU cycles (completed print operation)
		if (cpuClock - timeProcessEntered >= printCycles) {
			readyQueue.push(mainMemoryBase); //push starting location of process in mainMemory to the back of readyQueue
			readyQueue.push(dataBasePosition); //push location of current data to the back of readyQueue
			cout << "print" << endl; //print statement occurs after process has waited long enough in IOWaitingQueue
			cout << "Process " << mainMemory[mainMemoryBase] << " completed I/O and is moved to the ReadyQueue." << endl;
		}
		else {
			//otherwise, if processes are still waiting for the print operation, push them back to the temporary queue in the same order (FIFO)
			temporary.push(timeProcessEntered); //push timeProcessEntered to temporary queue
			temporary.push(mainMemoryBase); //push mainMemoryBase back to temporary queue
			temporary.push(dataBasePosition); //push dataBaseIndex back to temporary queue
			temporary.push(printCycles); //push number of cpu cycles back to temporary queue
		}
	}

	//push unready processes back to IOWaitingQueue
	while (!temporary.empty()) {
		int processInfo = temporary.front(); //important variables related to processes in IOWaitingQueue
		temporary.pop(); //remove element
		IOWaitingQueue.push(processInfo); //push back to IOWaitingQueue
	}
}

//function checks if process can be dynamically loaded into mainMemory, and updates linked list memory block nodes accordingly
bool assignAvailableMemory(list<memoryBlock>& memoryTracker, int processID, int processSizeNeeded) {
	memoryBlock nextFreeBlock; //node representing next free block of memory (to be inserted after occupied memory block)
	//I learned from chat gpt how to use arrow operators to access memoryBlock fields in linked list nodes
	//traverse through linked list and check for any free memory blocks with sufficient size for process 
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//if current node is free space (pid == -1) with enough size, update nodes and return true
		if (it->processID == -1 && processSizeNeeded <= it->size) {
			int freeMemorySize = it->size; //get original memory size of free block

			//update current node representing free block of memory with process that is now occupying
			it->processID = processID; //memory block is now occupied by this process 
			it->size = processSizeNeeded; //update size of memory block based on how much memory locations the process needs (including 10 pcb fields)

			//output that job has been dynamically loaded into main memory 
			cout << "Process " << processID << " loaded into memory at address " << it->startingAddress << " with size " << processSizeNeeded << "." << endl;

			//if there is any free memory block left, create new node representing it and insert into next position of the linked list
			if (freeMemorySize - processSizeNeeded > 0) {
				nextFreeBlock.processID = -1; //indicate it is free
				nextFreeBlock.startingAddress = it->startingAddress + processSizeNeeded; //starting address of next free memory block, accounting for 10 PCB fields
				nextFreeBlock.size = freeMemorySize - processSizeNeeded; //new free block size (original size - memory block occupied by process)
				memoryTracker.insert(next(it, 1), nextFreeBlock);//insert new free block node right after current node 
				return true; //indicate that process has successfully been loaded into main memory and nodes have been updated
			}
		}
	}
	return false; //return false if there is no available memory for process in mainMemory
}

//procedure for handling job termination and marking mainMemory vector locations as free (-1)
void releaseMemoryBlock(list<memoryBlock>& memoryTracker, int processID, vector<int>& mainMemory) {
	//find memory block that was occupied by process by iterating through linked list 
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//after finding memory block occupied by the process, update memoryBlock's process ID to -1 (free)
		if (it->processID == processID) {
			int endingAddress = (it->startingAddress + it->size) - 1; //ending address of process, accounting for pcb fields and vector indexing 
			it->processID = -1; //update memoryBlock process id to indicate it is now free (-1)
			cout << "Process " << processID << " terminated and released memory from " << it->startingAddress << " to " << endingAddress << "." << endl;

			//free main memory vector locations (set all to -1) that process occupied
			for (int startingAddress = it->startingAddress; startingAddress <= endingAddress; startingAddress++) {
				mainMemory[startingAddress] = -1;//update main memory vector locations to -1 to indicate free memory
			}
			break; //break out of loop after updating memory block and main memory vector
		}
	}
}

//function for checking if there is available space in memory for process with specified size 
bool checkAvailableMemory(list<memoryBlock>& memoryTracker, int processID, int processSize) {
	//traverse through linked list and check for any free memory blocks with sufficient size
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//if there is free space in memory with enough size, update nodes and return true
		if (it->processID == -1 && processSize <= it->size) {
			return true; //indicate that there is available memory for process in mainMemory
		}
	}
	return false; //return false if there is no available memory for process in mainMemory
}

//function that performs memory coalescing and returns whether there is enough space for process after coalescing
bool triggerMemoryCoalescing(list<memoryBlock>& memoryTracker, int processID, int processSize) {
	//scan linked list checking if current and next/adjacent node are both free (for each memory block node in list)
	for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
		//if current node and next node are both free, then perform memory coalescing 
		while (next(it, 1) != memoryTracker.end() && it->processID == -1 && next(it, 1)->processID == -1) {
			it->size += next(it, 1)->size; // current node becomes merged free memory block accounting for pcb fields
			next(it, 1) = memoryTracker.erase(next(it, 1)); //delete next node and have it point to the node after the deleted node
		}
	}
	//check to see if there is enough space in memory for process after coalescing
	return (checkAvailableMemory(memoryTracker, processID, processSize));
}


//procedure for managing jobs being dynamically loaded into available memory 
void loadJobsToMemory(queue<PCB>& NewJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, int maxMemory, list<memoryBlock>& memoryTracker) {
	//while there is available memory, keep loading processes 
	while (!NewJobQueue.empty()) {
		int mainMemoryBase = 0;
		PCB process = NewJobQueue.front(); //get current process in NewJobQueue
		int processID = process.processID; //process id of current process used in linked list  
		int processSize = process.memoryLimit + 10; //number of vector locations that the process occupies in main memory (accounting for 10 pcb fields)

		//if there is available memory for process in linked list, load to main memory
		if (assignAvailableMemory(memoryTracker, processID, processSize)) {
			//get mainMemoryBase of each process by using starting address of node in linked list
			for (auto it = memoryTracker.begin(); it != memoryTracker.end(); ++it) {
				//if we find the current memory block occupied by the node, get the process's starting location or main memory base
				if (it->processID == processID) {
					mainMemoryBase = it->startingAddress; //assign starting address of process to mainMemoryBase
				}
			}


			//load process to main memory vector
			int state; //variable for encoding the state (NEW, READY, RUNNING, TERMINATED)
			if (process.state == "NEW") {
				state = 1; //encode NEW as int 1
			}
			else if (process.state == "READY") {
				state = 2; //encode READY as int 2 
			}
			else if (process.state == "RUNNING") {
				state = 3; //encode RUNNING as int 3
			}
			else {
				state = 0; //encode TERMINATED as int 0
			}


			process.mainMemoryBase = mainMemoryBase; //array index where process begins in mainMemory 
			process.instructionBase = mainMemoryBase + 10; //array index marking starting address of instructions
			process.dataBase = process.instructionBase + process.numInstructions; // instructionBase + numInstructions

			//start adding the 10 PCB data fields into main memory 
			mainMemory[mainMemoryBase] = process.processID;
			mainMemory[mainMemoryBase + 1] = state;
			mainMemory[mainMemoryBase + 2] = process.programCounter;
			mainMemory[mainMemoryBase + 3] = process.instructionBase;
			mainMemory[mainMemoryBase + 4] = process.dataBase;
			mainMemory[mainMemoryBase + 5] = process.memoryLimit;
			mainMemory[mainMemoryBase + 6] = process.cpuCyclesUsed;
			mainMemory[mainMemoryBase + 7] = process.registerValue;
			mainMemory[mainMemoryBase + 8] = process.maxMemoryNeeded;
			mainMemory[mainMemoryBase + 9] = process.mainMemoryBase;


			/*Once these addresses are calculated, the process's logical memory,
			which includes both the instructions and the data, is copied into the designated
			space in main memory*/

			int opcode; //integer determining what instruction is performed (compute, store, print, load)
			int instructionData; //data variables associated with the process
			int instructionBase = process.instructionBase; //get starting address of instructions
			vector<int> instructions = process.encodedInstructions; //create new vector containing the opcode
			vector<int> data = process.data; //create new vector containing the data (iterations, cycles, etc)

			//use a for loop to load encoded opcodes from instruction vector into mainMemory
			for (int i = 0; i < instructions.size(); i++) {
				opcode = instructions.at(i); //get opcode
				mainMemory[instructionBase] = opcode; //load this opcode in mainMemory at instructionBase index
				instructionBase++; //update to store rest of opcodes sequentially in vector
			}

			//use another for loop to load data from data vector into mainMemory
			for (int i = 0; i < data.size(); i++) {
				instructionData = data.at(i); //get data int
				mainMemory[instructionBase] = instructionData;//store it in mainMemory
				instructionBase++;//update instructionBase to store rest of instruction data sequentially in vector
			}

			//push process to readyQueue
			process.state = "READY";//set process state to READY
			readyQueue.push(mainMemoryBase); //push the starting point of each process in main memory to the ready queue for CPU execution 
			readyQueue.push(process.dataBase); //also push the data pointer of the process 
			NewJobQueue.pop(); //pop newJobQueue and get next process	
		}
		else {
			break;//otherwise break out of loop and trigger memory coalescing
		}

	}

	//trigger memory coalescing if there is no available memory for process in NewJobQueue
	if (!NewJobQueue.empty()) {
		PCB process = NewJobQueue.front(); //get waiting process
		int processID = process.processID; //process id of current process used in linked list  
		int processSize = process.memoryLimit + 10; //number of vector locations that the process occupies, including 10 pcb fields
		//trigger memory coalescsing 
		cout << "Insufficient memory for Process " << processID << ". Attempting memory coalescing." << endl;

		//if memory coalescing successful and process can be loaded, load process
		if (triggerMemoryCoalescing(memoryTracker, processID, processSize)) {
			cout << "Memory coalesced. Process " << processID << " can now be loaded." << endl;
			loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, maxMemory, memoryTracker);
		}
		else {
			//otherwise, if there is no available memory, process stays in the NewJobQueue 
			cout << "Process " << processID << " waiting in NewJobQueue due to insufficient memory." << endl;
		}
	}

}

//procedure simulating the CPU executing processes
void executeCPU(int startAddress, vector<int>& mainMemory, int CPUAllocated, queue<int>& readyQueue,
	int dataPointer, int contextSwitch, queue<int>& IOWaitingQueue, vector<int>& startTimes, list <memoryBlock>& memoryTracker,
	queue<PCB>& NewJobQueue, int maxMemory) {

	int startTime, terminatedTime, executionTime; //store the time when process first entered running state, terminated, and its total execution time
	int opcode; //used to execute decoded instructions (compute, print, store, load)
	int iterations, cpuCycles, value, address; //data variables associated with the operations

	//using the start address of process, go through mainMemory and assign PCB details to corresponding variables 
	int processID = mainMemory[startAddress];
	int state = mainMemory[startAddress + 1];
	int programCounter = mainMemory[startAddress + 2]; //will be updated in mainMemory as we traverse through instructions
	int instructionBase = mainMemory[startAddress + 3];
	int dataBase = mainMemory[startAddress + 4];
	int memoryLimit = mainMemory[startAddress + 5];
	int cpuCyclesUsed = mainMemory[startAddress + 6];
	int registerValue = mainMemory[startAddress + 7];
	int maxMemoryNeeded = mainMemory[startAddress + 8];
	int mainMemoryBase = mainMemory[startAddress + 9];

	cout << "Process " << processID << " has moved to Running." << endl; //print out what process is running 
	cpuClock += contextSwitch; //context switch happens when process moves from READY to RUNNING state

	//programCounter = 0, means it is the first time the process is executed
	if (programCounter == 0) {
		programCounter = instructionBase; //get location where first opcode of process is located
		startTime = cpuClock; //get the time when the process first entered the running state
		startTimes.insert(startTimes.begin() + processID, startTime); //insert the starting cpu clock of process in vector with its processID as index		
	}
	//otherwise programCounter (next opcode) is loaded from saved state of process in mainMemory


	int dataBaseIndex = dataPointer; //used to traverse through data section in main memory (inspired by Clark)
	int dataValue = mainMemory[dataBaseIndex]; //get current data value of process using dataBaseIndex (used as end condition for while loop)
	int cpuCounter = 0; //to account for timeout interrupts


	/*If a process runs for more than CPUAllocated ticks without terminating or issuing an I/O
	operation (e.g., a print statement), it will be interrupted. The process is then moved to the back of the
	ReadyQueue, and a message is printed indicating a TimeOUT Interrupt.*/

	opcode = mainMemory[programCounter]; //get first opcode of process using program counter

	//while process still has enough CPU ticks and there is still data in mainMemory, keep executing process
	while (dataValue != -1) {
		//use a switch to determine what instruction to execute
		switch (opcode) {
		case 1: //compute operation 
			cout << "compute" << endl; //output operation 

			//go to data base and get iterations and cycles
			iterations = mainMemory[dataBaseIndex];//get number of iterations 
			cpuCycles = mainMemory[dataBaseIndex + 1];//get number of cpu cycles

			dataBaseIndex += 2; //update dataBaseIndex so we can move on to next data
			cpuCyclesUsed += cpuCycles; //update the number of cpu cycles to output later
			cpuCounter += cpuCycles; //update the number of cpu cycles used by the process so that we can check for timeout interrupts
			cpuClock += cpuCycles;//update number of cpu ticks consumed by instruction
			programCounter++; //update programCounter to move to next opcode
			break;
		case 2: //print instruction (i/o interrupt)
			cpuCycles = mainMemory[dataBaseIndex]; //read in number of cpu cycles useds

			dataBaseIndex++; //update dataBaseIndex so that we can move on to other data 
			cpuCyclesUsed += cpuCycles; //update number of cpu cycles
			cpuCounter += cpuCycles; //update the number of cpu cycles used by the process so that we can check for time out interrupts
			programCounter++; //update program counter to move to next opcode


			//save state of current process in mainMemory before moving it to IOWaitingQueue
			mainMemory[startAddress + 2] = programCounter; //update program counter so we can resume at next opcode (instruction) when process is reloaded
			mainMemory[startAddress + 6] = cpuCyclesUsed; //update cpuCyclesUsed in mainMemory
			mainMemory[startAddress + 7] = registerValue; //update registerValues in mainMemory

			//send process to IOWaitingQueue
			IOWaitingQueue.push(cpuClock); //record CPU clock time when process was moved to IOWaitingQueue 
			IOWaitingQueue.push(mainMemoryBase); //push starting address of process to IOWaitingQueue
			IOWaitingQueue.push(dataBaseIndex); //push interrupted process's current data pointer to back of IOWaitingQueue 
			IOWaitingQueue.push(cpuCycles); //push number of cpu cycles required to complete print operation in IOWaitingQueue 
			cout << "Process " << processID << " issued an IOInterrupt and moved to the IOWaitingQueue." << endl; //issue io interrupt 

			//since we have io interrupt, check if there are any processes in IOWaitingQueue that need to be tranfered back to readyQueue
			if (!IOWaitingQueue.empty()) {
				checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
			}
			return; //exit this process (context switch) and execute next process in readyQueue
			break;
		case 3: //store instruction
			value = mainMemory[dataBaseIndex];//read in value 
			address = mainMemory[dataBaseIndex + 1];//read in address

			//if the array location is within bounds say stored, otherwise say store error!
			if (address > memoryLimit) {
				cout << "store error!" << endl;
				registerValue = value;//update register value 

			}
			else {
				cout << "stored" << endl;
				registerValue = value; //update simulated register value  
				mainMemory[mainMemoryBase + address] = registerValue; //account for process's logical memory as shown by Xavier
			}
			dataBaseIndex += 2; //update dataBaseIndex to move on to other data
			cpuCyclesUsed++; //update cpu cycles - store takes 1 cpu time
			cpuCounter++; //update the number of cpu cycles used by the process so that we can check for interrupts
			cpuClock++; //update number of cpu ticks consumed by instruction
			programCounter++; //update programCounter to move to next opcode
			break;
		case 4: //load instruction
			address = mainMemory[dataBaseIndex]; //get address of where value is being retrieved
			//if array location is within bounds say loaded, otherwise say load error!
			if (address >= memoryLimit) {
				cout << "load error!" << endl;
			}
			else {
				cout << "loaded" << endl;
				registerValue = mainMemory[mainMemoryBase + address]; //account for process's logical memory as shown by Xavier

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
			if (mainMemory[dataBaseIndex] == -1) {
				break;
			}
			else {
				cout << "Process " << processID << " has a TimeOUT interrupt and is moved to the ReadyQueue." << endl; //output message

				//update the PCB fields in mainMemory to save state of process
				mainMemory[startAddress + 2] = programCounter; //update program counter so we can resume at next opcode when process is reloaded
				mainMemory[startAddress + 6] = cpuCyclesUsed;//update cpuCyclesUsed in mainMemory
				mainMemory[startAddress + 7] = registerValue;//update registerValues in mainMemory

				readyQueue.push(mainMemoryBase); //push starting address of process to back of readyQueue
				readyQueue.push(dataBaseIndex); //push interrupted process's data pointer to back of ready queue after storing its state

				//The IOWaitingQueue is checked for job transfers every time an interrupt occurs in the CPU	
				if (!IOWaitingQueue.empty()) {
					checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
				}
				return; //exit this process and execute the next process in the readyQueue
			}
		}

		opcode = mainMemory[programCounter]; //get next opcode in the process using programCounter
		dataValue = mainMemory[dataBaseIndex]; //get next data value
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

	//check if there are still processes in NewJobQueue waiting to be added to memory 
	if (!NewJobQueue.empty()) {
		//get waiting process information 
		PCB waitingProcess = NewJobQueue.front();
		int waitingProcessID = waitingProcess.processID;
		int waitingProcessSize = waitingProcess.maxMemoryNeeded + 10; //account for 10 pcb fields

		//if there is available memory, load the process into memory directly (first empty block with enough space)
		if (checkAvailableMemory(memoryTracker, waitingProcessID, waitingProcessSize)) {
			loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, maxMemory, memoryTracker);
		}
		else {
			//else, trigger memory coalescing and attempt to load the job into main memory 
			cout << "Insufficient memory for Process " << waitingProcessID << ". Attempting memory coalescing." << endl;

			//if memory coalescing successful and process can be loaded, try loading again 
			if (triggerMemoryCoalescing(memoryTracker, waitingProcessID, waitingProcessSize)) {
				cout << "Memory coalesced. Process " << waitingProcessID << " can now be loaded." << endl;
				loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, maxMemory, memoryTracker);
			}
			else {
				cout << "Process " << waitingProcessID << " waiting in NewJobQueue due to insufficient memory." << endl;
			}
		}

	}

	//check if there are any processes in waiting queue that need to be tranfered to readyQueue
	if (!IOWaitingQueue.empty()) {
		checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
	}

}


int main() {
	int maxMemory; //maximum size of main memory vector
	int numProcesses; //total number of processes
	queue<PCB> NewJobQueue; //for storing process metadata, instructions, and data 
	queue<int> readyQueue; //stores the starting address (int) of each process in main memory and data pointer (got help from Clark)
	queue<int> IOWaitingQueue; //queue to store information about processes that are waiting for I/O operations to complete
	PCB newProcess; // create a new struct variable that will store all the information for each process that will be added to the NewJobQueue

	//Implement input parsing and populate newJobQueue
	int processID, maxMemoryNeeded, numInstructions; //PCB variables
	int CPUAllocated; //specifies the number of CPU ticks a process can execute before it gives TimeOUT interrupt
	int contextSwitch; //specifies the number of CPU ticks required to perform a context switch (move a job from readyQueue to running state)

	cin >> maxMemory; //read in the first number specifying the maximum size of main memory
	vector <int> mainMemory(maxMemory, -1); //dynamically create vector with max memory size, with default value of -1


	list<memoryBlock> memoryBlockTracker; // main memory is tracked using linked list, with each node representing a memory block 

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

	for (int i = 0; i < numProcesses; i++) {
		//get process details and populate the newJobQueue
		cin >> processID; //get processID 
		cin >> maxMemoryNeeded; //get max memory needed for the process
		cin >> numInstructions; //get number of instructions in process

		//fill out 10 PCB data fields
		newProcess.processID = processID; //assign processID to PCB variable 
		newProcess.state = "NEW"; //set initial state of process to NEW
		newProcess.programCounter = 0;//set program counter to 0
		newProcess.instructionBase = 0; //array index where instructions start in main memory (initially set to 0 and update when loading into main memory)
		newProcess.dataBase = numInstructions + newProcess.instructionBase; //number of instructions + mainMemoryBase
		newProcess.memoryLimit = maxMemoryNeeded; //assign the amount of memory needed by the process as read in from the input file
		newProcess.cpuCyclesUsed = 0; //initially set to 0 and update later as the CPU executes the instructions
		newProcess.registerValue = 0; //initially set to 0 for the register value and update later when CPU executes
		newProcess.maxMemoryNeeded = maxMemoryNeeded; // assign amount of memory required for the process to PCB variable 
		newProcess.mainMemoryBase = 0; //initially intitialize this to 0 and update later after loading into memory
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
	loadJobsToMemory(NewJobQueue, readyQueue, mainMemory, maxMemory, memoryBlockTracker);

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

			// Execute job, checking after each termination if there are waiting jobs in NewJobQueue
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
	//Print total cpu time consumed by all processes
	cpuClock += contextSwitch; //context switch out of the last process
	cout << "Total CPU time used: " << cpuClock << "." << endl; //print out the total number of CPU ticks consumed by all processes
	return 0;
}