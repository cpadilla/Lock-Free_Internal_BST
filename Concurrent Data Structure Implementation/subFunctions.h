#include "header.h"
#include "bitManipulations.h"

Node* R;
Node* S;
Node* T;

void seek(struct tArgs* t, unsigned long key, SeekRecord* s)
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
			if(key == cKey || n)
			{
				//either key found or no next edge to follow. Stop the traversal
				s->pLastEdge = pLastEdge;
				s->lastEdge = lastEdge;
				// populateEdge(&s->injectionEdge,curr,next,which);
                s->injectionEdge = Edge(curr, next, which);
				if(key == cKey)
				{
					//key matches. So return
					return;					
				}
				else
				{
					break;
				}
			}
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
                s = new SeekRecord(&t->pSeekRecord);
				return;
			}
		}
		//store the results of the current traversal and restart
        t->pSeekRecord = SeekRecord(s);
		pAnchorRecord->node = anchorRecord->node;
		pAnchorRecord->key = anchorRecord->key;
		t->seekRetries++;
	}
}