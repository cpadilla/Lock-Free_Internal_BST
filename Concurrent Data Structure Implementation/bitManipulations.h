#pragma once

#include<stdint.h>

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