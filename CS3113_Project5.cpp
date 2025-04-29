// alan vo
// ou spring 2025
// operating systems - project 5
// computes the sum of an integer array using a structured hierarchy of pthreads arranged as a full binary tree

#include <iostream>
#include <vector>
#include <cmath>
#include <pthread.h>
using namespace std;

pthread_mutex_t print_mutex; // mutex preventing threads from printing at same time (cleaner output)

// struct holds data about each thread
struct ThreadArg
{
	int index;
	int level;
	int positionFromLeft;
	int leftChildIndex;		 // for results array
	int rightChildIndex;	 // for results array
	bool compute_ready;		 // used to signal that threads are ready to compute their sums
	bool terminate_ready;	 // used to signal that threads are ready to terminate
	vector<int> input;		 // input array that we are computing the sum of
	ThreadArg *threadArgPtr; // pointer to threadArg struct array
	int *resultsPtr;		 // pointer to results array storing sum of each thread
	bool *readyArray;		 // ready array idea from Clark that indicates if results are ready to be used by parent thread
	bool *terminateArray;	 // this array will be used to signal that parent threads are ready to terminate
	pthread_t thread;
	pthread_mutex_t compute_mutex;
	pthread_mutex_t terminate_mutex;
	pthread_cond_t compute_cond;
	pthread_cond_t terminate_cond;

	// for leaf threads:
	bool isLeaf;
	int startIndex; // of input array
	int endIndex;	// of input array
};

