#include "bitManipulations.h"

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
