#include "header.h"
#include "util.h"
#include "subFunctions.h"

bool search(tArgs* t, unsigned long key)
{
	Node* node;
	unsigned long nKey;
	
	seek(t,key,&t->targetRecord);
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

bool insert(tArgs* t, unsigned long key) {
	Node* node;
	Node* newNode;
	Node* address;
	unsigned long nKey;
	bool result;
	int which;
	struct Node* temp;

	while(true)
	{
		seek(t,key,&t->targetRecord);
		node = t->targetRecord.lastEdge.child;
		nKey = getKey(node->mKey);
		if(nKey == key)
		{
			t->unsuccessfulInserts++;
			return(false);
		}
		
		//create a new node and initialize its fields
		if(!t->isNewNodeAvailable)
		{
			t->newNode = newLeafNode(key);
			t->isNewNodeAvailable = true;
		}
		newNode = t->newNode;
		newNode->mKey = key;
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
