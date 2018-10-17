/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 */

#include "stack.h"

stack_t *stack_init() {
    return (stack_t *) calloc(1, sizeof(stack_t));
}

int stack_enqueue(stack_t *stack, pkt_t *pkt) {
    node_t *newNode = (node_t *) malloc(sizeof(node_t));
    if(newNode == NULL) {
        fprintf(stderr, "Out of memory at node creation\n");
        return -1;
    }
    newNode->pkt = pkt;
    newNode->seqnum = pkt_get_seqnum(pkt);
    if(stack->size == 0) {
        newNode->next = newNode;
        newNode->prev = newNode;
        stack->first = newNode;
        stack->last = newNode;
        stack->toSend = newNode;
    } else {
        newNode->next = stack->last->next;
        newNode->prev = stack->last;
        stack->last->next->prev = newNode;
        stack->last->next = newNode;
        stack->last = newNode;
    }
    stack->size += 1;

    return 0;
}

pkt_t *stack_remove(stack_t *stack, uint8_t seqnum) {
    if(stack->first->seqnum > seqnum) {
        fprintf(stderr, "Node to remove was already removed. Stack begins with seqnum %i\n", stack->first->seqnum);
        return NULL;
    }
    if(stack->last->seqnum < seqnum) {
        fprintf(stderr, "Node to remove is not yet in stack. Stack ends with seqnum %i\n", stack->last->seqnum);
        return NULL;
    }

    node_t *runner = stack->first;
    while(runner->seqnum != seqnum) {
        runner = runner->next;
    }
    if(runner == stack->first) {
        if(runner == stack->toSend) {
            fprintf(stderr, "Node to remove has not yet been send\n");
            return NULL;
        } else {
            runner->prev->next = runner->next;
            runner->next->prev = runner->prev;
            stack->first = stack->first->next;
        }
    } else if(runner == stack->last) {
        if(runner == stack->toSend) {
            fprintf(stderr, "Node to remove has not yet been send\n");
            return NULL;
        } else {
            runner->prev->next = runner->next;
            runner->next->prev = runner->prev;
            stack->last = stack->last->prev;
        }
    } else {
        if(runner == stack->toSend) {
            fprintf(stderr, "Node to remove has not yet been send\n");
            return NULL;
        } else {
            runner->prev->next = runner->next;
            runner->next->prev = runner->prev;
        }
    }
    stack->size -= 1;

    if(stack->size == 0) {
        stack->first = NULL;
        stack->last = NULL;
        stack->toSend = NULL;
    }

    runner->next = NULL;
    runner->prev = NULL;

    pkt_t *toReturn = runner->pkt;
    node_free(runner);

    return toReturn;
}

size_t stack_size(stack_t *stack) {
    return stack->size;
}

void stack_free(stack_t *stack) {
    while(stack->first != NULL) {
        stack_remove(stack, stack->first->seqnum);
    }
}

void node_free(node_t *node){
    pkt_del(node->pkt);
}
