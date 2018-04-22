#pragma once

#include "header.h"
#include "bitManipulations.h"

bool CAS(Node* parent, int which, Node* oldChild, Node* newChild);
Node* newLeafNode(unsigned long key);


