
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<stdbool.h>
#include<math.h>
#include<time.h>
#include<stdint.h>
#include<assert.h>
#include<atomic>

#include<api/api.hpp>
#include<common/platform.hpp>
#include<common/locks.hpp>
#include"bmconfig.hpp"
#include"../include/api/api.hpp"

#include <stdint.h>

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
typedef enum {LEFT=0, RIGHT=1} Side;
typedef enum {DELETE_FLAG, PROMOTE_FLAG} Flag;

void createHeadNodes();
bool search(struct tArgs*, unsigned long);
bool insert(struct tArgs*, unsigned long);
bool remove(struct tArgs*, unsigned long);
// unsigned long size();
void printKeys();
bool isValidTree();

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

    Edge();
    Edge(Node* parent, Node* child, Side which);
};


Edge::Edge() {}

Edge::Edge(Node* parent, Node* child, Side which) {
        this->parent = parent;
        this->child = child;
        this->which = which;
}



class SeekRecord
{
public:
	Edge lastEdge;
	Edge pLastEdge;
	Edge injectionEdge;

    SeekRecord(SeekRecord* s);
};

SeekRecord::SeekRecord(SeekRecord* s) {
    this->lastEdge = Edge(s->lastEdge.parent, s->lastEdge.child, s->lastEdge.which);
    this->pLastEdge = Edge(s->pLastEdge.parent, s->pLastEdge.child, s->pLastEdge.which);
    this->injectionEdge = Edge(s->injectionEdge.parent, s->injectionEdge.child, s->injectionEdge.which);
}



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

void helpTargetNode(tArgs* t, Edge* helpeeEdge, int depth);
void helpSuccessorNode(tArgs* t, Edge* helpeeEdge, int depth);
void initializeTypeAndUpdateMode(tArgs* t, StateRecord* state);
void findAndMarkSuccessor(tArgs* t, StateRecord* state);
void removeSuccessor(tArgs* t, StateRecord* state);
bool cleanup(tArgs* t, StateRecord* state);
void updateMode(tArgs* t, StateRecord* state);

unsigned long getKey(unsigned long key)
{
    return (key & MAX_KEY);
}

Node* getAddress(Node* p)
{
    return (Node*)((uintptr_t) p &  ~ADDRESS_MASK);
}

bool isNull(Node* p)
{
    return ((uintptr_t) p & NULL_BIT) != 0;
}

bool isKeyMarked(unsigned long key)
{
    return ((key & KEY_MASK) == KEY_MASK);
}

bool isDFlagSet(Node* p)
{
    return ((uintptr_t) p & DELETE_BIT) != 0;
}

bool isPFlagSet(Node* p)
{
    return ((uintptr_t) p & PROMOTE_BIT) != 0;
}

Node* setNull(Node* p)
{
    return (Node*) ((uintptr_t) p | NULL_BIT);
}

Node* setDFlag(Node* p)
{
    return (Node*) ((uintptr_t) p | DELETE_BIT);
}

Node* setPFlag(Node* p)
{
    return (Node*) ((uintptr_t) p | PROMOTE_BIT);
}

bool isIFlagSet(Node* p)
{
    return ((uintptr_t) p & INJECT_BIT) != 0;
}

Node* setIFlag(Node* p)
{
    return (Node*) ((uintptr_t) p | INJECT_BIT);
}

unsigned long setReplaceFlagInKey(unsigned long key)
{
    return (key | KEY_MASK);
}

bool CAS(Node* parent, int which, Node* oldChild, Node* newChild)
{
	return parent->child[which].compare_exchange_strong(oldChild,newChild,std::memory_order_seq_cst);
}

Node* newLeafNode(unsigned long key)
{
  Node* node = (Node*) malloc(sizeof(Node));
  node->mKey = key;
  node->child[LEFT] = setNull(NULL); 
  node->child[RIGHT] = setNull(NULL);
	node->readyToReplace=false;
  return(node);
}

Node* R;
Node* S;
Node* T;
unsigned long numOfNodes;

