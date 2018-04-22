#pragma once

#include <stdint.h>
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


unsigned long getKey(unsigned long);
Node* getAddress(Node* p);
bool isNull(Node* p);
bool isKeyMarked(unsigned long key);
bool isDFlagSet(Node* p);
bool isPFlagSet(Node* p);
Node* setNull(Node* p);
Node* setDFlag(Node* p);
Node* setPFlag(Node* p);
bool isIFlagSet(Node* p);
Node* setIFlag(Node* p);
unsigned long setReplaceFlagInKey(unsigned long key);

