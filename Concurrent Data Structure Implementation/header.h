#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<time.h>
#include<stdint.h>
#include<assert.h>
#include<atomic>


#define K 2

#define MAX_KEY 0x7FFFFFFF
#define INF_R 0x0
#define INF_S 0x1
#define INF_T 0x7FFFFFFE
#define KEY_MASK 0x80000000
#define ADDRESS_MASK 15	

#define NULL_BIT 8
#define INJECT_BIT 4
#define DELETE_BIT 2
#define PROMOTE_BIT 1

typedef enum {INJECTION, DISCOVERY, CLEANUP} Mode;
typedef enum {SIMPLE, COMPLEX} Type;
typedef enum {LEFT, RIGHT} Side;
typedef enum {DELETE_FLAG, PROMOTE_FLAG} Flag;


// Node class containing a key, two child nodes (LEFT, RIGHT) and a boolean if it's ready to be replaced
class Node
{
    std::atomic<unsigned long> mKey;
    std::atomic<Node*> child[K];
    std::atomic<bool> readyToReplace;
};

class Edge
{
	Node* parent;
	Node* child;
	Side which;
};

class SeekRecord
{
	Edge lastEdge;
	Edge pLastEdge;
	Edge injectionEdge;
};

class AnchorRecord
{
	Node* node;
	unsigned long key;
};

class StateRecord
{
	Edge targetEdge;
	Edge pTargetEdge;
	unsigned long targetKey;
	unsigned long currentKey;
	Mode mode;
	Type type;
    // the next field stores pointer to a seek record;
    // it is used for finding the successor if the delete operation is complex
	SeekRecord successorRecord;
};

struct tArgs
{
	int tId;
	unsigned long lseed;
	unsigned long readCount;
	unsigned long successfulReads;
	unsigned long unsuccessfulReads;
	unsigned long readRetries;
	unsigned long insertCount;
	unsigned long successfulInserts;
	unsigned long unsuccessfulInserts;
	unsigned long insertRetries;
	unsigned long deleteCount;
	unsigned long successfulDeletes;
	unsigned long unsuccessfulDeletes;
	unsigned long deleteRetries;
	Node* newNode;
	bool isNewNodeAvailable;
    // object to store information about the tree traversal when looking for a given key
    // (used by the seek function)
	SeekRecord targetRecord;
	SeekRecord pSeekRecord;
    // object to store information about process' own delete operation
	StateRecord myState;	
	AnchorRecord anchorRecord;
	AnchorRecord pAnchorRecord;
	unsigned long seekRetries;
	unsigned long seekLength;
};

void createHeadNodes();
bool search(struct tArgs*, unsigned long);
bool insert(struct tArgs*, unsigned long);
bool remove(struct tArgs*, unsigned long);
unsigned long size();
void printKeys();
bool isValidTree();