TM_CALLABLE
void seek(tArgs* t, unsigned long key TM_ARG, SeekRecord* s)
{
	AnchorRecord* pAnchorRecord;
	AnchorRecord* anchorRecord;
	
	Edge pLastEdge;
	Edge lastEdge;
	
	Node* curr;
	Node* next;
	Node* temp;
	Side which;
	
	bool n;
	bool d;
	bool p;
	unsigned long cKey;
	unsigned long aKey;
	
	pAnchorRecord = &t->pAnchorRecord;
	anchorRecord = &t->anchorRecord;
	
	pAnchorRecord->node = S;
	pAnchorRecord->key = INF_S;
	
	while(true)
	{
		//initialize all variables used in traversal
        pLastEdge = Edge(R, S, RIGHT);
        lastEdge = Edge(S, T, RIGHT);
		// populateEdge(&pLastEdge,R,S,RIGHT);
		// populateEdge(&lastEdge,S,T,RIGHT);
		curr = T;
		anchorRecord->node = S;
		anchorRecord->key = INF_S;
		while(true)
		{
			t->seekLength++;
			//read the key stored in the current node
			cKey = getKey(curr->mKey);
			//find the next edge to follow
			which = key<cKey ? LEFT:RIGHT;
			temp = curr->child[which];
			n=isNull(temp);	d=isDFlagSet(temp);	p=isPFlagSet(temp);	next=getAddress(temp);
			//check for completion of the traversal
			TM_BEGIN(atomic){
			if(TM_READ(key) == cKey || n)
			{
				// TM_BEGIN(atomic){
					//either key found or no next edge to follow. Stop the traversal
					s->pLastEdge = pLastEdge;
					s->lastEdge = lastEdge;
					// TM_WRITE(s->pLastEdge, TM_READ(pLastEdge));
					// TM_WRITE(s->lastEdge, TM_READ(lastEdge));
					// populateEdge(&s->injectionEdge,curr,next,which);
					s->injectionEdge = Edge(curr, next, which);
					// TM_WRITE(s->injectionEdge, TM_READ(Edge(curr, next, which)));
				// }TM_END;
				if(TM_READ(key) == cKey)
				{
					//key matches. So return
					return;					
				}
				else
				{
					break;
				}
			}
			}TM_END;
			if(which == RIGHT)
			{
				//the next edge that will be traversed is a right edge. Keep track of the current node and its key
				anchorRecord->node = curr;
				anchorRecord->key = cKey;
			}
			//traverse the next edge
            lastEdge = Edge(curr, next, which);
			pLastEdge = lastEdge;
			// populateEdge(&lastEdge,curr,next,which);
			curr = next;			
		}
		//key was not found. check if can stop
		aKey = getKey(anchorRecord->node->mKey);
		if(anchorRecord->key == aKey)
		{
			temp = anchorRecord->node->child[RIGHT];
			d=isDFlagSet(temp);	p=isPFlagSet(temp);
			if(!d && !p)
			{
				//the anchor node is part of the tree. Return the results of the current traversal
				return;
			}
			if(pAnchorRecord->node == anchorRecord->node && pAnchorRecord->key == anchorRecord->key)
			{
				//return the results of previous traversal
				// TM_BEGIN(atomic){
					s = new SeekRecord(&t->pSeekRecord);
					// TM_WRITE(s, TM_READ(new SeekRecord(&t->pSeekRecord)));
				// }TM_END;
				return;
			}
		}
		//store the results of the current traversal and restart
			TM_BEGIN(atomic){
				t->pSeekRecord = SeekRecord(TM_READ(s));
			}TM_END;
			pAnchorRecord->node = anchorRecord->node;
			pAnchorRecord->key = anchorRecord->key;
			t->seekRetries++;
	}
}

void findSmallest(tArgs* t, Node* node, SeekRecord* s)
{
	Node* curr;
	Node* left;
	Node* right;
	Node* temp;
	bool n;
	Edge lastEdge;
	Edge pLastEdge;
	
	//find the node with the smallest key in the subtree rooted at the right child
	//initialize the variables used in the traversal
	right = getAddress(node->child[RIGHT]);
	// populateEdge(&lastEdge,node,right,RIGHT);
	// populateEdge(&pLastEdge,node,right,RIGHT);
    lastEdge = Edge(node, right, RIGHT);
    pLastEdge = Edge(node, right, RIGHT);

	while(true)
	{
		curr = lastEdge.child;
		temp = curr->child[LEFT];
		n=isNull(temp);	left=getAddress(temp);
		if(n)
		{
			break;
		}
		//traverse the next edge
		pLastEdge = lastEdge;
		// populateEdge(&lastEdge,curr,left,LEFT);
        lastEdge = Edge(curr, left, LEFT);
	}
	//initialize seek record and return
	s->lastEdge = lastEdge;
	s->pLastEdge = pLastEdge;
	return;
}

