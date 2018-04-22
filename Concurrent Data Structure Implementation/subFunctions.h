#pragma once

#include "header.h"
#include "util.h"

void seek(tArgs*, unsigned long, SeekRecord*);
void initializeTypeAndUpdateMode(tArgs*, StateRecord*);
void updateMode(tArgs*, StateRecord*);
void inject(tArgs*, StateRecord*);
void findSmallest(tArgs*, Node*, SeekRecord*);
void findAndMarkSuccessor(tArgs*, StateRecord*);
void removeSuccessor(tArgs*, StateRecord*);
bool cleanup(tArgs*, StateRecord*);
bool markChildEdge(tArgs*, StateRecord*, bool);
void helpTargetNode(tArgs*, Edge*, int);
void helpSuccessorNode(tArgs*, Edge*, int);
void createHeadNodes();
void nodeCount();
unsigned long size();
bool isValidBST(Node* node, unsigned long min, unsigned long max);
bool isValidTree();

