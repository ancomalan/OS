//alan vo 
//ou spring 2025 
//operating systems - project 1 

#include <iostream>
#include <queue>
#include <string>
#include <fstream> //for files
using namespace std;

struct PCB {
	// Define PCB fields as described earlier
	int processID; //unique identifier for the process
	string state; // tracks current status of the process: NEW, READY, RUNNING,or TERMINATED.
	int programCounter; //index of the next instruction to be executed within the process's logical memory
	int instructionBase; //The instructionBase specifies the starting address of the instructions in the logical memory
	int dataBase; //points to the beginning of the data segment within the logical memory.
	int memoryLimit; // the total size of logical memory allocated to the process
	int cpuCyclesUsed; //accumulates the total CPU cycles consumed by the process during its execution
	int registerValue; //simulated register used to store intermediate values during load and store operations
	int maxMemoryNeeded; //maximum memory required by the process as defined in the input file
	int mainMemoryBase; //denotes the starting address in main memory where the process, including its PCB and logical memory, is loaded

	//additional PCB fields used to display output 
	int numInstructions; //number of instructions associated with the process
	vector <int> encodedInstructions; //stores encoded opcode instructions for the process. I was inspired by chatGPT to use this as a way to store the encoded instructions
	vector <int> data; //stores the data associated with each process (iterations, cycles)
};

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
			state = 4; //encode TERMINATED as int 4
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
		mainMemoryBase += process.memoryLimit + 10; //after each process, update the mainMemoryBase for the next process! Add 20 to match sample output
		newJobQueue.pop(); //pop newJobQueue and get next process
	}

}