bool markChildEdge(tArgs* t, StateRecord* state, Side which)
{
	Node* node;
	Edge edge;
	Flag flag;
	Node* address;
	Node* temp;
	bool n;
	bool i;
	bool d;
	bool p;
	Edge helpeeEdge;
	Node* oldValue;
	Node* newValue;
	bool result;
	
	if(state->mode == INJECTION)
	{	
		edge = state->targetEdge;
		flag = DELETE_FLAG;
	}
	else
	{
		edge = state->successorRecord.lastEdge;
		flag = PROMOTE_FLAG;
	}
	node = edge.child;
	while(true)
	{
		temp = node->child[which];
		n=isNull(temp);	i=isIFlagSet(temp);	d=isDFlagSet(temp);	p=isPFlagSet(temp);	address=getAddress(temp);
		if(i)
		{
			// populateEdge(&helpeeEdge,node,address,which);
            helpeeEdge = Edge(node, address, which);
			helpTargetNode(t,&helpeeEdge,state->depth+1);
			continue;
		}
		else if(d)
		{
			if(flag == PROMOTE_FLAG)
			{
				helpTargetNode(t,&edge,state->depth+1);
				return false;
			}
			else
			{
				return true;
			}
		}
		else if(p)
		{
			if(flag == DELETE_FLAG)
			{
				helpSuccessorNode(t,&edge,state->depth+1);
				return false;
			}
			else
			{
				return true;
			}
		}
		if(n)
		{
			oldValue = setNull(address);
		}
		else
		{
			oldValue = address;
		}
		if(flag == DELETE_FLAG)
		{
			newValue = setDFlag(oldValue);
		}
		else
		{
			newValue = setPFlag(oldValue);
		}
		result = CAS(node,which,oldValue,newValue);
		if(!result)
		{
			continue;
		}
		else
		{
			break;
		}
	}
	return true;
}

void helpTargetNode(tArgs* t, Edge* helpeeEdge, int depth)
{
	StateRecord* state;
	bool result;
	// intention flag must be set on the edge
	// obtain new state record and initialize it
	state = (StateRecord*) malloc(sizeof(StateRecord));
	state->targetEdge = *helpeeEdge;
	state->depth = depth;
	state->mode = INJECTION;
	
	// mark the left and right edges if unmarked
	result = markChildEdge(t,state, LEFT);
	if(!result)
	{
		return;
	}
	markChildEdge(t,state,RIGHT);
	initializeTypeAndUpdateMode(t,state);
	if(state->mode == DISCOVERY)
	{
		findAndMarkSuccessor(t,state);
	}
	
	if(state->mode == DISCOVERY)
	{
		removeSuccessor(t,state);
	}
	if(state->mode == CLEANUP)
	{
		cleanup(t,state);
	}
	
	return;
}

