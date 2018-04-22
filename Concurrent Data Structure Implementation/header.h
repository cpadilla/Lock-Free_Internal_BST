#pragma once

#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<time.h>
#include<stdint.h>
#include<assert.h>
#include<atomic>

#define K 2

typedef enum {INJECTION, DISCOVERY, CLEANUP} Mode;
typedef enum {SIMPLE, COMPLEX} Type;
typedef enum {LEFT=0, RIGHT=1} Side;
typedef enum {DELETE_FLAG, PROMOTE_FLAG} Flag;

// Node class containing a key, two child nodes (LEFT, RIGHT) and a boolean if it's ready to be replaced
class Node
{
private:

public:
    std::atomic<unsigned long> mKey;
    std::atomic<Node*> child[K];
    std::atomic<bool> readyToReplace;
};

class Edge
{
public:
	Node* parent;
	Node* child;
	Side which;

    Edge() {

    }

    Edge(Node* parent, Node* child, Side which) {
        this->parent = parent;
        this->child = child;
        this->which = which;
    }
};

class SeekRecord
{
public:
	Edge lastEdge;
	Edge pLastEdge;
	Edge injectionEdge;

    SeekRecord(SeekRecord* s) {
        this->lastEdge = Edge(s->lastEdge.parent, s->lastEdge.child, s->lastEdge.which);
        this->pLastEdge = Edge(s->pLastEdge.parent, s->pLastEdge.child, s->pLastEdge.which);
        this->injectionEdge = Edge(s->injectionEdge.parent, s->injectionEdge.child, s->injectionEdge.which);
    }
};

class AnchorRecord
{
public:
	Node* node;
	unsigned long key;
};

class StateRecord
{
public:
    int depth;
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

class tArgs
{
public:
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
// unsigned long size();
void printKeys();
bool isValidTree();