// compute sum function (executed by each thread)
void *computeSum(void *arg)
{
	ThreadArg *threadArgs = static_cast<ThreadArg *>(arg); // declare ThreadArg pointer to access arguments of thread
	int parentIndex = (threadArgs->index - 1) / 2;		   // current thread's parent index
	int leftChildIndex = threadArgs->leftChildIndex;	   // current thread's left child index
	int rightChildIndex = threadArgs->rightChildIndex;	   // current thread's right child index
	int sum = 0;										   // running total for each thread's sum

	// COMPUTATION PHASE
	// all threads wait until compute_ready is true before computing their sums (thread is signaled to possibly begin computation)
	pthread_mutex_lock(&threadArgs->compute_mutex);
	while (threadArgs->compute_ready != true)
	{
		pthread_cond_wait(&threadArgs->compute_cond, &threadArgs->compute_mutex);
	}
	pthread_mutex_unlock(&threadArgs->compute_mutex);

	// leaf thread computation
	if (threadArgs->isLeaf)
	{
		// compute sum of input array from startIndex to endIndex
		for (int i = threadArgs->startIndex; i <= threadArgs->endIndex; i++)
		{
			sum += threadArgs->input[i]; // add each element in chunk to sum
		}
		threadArgs->resultsPtr[threadArgs->index] = sum;  // store sum of leaf thread in results array at its index
		threadArgs->readyArray[threadArgs->index] = true; // set ready array to true for this thread so parent thread can use it

		pthread_mutex_lock(&print_mutex); // lock print mutex for clean output
		cout << "[Thread Index " << threadArgs->index << "]" << " [Level " << threadArgs->level << ", Position " << threadArgs->positionFromLeft << "] [TID " << threadArgs->thread << "] computed leaf sum: " << sum << endl;
		pthread_mutex_unlock(&print_mutex);

		// signal parent thread that its child thread has finished computing its sum and that it MAY be able to compute its sum
		pthread_mutex_lock(&threadArgs->threadArgPtr[parentIndex].compute_mutex);	// lock compute mutex for each leaf thread
		threadArgs->threadArgPtr[parentIndex].compute_ready = true;					// set compute_ready to true for parent thread
		pthread_cond_signal(&threadArgs->threadArgPtr[parentIndex].compute_cond);	// wake up parent thread
		pthread_mutex_unlock(&threadArgs->threadArgPtr[parentIndex].compute_mutex); // unlock compute mutex so other threads can compute
	}
	// computation for internal/parent threads that have children
	else
	{
		while (threadArgs->readyArray[leftChildIndex] != true || threadArgs->readyArray[rightChildIndex] != true)
		{
			// parent thread keeps waiting for both of its children to be ready before computing
			// learned to use while loop from claude ai
		}
		pthread_mutex_lock(&print_mutex);
		cout << "[Thread Index " << threadArgs->index << "] [Level " << threadArgs->level << ", Position " << threadArgs->positionFromLeft << "] [TID " << threadArgs->thread << "] received: " << endl;
		cout << "Left Child [Index " << leftChildIndex << ", Level " << threadArgs->threadArgPtr[leftChildIndex].level << ", Position " << threadArgs->threadArgPtr[leftChildIndex].positionFromLeft << ", TID " << threadArgs->threadArgPtr[leftChildIndex].thread << "]: " << threadArgs->resultsPtr[leftChildIndex] << endl;
		cout << "Right Child [Index " << rightChildIndex << ", Level " << threadArgs->threadArgPtr[rightChildIndex].level << ", Position " << threadArgs->threadArgPtr[rightChildIndex].positionFromLeft << ", TID " << threadArgs->threadArgPtr[rightChildIndex].thread << "]: " << threadArgs->resultsPtr[rightChildIndex] << endl;
		sum = threadArgs->resultsPtr[leftChildIndex] + threadArgs->resultsPtr[rightChildIndex]; // parent computes sum of left and right child threads
		threadArgs->resultsPtr[threadArgs->index] = sum;										// store sum of internal thread in results array at its index
		threadArgs->readyArray[threadArgs->index] = true;
		if (threadArgs->index == 0)
		{
			cout << "[Thread Index 0] computed final sum: " << sum << endl;
		}
		else
		{
			cout << "[Thread Index " << threadArgs->index << "] computed sum: " << sum << endl;
		}
		pthread_mutex_unlock(&print_mutex);

		if (threadArgs->index != 0)
		{																				// if not root thread, signal their own parent thread that they have completed computing their sums
			pthread_mutex_lock(&threadArgs->threadArgPtr[parentIndex].compute_mutex);	// lock compute mutex for parent thread
			threadArgs->threadArgPtr[parentIndex].compute_ready = true;					// set compute_ready to true for parent thread
			pthread_cond_signal(&threadArgs->threadArgPtr[parentIndex].compute_cond);	// wake up this thread's parent
			pthread_mutex_unlock(&threadArgs->threadArgPtr[parentIndex].compute_mutex); // unlock compute mutex so other threads can compute
		}
	}

	// TERMINATION PHASE
	pthread_mutex_lock(&threadArgs->terminate_mutex);
	// keep waiting until terminate signal from parent is sent
	while (threadArgs->terminate_ready != true)
	{
		pthread_cond_wait(&threadArgs->terminate_cond, &threadArgs->terminate_mutex);
	}
	pthread_mutex_unlock(&threadArgs->terminate_mutex);

	// if thread leaf terminate right away
	if (threadArgs->isLeaf)
	{
		threadArgs->terminateArray[threadArgs->index] = true; // set terminate array to true for this thread so parent thread can possibly terminate
		pthread_mutex_lock(&print_mutex);					  // lock print mutex so only one thread can print at a time
		cout << "[Thread Index " << threadArgs->index << "] [Level " << threadArgs->level << ", Position " << threadArgs->positionFromLeft << "] terminated." << endl;
		pthread_mutex_unlock(&print_mutex);
	}
	// if internal thread, signal two children and to wait for children to terminate first
	else
	{
		// signal left child thread to terminate
		pthread_mutex_lock(&threadArgs->threadArgPtr[leftChildIndex].terminate_mutex);
		threadArgs->threadArgPtr[leftChildIndex].terminate_ready = true;
		pthread_cond_signal(&threadArgs->threadArgPtr[leftChildIndex].terminate_cond);
		pthread_mutex_unlock(&threadArgs->threadArgPtr[leftChildIndex].terminate_mutex);

		// signal right child thread to terminate
		pthread_mutex_lock(&threadArgs->threadArgPtr[rightChildIndex].terminate_mutex);
		threadArgs->threadArgPtr[rightChildIndex].terminate_ready = true;
		pthread_cond_signal(&threadArgs->threadArgPtr[rightChildIndex].terminate_cond);
		pthread_mutex_unlock(&threadArgs->threadArgPtr[rightChildIndex].terminate_mutex);
		while (threadArgs->terminateArray[leftChildIndex] != true || threadArgs->terminateArray[rightChildIndex] != true)
		{
			// internal thread keeps waiting for both of its children to terminate before terminating itself
		}
		threadArgs->terminateArray[threadArgs->index] = true; // update terminate array indicating that this thread has terminated
		pthread_mutex_lock(&print_mutex);
		cout << "[Thread Index " << threadArgs->index << "] [Level " << threadArgs->level << ", Position " << threadArgs->positionFromLeft << "] terminated." << endl;
		pthread_mutex_unlock(&print_mutex);
	}
	return NULL;
}