void removeSuccessor(tArgs* t, StateRecord* state)
{
	Node* node;
	SeekRecord* s;
	Edge successorEdge;
	Edge pLastEdge;
	Node* temp;
	Node* right;
	Node* address;
	Node* oldValue;
	Node* newValue;
	bool n;
	bool d;
	bool p;
	bool i;
	bool dFlag;
	bool which;
	bool result;
	
	//retrieve addresses from the state record
	node = state->targetEdge.child;
	s = &state->successorRecord;
    findSmallest(t,node,s);
	//extract information about the successor node
	successorEdge = s->lastEdge;
	//ascertain that the seek record for the successor node contains valid information
	temp = successorEdge.child->child[LEFT];
	p=isPFlagSet(temp);	address=getAddress(temp);
	if(address!=node)
	{
		node->readyToReplace = true;
		updateMode(t,state);
		return;
	}
	if(!p)
	{
		node->readyToReplace = true;
		updateMode(t,state);
		return;
	}

	//mark the right edge for promotion if unmarked
	temp = successorEdge.child->child[RIGHT];
	p=isPFlagSet(temp);
	if(!p)
	{
		//set the promote flag on the right edge
		markChildEdge(t,state,RIGHT);
	}
	//promote the key
	node->mKey = setReplaceFlagInKey(successorEdge.child->mKey);
	while(true)
	{
		//check if the successor is the right child of the target node itself
		if(successorEdge.parent == node)
		{
			dFlag = true; which = RIGHT;
		}
		else
		{
			dFlag = false; which = LEFT;
		}
		i=isIFlagSet(successorEdge.parent->child[which]);
		temp = successorEdge.child->child[RIGHT];
		n=isNull(temp);	right=getAddress(temp);
		if(n)
		{
			//only set the null flag. do not change the address
			if(i)
			{
				if(dFlag)
				{
					oldValue = setIFlag(setDFlag(successorEdge.child));
					newValue = setNull(setDFlag(successorEdge.child));
				}
				else
				{
					oldValue = setIFlag(successorEdge.child);
					newValue = setNull(successorEdge.child);
				}
			}
			else
			{
				if(dFlag)
				{
					oldValue = setDFlag(successorEdge.child);
					newValue = setNull(setDFlag(successorEdge.child));
				}
				else
				{
					oldValue = successorEdge.child;
					newValue = setNull(successorEdge.child);
				}
			}
			result = CAS(successorEdge.parent,which,oldValue,newValue);
		}
		else
		{
			if(i)
			{
				if(dFlag)
				{
					oldValue = setIFlag(setDFlag(successorEdge.child));
					newValue = setDFlag(right);
				}
				else
				{
					oldValue = setIFlag(successorEdge.child);
					newValue = right;
				}
			}
			else
			{
				if(dFlag)
				{
					oldValue = setDFlag(successorEdge.child);
					newValue = setDFlag(right);
				}
				else
				{
					oldValue = successorEdge.child;
					newValue = right;
				}
			}
			result = CAS(successorEdge.parent,which,oldValue,newValue);
		}
		if(result)
		{
			break;
		}	
		if(dFlag)
		{
			break;
		}
		temp=successorEdge.parent->child[which];
		d=isDFlagSet(temp);
		pLastEdge = s->pLastEdge;
		if(d && pLastEdge.parent != NULL)
		{
			helpTargetNode(t,&pLastEdge,state->depth+1);
		}
		findSmallest(t,node,s);
		if(s->lastEdge.child != successorEdge.child)
		{
			//the successor node has already been removed
			break;
		}
		else
		{
			successorEdge = s->lastEdge;
		}
	}
	node->readyToReplace = true;
	updateMode(t,state);
	return;
}

bool cleanup(tArgs* t, StateRecord* state)
{
	Node* parent;
	Node* node;
	Node* newNode;
	Node* left;
	Node* right;
	Node* address;
	Node* temp;
	bool pWhich;
	bool nWhich;
	bool result;
	bool n;
	
	//retrieve the addresses from the state record
	parent = state->targetEdge.parent;
	node = state->targetEdge.child;
	pWhich = state->targetEdge.which;
		
	if(state->type == COMPLEX)
	{
		//replace the node with a new copy in which all the fields are unmarked
		newNode = (Node*) malloc(sizeof(Node));
		newNode->mKey = getKey(node->mKey);
		newNode->readyToReplace = false;
		left = getAddress(node->child[LEFT]);
		newNode->child[LEFT] = left;
		temp=node->child[RIGHT];
		n=isNull(temp);	right=getAddress(temp);
		if(n)
		{
			newNode->child[RIGHT] = setNull(NULL);
		}
		else
		{
			newNode->child[RIGHT] = right;
		}
		
		//switch the edge at the parent
		result = CAS(parent,pWhich,setIFlag(node),newNode);
	}
	else
	{
		//remove the node. determine to which grand child will the edge at the parent be switched
		if(isNull(node->child[LEFT]))
		{
			nWhich = RIGHT;
		}
		else
		{
			nWhich = LEFT;
		}
		temp = node->child[nWhich];
		n = isNull(temp); address = getAddress(temp);
		if(n)
		{
			//set the null flag only; do not change the address
			result = CAS(parent,pWhich,setIFlag(node),setNull(node));
		}
		else
		{
			result = CAS(parent,pWhich,setIFlag(node),address);
		}
	}
	return result;	
}