void executeCPU(int startAddress, vector<int>& mainMemory) {
	// TODO: Implement CPU instruction execution

	int processID; //unique identifier for the process
	int state; // tracks current status of the process: NEW, READY, RUNNING,or TERMINATED.
	int programCounter; //index of the next instruction to be executed within the process's logical memory
	int instructionBase; //The instructionBase specifies the starting address of the instructions in the logical memory
	int dataBase; //points to the beginning of the data segment within the logical memory.
	int memoryLimit; // the total size of logical memory allocated to the process
	int cpuCyclesUsed; //accumulates the total CPU cycles consumed by the process during its execution
	int registerValue; //simulated register used to store intermediate values during load and store operations
	int maxMemoryNeeded; //maximum memory required by the process as defined in the input file
	int mainMemoryBase; //denotes the starting address in main memory where the process, including its PCB and logical memory, is loaded'
	int opcode; //use for the decoded instructions
	int iterations, cpuCycles, value, address; //data variables associated with the operations


	//using the start address, go through the vector and assign PCB details 
	processID = mainMemory[startAddress];
	state = mainMemory[startAddress + 1];
	programCounter = mainMemory[startAddress + 2];
	instructionBase = mainMemory[startAddress + 3];
	dataBase = mainMemory[startAddress + 4];
	memoryLimit = mainMemory[startAddress + 5];
	cpuCyclesUsed = mainMemory[startAddress + 6];
	registerValue = mainMemory[startAddress + 7];
	maxMemoryNeeded = mainMemory[startAddress + 8];
	mainMemoryBase = mainMemory[startAddress + 9];


	/* Utilize the programCounter as well as the instructionBase and dataBase fields to navigate this layout.
	Also remember that for store and load instructions, addresses are in respect to the process' logical memory,
	not the raw addresses in mainMemory. When the process is done executing, programCounter should reset and point
	to the address directly before instructionBase.*/
	int numInstructions = dataBase - instructionBase; //get number of instructions for the process
	programCounter = instructionBase; //set current index of vector to instructionBase
	int dataBaseIndex = dataBase; //used to traverse through data


	//execute all the instructions
	for (int i = 0; i < numInstructions; i++) {
		opcode = mainMemory[programCounter]; 	//get first opcode by using program counter 


		//use a switch to determine what instruction to execute
		switch (opcode) {
		case 1: //compute operation 
			cout << "compute" << endl; //output operation 
			//go to data base and get iterations and cycles
			iterations = mainMemory[dataBaseIndex];//get number of iterations 
			cpuCycles = mainMemory[dataBaseIndex + 1];//get number of cpu cycles
			dataBaseIndex += 2; //update dataBaseIndex so that we can move on to the other data
			cpuCyclesUsed += cpuCycles; //update the number of cpu cycles to output later
			opcode = mainMemory[programCounter++]; //update the opcode to the next encoded instruction in the process using program counter
			break;
		case 2: //print instruction
			cout << "print" << endl; //output operation
			cpuCycles = mainMemory[dataBaseIndex]; //read in number of cpu cycles useds
			dataBaseIndex++; //update dataBaseIndex so that we can move on to other data 
			cpuCyclesUsed += cpuCycles; //update number of cpu cycles
			opcode = mainMemory[programCounter++]; //update opcode to next encoded instruction in process using program counter
			break;
		case 3: //store instruction
			value = mainMemory[dataBaseIndex];//read in value 
			address = mainMemory[dataBaseIndex + 1];//read in address
			dataBaseIndex += 2; //update dataBaseIndex to move on to other data
			//if the array location is within bounds say stored, otherwise say store error!
			if (address > memoryLimit) {
				cout << "store error!" << endl;
				registerValue = value;//update register value 

			}
			else {
				cout << "stored" << endl;
				registerValue = value; //update simulated register value  
				mainMemory[mainMemoryBase + address] = registerValue;////account for process's logical memory as shown by Xavier
			}
			cpuCyclesUsed++; //update cpu cycles - store takes 1 cpu time
			opcode = mainMemory[programCounter++]; //update the opcode to the next encoded instruction related to the process using programCounter
			break;
		case 4: //load instruction
			address = mainMemory[dataBaseIndex]; //get address of where value is being retrieved
			dataBaseIndex++;//update dataBaseIndex so we can move to other data 
			//if array location is within bounds say loaded, otherwise say load error!
			if (address >= memoryLimit) {
				cout << "load error!" << endl;
			}
			else {
				cout << "loaded" << endl;
				registerValue = mainMemory[mainMemoryBase + address]; //account for process's logical memory as shown by Xavier

			}
			cpuCyclesUsed++;//update cpu cycles - load takes 1 cpu time 
			opcode = mainMemory[programCounter++]; //update the opcode to the next encoded instruction in the process using programCounter
			break;
		default:
			break;
		}

	}

	programCounter = instructionBase - 1;//reset programCounter to the address directly before instructionBase.
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
	cout << "Total CPU Cycles Consumed: " << cpuCyclesUsed << endl;
}


int main() {
	int maxMemory; //maximum size of main memory, first line of input file
	int numProcesses; //total number of processes, second line of input file
	queue<PCB> newJobQueue; //for storing PCB metadata, instructions, and data 
	queue<int> readyQueue; //stores the starting address (int) of each process in main memory
	PCB newProcess; // create a new struct variable that will store all the information for each process that will be added to the newJobQueue


	// Step 1: Read and parse input file
	// TODO: Implement input parsing and populate newJobQueue

	int processID, maxMemoryNeeded, numInstructions; //PCB variables
	cin >> maxMemory; //read in the first number specifying the maximum size of main memory
	vector <int> mainMemory(maxMemory, -1); //dynamically create vector with the first number read from file, with default value of -1
	cin >> numProcesses; //read in total number of processes

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


	// Step 4: Process execution
	while (!readyQueue.empty()) {
		int startAddress = readyQueue.front();
		//readyQueue contains start addresses w.r.t main memory for jobs
		readyQueue.pop();
		// Execute job
		executeCPU(startAddress, mainMemory);
		// Output Job that just completed execution - see example below
	}

	return 0;
}