int main()
{
	// Step 1: Read inputs
	int H; // tree height (longest distance from root to leaf)
	int M; // input array size
	cin >> H >> M;
	vector<int> input(M); // create vector with M integers (that we want to get the sum of)
	// Read M integers into input vector
	for (int i = 0; i < M; i++)
	{
		cin >> input[i];
	}

	// Step 2: Compute tree parameters
	int N = pow(2, (H - 1)); // (number of leaf threads) = 2^(H-1)
	// if input array size is not divisible by N, pad input array with zeros to make it divisible
	if (M % N != 0)
	{
		int numZerosPadded = N - M % N;	  // amount of 0s needed to pad array
		input.resize(M + numZerosPadded); // resize vector with padded 0's
	}
	int chunkSize = input.size() / N; // array is divided into N equal chunks of this size
	int totalThreads = pow(2, H) - 1; // total number of threads

	// Step 3: Allocate arrays
	ThreadArg threadArgs[totalThreads]; // array of structs to hold data for each thread
	int results[totalThreads];			// stores sums of each thread
	bool resultsReady[totalThreads];	// ready array idea from Clark that indicates if results are ready to be used by parent thread
	bool terminateReady[totalThreads];	// this array will be used to signal that parent threads are ready to terminate

	// Step 4: Initialize each thread's data
	pthread_mutex_init(&print_mutex, NULL); // initialize mutex for printing clean output
	int j = 0;								// used to calculate which leaf thread we are on
	for (int i = 0; i <= totalThreads - 1; i++)
	{
		// initialize boolean arrays to false
		resultsReady[i] = false;
		terminateReady[i] = false;

		// initialize rest of thread arguments
		threadArgs[i].index = i;													// assign index of thread in array based tree
		threadArgs[i].level = floor(log2(i + 1)) + 1;								// assign level of thread in tree
		threadArgs[i].positionFromLeft = i - (pow(2, threadArgs[i].level - 1) - 1); // assign position from left
		threadArgs[i].leftChildIndex = (2 * i) + 1;									// assign index of left child
		threadArgs[i].rightChildIndex = (2 * i) + 2;								// assign index of right child
		threadArgs[i].compute_ready = false;										// initialize compute ready to false
		threadArgs[i].terminate_ready = false;										// initialize terminate ready to false
		threadArgs[i].input = input;												// assign input vector
		threadArgs[i].threadArgPtr = threadArgs;									// assign pointer to threadArgs array so we can access in computeSum function
		threadArgs[i].resultsPtr = results;											// assign pointer to results array so we can access in computeSum function
		threadArgs[i].readyArray = resultsReady;									// assign pointer to ready array so we can access in computeSum function
		threadArgs[i].terminateArray = terminateReady;								// assign pointer to terminate array so we can access in computeSum function
		threadArgs[i].isLeaf = false;												// indicate that this thread is not a leaf

		// check if leaf thread (begin at index 2^(H-1) - 1 and go up to 2^(H-1) + 2
		if (i >= pow(2, H - 1) - 1 && i <= pow(2, H) - 2)
		{
			threadArgs[i].isLeaf = true;					  // indicate that this thread is a leaf
			threadArgs[i].startIndex = j * chunkSize;		  // assign start index of input array
			threadArgs[i].endIndex = (j + 1) * chunkSize - 1; // assign end index of input array
			j++;											  // update which leaf thread we are on
		}

		// Initialize mutexes and condition variables
		pthread_mutex_init(&threadArgs[i].compute_mutex, NULL);	  // initialize compute mutex for thread
		pthread_mutex_init(&threadArgs[i].terminate_mutex, NULL); // initialize terminate mutex for thread
		pthread_cond_init(&threadArgs[i].compute_cond, NULL);	  // initialize compute cond for thread
		pthread_cond_init(&threadArgs[i].terminate_cond, NULL);	  // initialize terminate cond for thread
	}

	// Step 5: Create all threads (loop)
	// - For each index i, call pthread_create(&threadArgs[i].thread, ...,
	// computeSum, &threadArgs[i])
	// pass the struct (ThreadArg) as an argument
	for (int i = 0; i < totalThreads; i++)
	{
		pthread_create(&threadArgs[i].thread, NULL, computeSum, &threadArgs[i]);
	}

	// Step 6: Trigger leaf threads to begin computation
	// - For each leaf thread (index from 2^(H-1)-1 to 2^H-2):
	// - Lock compute_mutex
	// - Set compute_ready = true
	// - Signal compute_cond
	// - Unlock compute_mutex
	// for loop from first leaf index to last leaf index
	for (int i = pow(2, H - 1) - 1; i <= pow(2, H) - 2; i++)
	{
		pthread_mutex_lock(&threadArgs[i].compute_mutex);	// lock compute mutex for each leaf thread
		threadArgs[i].compute_ready = true;					// set compute_ready to true
		pthread_cond_signal(&threadArgs[i].compute_cond);	// send signal from main thread to any thread waiting on this condition
		pthread_mutex_unlock(&threadArgs[i].compute_mutex); // unlock compute mutex so other threads can compute
	}

	// Step 7: Wait for root thread to finish computation
	// - Wait on a condition variable OR
	// - Use polling to detect when result[0] is ready
	while (resultsReady[0] != true)
	{
		// waiting.....
	}
	pthread_mutex_lock(&print_mutex);
	cout << "\n[Thread Index 0] now initiates tree cleanup." << endl;
	cout << endl;
	pthread_mutex_unlock(&print_mutex);

	// Step 8: Trigger root thread to begin termination
	// - Lock terminate_mutex for root
	// - Set terminate_ready = true
	// - Signal terminate_cond
	// - Unlock terminate_mutex
	pthread_mutex_lock(&threadArgs[0].terminate_mutex);	  // lock terminate mutex for root thread
	threadArgs[0].terminate_ready = true;				  // set terminate_ready to true for root thread
	pthread_cond_signal(&threadArgs[0].terminate_cond);	  // send signal from main thread to any thread waiting on this condition
	pthread_mutex_unlock(&threadArgs[0].terminate_mutex); // unlock terminate mutex so other threads can terminate

	// Step 9: Join all threads
	// - For all threads in threadArgs, call pthread_join
	for (int i = 0; i < totalThreads; i++)
	{
		pthread_join(threadArgs[i].thread, NULL);
	}

	// Step 10: Destroy mutexes and condition variables
	// - For each threadArg, destroy compute/terminate mutexes and conds
	for (int i = 0; i < totalThreads; i++)
	{
		pthread_mutex_destroy(&threadArgs[i].compute_mutex);   // destroy compute mutex
		pthread_mutex_destroy(&threadArgs[i].terminate_mutex); // destroy terminate mutex
		pthread_cond_destroy(&threadArgs[i].compute_cond);	   // destroy compute cond
		pthread_cond_destroy(&threadArgs[i].terminate_cond);   // destroy terminate cond
	}
	pthread_mutex_destroy(&print_mutex); // destroy print mutex
	return 0;
}