void initializeTypeAndUpdateMode(tArgs* t, StateRecord* state)
{
	Node* node;
	//retrieve the address from the state record
	node = state->targetEdge.child;
	if(isNull(node->child[LEFT]) || isNull(node->child[RIGHT]))
	{
		//one of the child pointers is null
		if(isKeyMarked(node->mKey))
		{
			state->type = COMPLEX;
		}
		else
		{
			state->type = SIMPLE;
		}
	}
	else
	{
		//both the child pointers are non-null
		state->type= COMPLEX;
	}
	updateMode(t,state);
}

void updateMode(tArgs* t, StateRecord* state)
{
	Node* node;
	//retrieve the address from the state record
	node = state->targetEdge.child;
	
	//update the operation mode
	if(state->type == SIMPLE)
	{
		//simple delete
		state->mode = CLEANUP;
	}
	else
	{
		//complex delete
		if(node->readyToReplace)
		{
			assert(isKeyMarked(node->mKey));
			state->mode = CLEANUP;
		}
		else
		{
			state->mode = DISCOVERY;
		}
	}
	return;
}

void inject(tArgs* t, StateRecord* state)
{
	Node* parent;
	Node* node;
	Edge targetEdge;
	int which;
	bool result;
	bool i;
	bool d;
	bool p;
	Node* temp;
	
	targetEdge = state->targetEdge;
	parent = targetEdge.parent;
	node = targetEdge.child;
	which = targetEdge.which;
	
	result = CAS(parent,which,node,setIFlag(node));
	if(!result)
	{
		//unable to set the intention flag on the edge. help if needed
		temp = parent->child[which];
		i=isIFlagSet(temp);	d=isDFlagSet(temp);	p=isPFlagSet(temp);
		
		if(i)
		{
			helpTargetNode(t,&targetEdge,1);
		}
		else if(d)
		{
			helpTargetNode(t,&state->pTargetEdge,1);
		}
		else if(p)
		{
			helpSuccessorNode(t,&state->pTargetEdge,1);
		}
		return;
	}
	//mark the left edge for deletion
	result = markChildEdge(t,state,LEFT);
	if(!result)
	{
		return;
	}
	//mark the right edge for deletion
	result = markChildEdge(t,state,RIGHT);
	
	//initialize the type and mode of the operation
	initializeTypeAndUpdateMode(t,state);
	return;
}

void findAndMarkSuccessor(tArgs* t, StateRecord* state)
{
	Node* node;
	SeekRecord* s;
	Edge successorEdge;
	bool m;
	bool n;
	bool d;
	bool p;
	bool result;
	Node* temp;
	Node* left;
	
	//retrieve the addresses from the state record
	node = state->targetEdge.child;
	s = &state->successorRecord;
	while(true)
	{
		//read the mark flag of the key in the target node
		m=isKeyMarked(node->mKey);
		//find the node with the smallest key in the right subtree
		findSmallest(t,node,s);
		if(m)
		{
			//successor node has already been selected before the traversal
			break;
		}
		//retrieve the information from the seek record
		successorEdge = s->lastEdge;
		temp = successorEdge.child->child[LEFT];
		n=isNull(temp);	p=isPFlagSet(temp);
        left=getAddress(temp);
		if(!n)
		{
			continue;
		}
		//read the mark flag of the key under deletion
		m=isKeyMarked(node->mKey);
		if(m)
		{
			//successor node has already been selected
			if(p)
			{
				break;
			}
			else
			{
				continue;
			}
		}
		//try to set the promote flag on the left edge
		result = CAS(successorEdge.child,LEFT,setNull(left),setPFlag(setNull(node)));
		if(result)
		{
			break;
		}
		//attempt to mark the edge failed; recover from the failure and retry if needed
		temp = successorEdge.child->child[LEFT];
		n = isNull(temp); d = isDFlagSet(temp); p = isPFlagSet(temp);
		if(p)
		{	
			break;
		}
		if(!n)
		{
			//the node found has since gained a left child
			continue;
		}
		if(d)
		{
			//the node found is undergoing deletion; need to help
			helpTargetNode(t,&s->lastEdge,state->depth+1);
		}
	}
	// update the operation mode
	updateMode(t,state);
	return;
}

