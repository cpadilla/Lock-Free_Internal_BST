#include "header.h"


Edge::Edge() {}

Edge::Edge(Node* parent, Node* child, Side which) {
        this->parent = parent;
        this->child = child;
        this->which = which;
}


    SeekRecord::SeekRecord(SeekRecord* s) {
        this->lastEdge = Edge(s->lastEdge.parent, s->lastEdge.child, s->lastEdge.which);
        this->pLastEdge = Edge(s->pLastEdge.parent, s->pLastEdge.child, s->pLastEdge.which);
        this->injectionEdge = Edge(s->injectionEdge.parent, s->injectionEdge.child, s->injectionEdge.which);
    }


