#include "LockFreeBST.h"

int NUM_OF_THREADS;
int findPercent;
int insertPercent;
int removePercent;
unsigned long keyRange;
double MOPS;
volatile bool start=false;
volatile bool stop=false;
volatile bool steadyState=false;
struct timespec runTime, transientTime;

void *operateOnTree(void* targuments) {

    tArgs* tData = (tArgs*) targuments;

    int chooseOperation;
    unsigned long lseed;
    unsigned long key;
    lseed = tData->lseed;

    // randomly seed

	tData->newNode=NULL;
    tData->isNewNodeAvailable=false;
	tData->readCount=0;
    tData->successfulReads=0;
    tData->unsuccessfulReads=0;
    tData->readRetries=0;
    tData->insertCount=0;
    tData->successfulInserts=0;
    tData->unsuccessfulInserts=0;
    tData->insertRetries=0;
    tData->deleteCount=0;
    tData->successfulDeletes=0;
    tData->unsuccessfulDeletes=0;
    tData->deleteRetries=0;
	tData->seekRetries=0;
	tData->seekLength=0;

    while (!start) { }

    while (!steadyState) {
        chooseOperation = rand() % 100; // randomly number * 100
        key = rand() % keyRange + 2; // randomly generage from keyRange + 2

        if (chooseOperation < findPercent) {
            search(tData, key);
        } else if (chooseOperation < insertPercent) {
            insert(tData, key);
        } else {
            remove(tData, key);
        }
    }

    tData->readCount=0;
    tData->successfulReads=0;
    tData->unsuccessfulReads=0;
    tData->readRetries=0;
    tData->insertCount=0;
    tData->successfulInserts=0;
    tData->unsuccessfulInserts=0;
    tData->insertRetries=0;
    tData->deleteCount=0;
    tData->successfulDeletes=0;
    tData->unsuccessfulDeletes=0;
    tData->deleteRetries=0;
	tData->seekRetries=0;
	tData->seekLength=0;

    while (!stop) {
        chooseOperation = rand() % 100; // random * 100
        key = rand() % keyRange + 2; // random + 2

        if(chooseOperation < findPercent)
        {
            tData->readCount++;
            search(tData, key);
        }
        else if (chooseOperation < insertPercent)
        {
            tData->insertCount++;
            insert(tData, key);
        }
        else
        {
            tData->deleteCount++;
            remove(tData, key);
        }
    }

    return NULL;
}

tArgs** tArgsArr;

int main(int argc, char **argv) {
	unsigned long lseed;
	//get run configuration from command line
  NUM_OF_THREADS = atoi(argv[1]);
  findPercent = atoi(argv[2]);
  insertPercent= findPercent + atoi(argv[3]);
  removePercent = insertPercent + atoi(argv[4]);

	runTime.tv_sec = atoi(argv[5]);
	runTime.tv_nsec =0;
	transientTime.tv_sec=0;
	transientTime.tv_nsec=2000000;

  keyRange = (unsigned long) atol(argv[6])-1;
	lseed = (unsigned long) atol(argv[7]);

  tArgsArr = (tArgs**) malloc(NUM_OF_THREADS * sizeof(tArgs*)); 

//   const gsl_rng_type* T;
//   gsl_rng* r;
//   gsl_rng_env_setup();
//   T = gsl_rng_default;
//   r = gsl_rng_alloc(T);
//   gsl_rng_set(r,lseed);
	
  createHeadNodes(); //Initialize the tree. Must be called before doing any operations on the tree
  
  tArgs* initialInsertArgs = (tArgs*) malloc(sizeof(tArgs));
  initialInsertArgs->successfulInserts=0;
  initialInsertArgs->newNode=NULL;
  initialInsertArgs->isNewNodeAvailable=false;
	
  while(initialInsertArgs->successfulInserts < keyRange/2) //populate the tree with 50% of keys
  {
    insert(initialInsertArgs, (rand() % keyRange + 2));
  }
  pthread_t threadArray[NUM_OF_THREADS];
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    tArgsArr[i] = (tArgs*) malloc(sizeof(tArgs));
    tArgsArr[i]->tId = i;
    tArgsArr[i]->lseed = rand();
  }

	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		pthread_create(&threadArray[i], NULL, operateOnTree, (void*)((tArgs*)tArgsArr[i]));
	}
	
	start=true; 										//start operations
	nanosleep(&transientTime,NULL); //warmup
	steadyState=true;
	nanosleep(&runTime,NULL);
	stop=true;											//stop operations
	
	for(int i=0;i<NUM_OF_THREADS;i++)
	{
		pthread_join(threadArray[i], NULL);
	}	

  unsigned long totalReadCount=0;
  unsigned long totalSuccessfulReads=0;
  unsigned long totalUnsuccessfulReads=0;
  unsigned long totalReadRetries=0;
  unsigned long totalInsertCount=0;
  unsigned long totalSuccessfulInserts=0;
  unsigned long totalUnsuccessfulInserts=0;
  unsigned long totalInsertRetries=0;
  unsigned long totalDeleteCount=0;
  unsigned long totalSuccessfulDeletes=0;
  unsigned long totalUnsuccessfulDeletes=0;
  unsigned long totalDeleteRetries=0;
	unsigned long totalSeekRetries=0;
	unsigned long totalSeekLength=0;
 
  for(int i=0;i<NUM_OF_THREADS;i++)
  {
    totalReadCount += tArgsArr[i]->readCount;
    totalSuccessfulReads += tArgsArr[i]->successfulReads;
    totalUnsuccessfulReads += tArgsArr[i]->unsuccessfulReads;
    totalReadRetries += tArgsArr[i]->readRetries;

    totalInsertCount += tArgsArr[i]->insertCount;
    totalSuccessfulInserts += tArgsArr[i]->successfulInserts;
    totalUnsuccessfulInserts += tArgsArr[i]->unsuccessfulInserts;
    totalInsertRetries += tArgsArr[i]->insertRetries;
    totalDeleteCount += tArgsArr[i]->deleteCount;
    totalSuccessfulDeletes += tArgsArr[i]->successfulDeletes;
    totalUnsuccessfulDeletes += tArgsArr[i]->unsuccessfulDeletes;
    totalDeleteRetries += tArgsArr[i]->deleteRetries;
		totalSeekRetries += tArgsArr[i]->seekRetries;
		totalSeekLength += tArgsArr[i]->seekLength;
  }
	unsigned long totalOperations = totalReadCount + totalInsertCount + totalDeleteCount;
	MOPS = totalOperations/(runTime.tv_sec*1000000.0);
	printf("k%d;%d-%d-%d;%d;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%ld;%.2f;%.2f\n",atoi(argv[6]),findPercent,(insertPercent-findPercent),(removePercent-insertPercent),NUM_OF_THREADS,size(),totalReadCount,totalInsertCount,totalDeleteCount,totalReadRetries,totalSeekRetries,totalInsertRetries,totalDeleteRetries,totalSeekLength*1.0/totalOperations,MOPS);
	assert(isValidTree());
	pthread_exit(NULL);
}