void helpSuccessorNode(tArgs* t, Edge* helpeeEdge, int depth)
{
	Node* parent;
	Node* node;
	Node* left;
	StateRecord* state;
	SeekRecord* s;
	// retrieve the address of the successor node
	parent = helpeeEdge->parent;
	node = helpeeEdge->child;
	// promote flag must be set on the successor node's left edge
	// retrieve the address of the target node
	left=getAddress(node->child[LEFT]);
	// obtain new state record and initialize it
	state = (StateRecord*) malloc(sizeof(StateRecord));
	// populateEdge(&state->targetEdge,NULL,left,LEFT);
    state->targetEdge = Edge(NULL, left, LEFT);
	state->depth = depth;
	state->mode = DISCOVERY;
	s = &state->successorRecord;
	// initialize the seek record in the state record
	s->lastEdge = *helpeeEdge;
	// populateEdge(&s->pLastEdge,NULL,parent,LEFT);
    s->pLastEdge = Edge(NULL, parent, LEFT);
	// remove the successor node
	removeSuccessor(t,state);
	return;
}

void createHeadNodes()
{
	R = newLeafNode(INF_R);
	R->child[RIGHT] = newLeafNode(INF_S);
	S = R->child[RIGHT];
	S->child[RIGHT] = newLeafNode(INF_T);
	T = S->child[RIGHT];
}

void nodeCount(Node* node)
{
	if (isNull(node)) {
		return;
	}
	numOfNodes++;

	nodeCount(node->child[LEFT]);
	nodeCount(node->child[RIGHT]);
}

unsigned long size()
{
	numOfNodes=0;
	nodeCount(T->child[LEFT]);
	return numOfNodes;
}

bool isValidBST(Node* node, unsigned long min, unsigned long max)
{
	if(isNull(node))
	{
		return true;
	}
	Node* address = getAddress(node);
	unsigned long nKey = getKey(address->mKey);
	if(nKey > min && nKey < max && isValidBST(address->child[LEFT],min,nKey) && isValidBST(address->child[RIGHT],nKey,max))
	{
		return true;
	}
	return false;
}

bool isValidTree()
{
	return(isValidBST(T->child[LEFT], 0, MAX_KEY));
}



bool search(tArgs* t, unsigned long key)
{
	Node* node;
	unsigned long nKey;
	
	TM_BEGIN(atomic){
	seek(t,key TM_PARAM,&t->targetRecord);
	}TM_END;
	node = t->targetRecord.lastEdge.child;
	nKey = getKey(node->mKey);
	if(nKey == key)
	{
		t->successfulReads++;
		return(true);
	}
	else
	{
		t->unsuccessfulReads++;
		return(false);
	}	
}

TM_CALLABLE
bool insert(tArgs* t, unsigned long key TM_ARG) {
	Node* node;
	Node* newNode;
	Node* address;
	unsigned long nKey;
	bool result;
	int which;
	struct Node* temp;

	while(true)
	{
		TM_BEGIN(atomic){
		seek(t,key TM_PARAM,&t->targetRecord);
		}TM_END;
		node = t->targetRecord.lastEdge.child;
		nKey = getKey(node->mKey);
		TM_BEGIN(atomic){
		if(nKey == TM_READ(key))
		{
			t->unsuccessfulInserts++;
			return(false);
		}
		}TM_END;
		
		//create a new node and initialize its fields
		if(!t->isNewNodeAvailable)
		{
			TM_BEGIN(atomic){
			t->newNode = newLeafNode(TM_READ(key));
			}TM_END;
			t->isNewNodeAvailable = true;
		}
		newNode = t->newNode;
		TM_BEGIN(atomic){
		newNode->mKey = TM_READ(key);
		}TM_END;
		which = t->targetRecord.injectionEdge.which;
		address = t->targetRecord.injectionEdge.child;
		result = CAS(node,which,setNull(address),newNode);
		if(result)
		{
			t->isNewNodeAvailable = false;
			t->successfulInserts++;
			return(true);
		}
		t->insertRetries++;
		
		temp = node->child[which];
		if(isDFlagSet(temp))
		{
			helpTargetNode(t,&t->targetRecord.lastEdge,1);
		}
		else if(isPFlagSet(temp))
		{
			helpSuccessorNode(t,&t->targetRecord.lastEdge,1);
		}
	}
}

