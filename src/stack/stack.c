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
    } else {
        node_t *runner = stack->last;
        uint8_t pkt_seqnum = pkt_get_seqnum(pkt);
        while(pkt_seqnum < runner->seqnum && !(runner->seqnum > 200 && pkt_seqnum < 100)) { // condition for separating two blocks of 256
            runner = runner->prev;
        }
        // insert in front of runner
        newNode->next = runner->next;
        newNode->prev = runner;
        runner->next->prev = newNode;
        runner->next = newNode;
        if(runner == stack->last) {
            stack->last = newNode;
        } else if(newNode->next == stack->first) {
            stack->first = newNode;
        }
    }
    stack->size += 1;

    return 0;
}

pkt_t *stack_remove(stack_t *stack, uint8_t seqnum) {
    node_t *runner = stack->first;
    if(runner == NULL) {
        return NULL;
    }

    while(runner->seqnum != seqnum) {
        runner = runner->next;
        if(runner == stack->first) {
            fprintf(stderr, "Node to remove (%i) not in stack.\n", seqnum);
            return NULL;
        }
    }
    if(runner == stack->first) {
        runner->prev->next = runner->next;
        runner->next->prev = runner->prev;
        stack->first = stack->first->next;

    } else if(runner == stack->last) {
        runner->prev->next = runner->next;
        runner->next->prev = runner->prev;
        stack->last = stack->last->prev;

    } else {
        runner->prev->next = runner->next;
        runner->next->prev = runner->prev;
    }
    stack->size -= 1;

    if(stack->size == 0) {
        stack->first = NULL;
        stack->last = NULL;
    }

    runner->next = NULL;
    runner->prev = NULL;

    pkt_t *toReturn = runner->pkt;
    node_free(runner);

    return toReturn;
}

int stack_remove_acked(stack_t *stack, uint8_t seqnum) {
    node_t *runner = stack->first;
    int count = 0;
    while(runner->seqnum != seqnum) {
        node_t *toRemove = runner;
        runner = runner->next;
        stack->last->next = runner;
        stack->first = runner;
        runner->prev = stack->last;

        pkt_del(toRemove->pkt);
        node_free(toRemove);
        count++;
    }
    stack->size -= count;
    return count;
}

pkt_t *stack_get_pkt(stack_t *stack, uint8_t seqnum) {
    if(stack->size == 0) {
        return NULL;
    }

    node_t *runner = stack->first;
    while(runner->seqnum < seqnum) {
        runner = runner->next;
    }
    return runner->pkt;
}

size_t stack_size(stack_t *stack) {
    return stack->size;
}

void stack_free(stack_t *stack) {
    while(stack->first != NULL) {
        pkt_del(stack_remove(stack, stack->first->seqnum));
    }
    if(stack_size(stack) != 0) {
        fprintf(stderr, "Exiting stack_free() with stack_size != 0\n");
    }

    free(stack);
}

void node_free(node_t *node){
    free(node);
}
