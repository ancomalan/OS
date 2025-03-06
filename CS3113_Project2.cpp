//alan vo 
//ou spring 2025 
//operating systems - project 2
//this program simulates context switching and interrupts during CPU execution

#include <iostream>
#include <queue>
#include <string>
using namespace std;

int cpuClock = 0; //global variable to keep track of the CPU clock

struct PCB {
	// Define 10 PCB fields as described earlier
	int processID; //unique identifier for the process
	string state; // tracks current status of the process: NEW, READY, RUNNING,or TERMINATED.
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





void loadJobsToMemory(queue<PCB>& newJobQueue, queue<int>& readyQueue, vector<int>& mainMemory, int maxMemory) {
	// TODO: Implement loading jobs into main memory

	//first load in 10 PCB fields, then the instructions
	int mainMemoryBase = 0; //initialize to 0, but update later to determine where each process begins in main memory
	while (!newJobQueue.empty()) {
		PCB process = newJobQueue.front(); //get the first PCB variable from the new job queue
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

		process.state = "READY";//set process state to READY
		readyQueue.push(mainMemoryBase); //push the starting point of each process in main memory to the ready queue for CPU execution 
		readyQueue.push(process.dataBase); //also push the data pointer of the process 
		mainMemoryBase += process.memoryLimit + 10; //after each process, update the mainMemoryBase for the next process!
		newJobQueue.pop(); //pop newJobQueue and get next process
	}

}

void executeCPU(int startAddress, vector<int>& mainMemory, int CPUAllocated, queue<int>& readyQueue,
	int dataPointer, int contextSwitch, queue<int>& IOWaitingQueue, vector<int>& startTimes) {

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

	//check if there are any processes in waiting queue that need to be tranfered to readyQueue (process termination counts as interrupt)
	if (!IOWaitingQueue.empty()) {
		checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory);
	}
}

int main() {
	int maxMemory; //maximum size of main memory, first line of input file
	int numProcesses; //total number of processes, second line of input file
	queue<PCB> newJobQueue; //for storing PCB metadata, instructions, and data 
	queue<int> readyQueue; //stores the starting address (int) of each process in main memory and data pointer (inspired by Clark)
	queue<int> IOWaitingQueue; //queue storing processes that are waiting for I/O operation (print) to complete
	PCB newProcess; //create a new struct variable that will store all the information for each process that will be added to the newJobQueue


	// Step 1: Read and parse input file
	// TODO: Implement input parsing and populate newJobQueue

	int processID, maxMemoryNeeded, numInstructions; //PCB variables
	int CPUAllocated; //specifies the number of CPU ticks a process can execute before it gives TimeOUT interrupt
	int contextSwitch; //specifies the number of CPU ticks required to perform a context switch (move a job from readyQueue to running state)

	cin >> maxMemory; //read in the first number specifying the maximum size of main memory
	vector <int> mainMemory(maxMemory, -1); //create vector with size of maxMemory, initialized with default values of -1

	cin >> CPUAllocated; //read in number of CPU ticks that each process can execute before timing out 
	cin >> contextSwitch; //read in number of CPU ticks required to perform a context switch
	cin >> numProcesses; //read in total number of processes
	vector<int> startingTimes(numProcesses, 0); //vector to store the cpu clock time of each process when they first entered running state

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
		//populate newJobQueue
		newJobQueue.push(newProcess);
	}


	// Step 2: Load jobs into main memory
	loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

	// Step 3: After you load the jobs in the queue go over the main memory and print its contents
	for (int i = 0; i < mainMemory.size(); i++) {
		cout << i << " : " << mainMemory.at(i) << endl;
	}


	//Step 4: Process execution
	//readyQueue contains start addresses w.r.t main memory for jobs and data pointers
	//while there are still processes in readyQueue or IOWaitingQueue, keep executing processes
	while (!readyQueue.empty() || !IOWaitingQueue.empty()) {

		//if ready queue is not empty, keep executing processes
		if (!readyQueue.empty()) {
			int startAddress = readyQueue.front(); //get starting address of process
			readyQueue.pop();
			int dataBaseIndex = readyQueue.front(); //get data pointer for process
			readyQueue.pop();

			// Execute job
			executeCPU(startAddress, mainMemory, CPUAllocated, readyQueue, dataBaseIndex, contextSwitch, IOWaitingQueue, startingTimes);
		}
		//if ready queue IS empty, keep checking IOWaitingQueue for a process that has completed the IO operation
		else {
			//while readyQueue is empty, keep looking for processes in waiting queue
			while (readyQueue.empty()) {
				cpuClock += contextSwitch; //increment cpu clock when readyQueue is empty
				checkIOWaitingQueue(IOWaitingQueue, readyQueue, mainMemory); //check IOWaitingQueue
			}
		}
	}

	// Step 5: Print total cpu time consumed by all processes
	cpuClock += contextSwitch; //context switch out of the last process
	cout << "Total CPU time used: " << cpuClock << "." << endl; //print out the total number of CPU ticks consumed by all processes

	return 0;
}