TM_CALLABLE
bool remove(tArgs* t, unsigned long key TM_ARG)
{
	StateRecord* myState;
	SeekRecord* targetRecord;
	Edge targetEdge;
	Edge pTargetEdge;
	unsigned long nKey;
	bool result;
	
	//initialize the state record
	myState = &t->myState;
	myState->depth = 0;
	TM_BEGIN(atomic){
	TM_WRITE(myState->targetKey, TM_READ(key));
	TM_WRITE(myState->currentKey, TM_READ(key));
	}TM_END;
	myState->mode = INJECTION;
	targetRecord = &t->targetRecord;
	
	while(true)
	{
		TM_BEGIN(atomic){
		seek(t,myState->currentKey TM_PARAM,targetRecord);
		}TM_END;
		targetEdge = targetRecord->lastEdge;
		pTargetEdge = targetRecord->pLastEdge;
		nKey = getKey(targetEdge.child->mKey);
		if(myState->currentKey != nKey)
		{
			//the key does not exist in the tree
			if(myState->mode == INJECTION)
			{
				t->unsuccessfulDeletes++;
				return(false);
			}
			else
			{
				t->successfulDeletes++;
				return(true);
			}
		}
		//perform appropriate action depending on the mode
		if(myState->mode == INJECTION)
		{
			//store a reference to the target node
			myState->targetEdge = targetEdge;
			myState->pTargetEdge = pTargetEdge;
			//attempt to inject the operation at the node
			inject(t,myState);
		}
		//mode would have changed if the operation was injected successfully
		if(myState->mode != INJECTION)
		{
			//if the target node found by the seek function is different from the one stored in state record, then return
			if(myState->targetEdge.child != targetEdge.child)
			{
				t->successfulDeletes++;
				return(true);
			}
			//update the target edge using the most recent seek
			myState->targetEdge = targetEdge;
		}
		if(myState->mode == DISCOVERY)
		{
			findAndMarkSuccessor(t,myState);
		}
		if(myState->mode == DISCOVERY)
		{
			removeSuccessor(t,myState);
		}
		if(myState->mode == CLEANUP)
		{
			result = cleanup(t,myState);
			if(result)
			{
				t->successfulDeletes++;
				return(true);
			}
			else
			{
				nKey = getKey(targetEdge.child->mKey);
				myState->currentKey = nKey;
			}
		}
	}
}


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

    // each thread must be initialized before running transactions
    TM_THREAD_INIT();

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
			TM_BEGIN(atomic){
            insert(tData, key TM_PARAM);
			}TM_END;
        } else {
			TM_BEGIN(atomic){
            remove(tData, key TM_PARAM);
			}TM_END;
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
			TM_BEGIN(atomic){
            insert(tData, key TM_PARAM);
			}TM_END;
        }
        else
        {
            tData->deleteCount++;
			TM_BEGIN(atomic){
            remove(tData, key TM_PARAM);
			}TM_END;
        }
    }

    TM_THREAD_SHUTDOWN();

    return NULL;
}

tArgs** tArgsArr;

int main(int argc, char **argv) {
	TM_SYS_INIT();

	TM_THREAD_INIT();

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
	TM_BEGIN(atomic){
		insert(initialInsertArgs, (rand() % keyRange + 2) TM_PARAM);
	}TM_END;
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
	printf("maximumKeySize:%d\nfindPercent:%d\ninsertPercent:%d\nremovePercent:%d\nNUM_OF_THREADS:%d\nsize:%ld\ntotalReadCount:%ld\ntotalInsertCount:%ld\ntotalDeleteCount:%ld\ntotalReadRetries:%ld\ntotalSeekRetries:%ld\ntotalInsertRetries:%ld\ntotalDeleteRetries:%ld\nseekPercentage:%.2f\nMOPS:%.2f\n",
        atoi(argv[6]),
        findPercent,
        (insertPercent-findPercent),
        (removePercent-insertPercent),
        NUM_OF_THREADS,
        size(),
        totalReadCount,
        totalInsertCount,
        totalDeleteCount,
        totalReadRetries,
        totalSeekRetries,
        totalInsertRetries,
        totalDeleteRetries,
        totalSeekLength*1.0/totalOperations,
        MOPS);
	assert(isValidTree());
	pthread_exit(NULL);

TM_SYS_SHUTDOWN();
}
