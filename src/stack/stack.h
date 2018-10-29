/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 */

#ifndef CODE_STACK_H
#define CODE_STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h> //size_t
#include <stdint.h> // uintx_t
#include <string.h> // memcpy

#include "../packet/packet.h"

typedef struct node {
    struct node *next;
    struct node *prev;
    struct pkt *pkt;
    uint8_t seqnum;
} node_t;

typedef struct stack {
    struct node *first;
    struct node *last;
    size_t size;
} stack_t;

/**
 * initialises stack
 * @return : ptr to callocated stack, NULL if calloc failed
 */
stack_t *stack_init();

/**
 * adds new node in [stack] with [pkt] inside, keeps the stack sorted (first < last), does not accept
 * @param node
 * @param pkt
 * @return : 0 if succeeds, 1 otherwise (stack not modified)
 */
int stack_enqueue(stack_t *stack, pkt_t *pkt);

/**
 * removes first node with seqnum [seqnum] from [stack] starting from [stack->first]
 * @param stack
 * @param seqnum
 * @return : ptr to [pkt] inside removed node, NULL if failed
 */
pkt_t *stack_remove(stack_t *stack, uint8_t seqnum);

/**
 * removes all nodes from first until seqnum [seqnum] (not included) from [stack]
 * @param stack
 * @param seqnum
 * @return : number of removed nodes
 */
int stack_remove_acked(stack_t *stack, uint8_t seqnum);

/**
 * returns first pkt_t with seqnum [seqnum] starting from [stack->first]
 * @param stack
 * @return : [pkt] with [seqnum], NULL if seqnum not in [stack]
 */
pkt_t *stack_get_pkt(stack_t *stack, uint8_t seqnum);

/**
 * indicates number of nodes in [stack]
 * @param stack
 * @return : number of nodes stocked in [stack]
 */
size_t stack_size(stack_t *stack);

int is_in_stack(stack_t *stack, uint8_t seqnum);

/**
 * frees the whole stack and its content
 * @param stack
 * @return
 */
void stack_free(stack_t *stack);

/**
 * frees the node and its content
 * @param stack
 * @return
 */
void node_free(node_t *node);

#endif //CODE_STACK_H
