#include "subFunctions.h"

Node* R;
Node* S;
Node* T;
unsigned long numOfNodes;

void seek(tArgs* t, unsigned long key, SeekRecord* s)
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
