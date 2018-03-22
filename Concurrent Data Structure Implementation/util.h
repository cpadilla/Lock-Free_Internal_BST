#pragma once

#include "header.h"
#include "bitManipulations.h"

static bool CAS(Node* parent, int which, Node* oldChild, Node* newChild)
{
	return parent->child[which].compare_exchange_strong(oldChild,newChild,std::memory_order_seq_cst);
}

static Node* newLeafNode(unsigned long key)
{
  Node* node = (Node*) malloc(sizeof(Node));
  node->mKey = key;
  node->child[LEFT] = setNull(NULL); 
  node->child[RIGHT] = setNull(NULL);
	node->readyToReplace=false;
  return(node);
}