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

void *operateOnTree(tArgs* tData) {
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
        chooseOperation = 0; // randomly number * 100
        key = 0; // randomly generage from keyRange + 2

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
        chooseOperation = 0; // random * 100
        key = 0; // random + 2

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
};

int main(int argc, char **argv) {

    return 0;
}