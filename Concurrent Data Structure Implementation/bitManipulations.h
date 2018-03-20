#pragma once

#include<stdint.h>
#include "header.h"

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

bool isDFlagSet(Node* p)
{
	return ((uintptr_t) p & DELETE_BIT) != 0;
}

bool isPFlagSet(Node* p)
{
	return ((uintptr_t) p & PROMOTE_BIT) != 0;
}