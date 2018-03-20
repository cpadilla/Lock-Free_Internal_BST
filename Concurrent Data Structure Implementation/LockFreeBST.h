#include "header.h"
#include "subFunctions.h"

bool search(struct tArgs* t, unsigned long key)
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
