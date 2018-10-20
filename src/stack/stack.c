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
        node_t *runner = stack->last;
        uint8_t pkt_seqnum = pkt_get_seqnum(pkt);
        int hasMoved = 0;
        while(pkt_seqnum <= runner->seqnum) {
            runner = runner->next;
            hasMoved++;
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
        if(newNode->next == stack->toSend && hasMoved) {
            stack->toSend = newNode;
        }
    }
    stack->size += 1;

    return 0;
}

pkt_t *stack_remove(stack_t *stack, uint8_t seqnum) {
    /* not possible because seqnum loops from 0 to 255
    if(stack->first->seqnum > seqnum) {
        fprintf(stderr, "Node to remove already removed. Stack begins with seqnum %i\n", stack->first->seqnum);
        return NULL;
    }
    if(stack->last->seqnum < seqnum) {
        fprintf(stderr, "Node to remove not yet in stack. Stack ends with seqnum %i\n", stack->last->seqnum);
        return NULL;
    }*/
    node_t *runner = stack->first;
    /*
    // Code to print what's inside the stack
    int go = 1;
    fprintf(stderr, "Trying to remove %i and here is the stack content : ", seqnum);
    while (runner != stack->first || go == 1) {
        fprintf(stderr,"%i,", runner->seqnum);
        runner = runner->next;
        go = 0;
    }
    fprintf(stderr, "\n");

    runner = stack->first;
     */

    while(runner->seqnum != seqnum) {
        runner = runner->next;
        if(runner == stack->first) {
            //fprintf(stderr, "Node to remove (%i) not in stack.\n", seqnum);
            return NULL;
        }
    }
    if(runner == stack->toSend) {
        fprintf(stderr, "Node %i to remove has not yet been send\n", runner->seqnum);
        return NULL;
    }else if(runner == stack->first) {
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
        stack->toSend = NULL;
    }

    runner->next = NULL;
    runner->prev = NULL;

    pkt_t *toReturn = runner->pkt;
    node_free(runner);

    return toReturn;
}

int *stack_remove_acked(stack_t *stack, uint8_t seqnum) {
    // TODO do not forget to free
}

pkt_t *stack_force_remove(stack_t *stack, uint8_t seqnum) {

    node_t *runner = stack->first;

    if (runner == NULL) {
        return NULL;
    }

    while(runner->seqnum != seqnum) {
        runner = runner->next;
        if(runner == stack->first) {
            //fprintf(stderr, "Node to remove not in stack.\n");
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
        stack->toSend = NULL;
    }

    runner->next = NULL;
    runner->prev = NULL;

    pkt_t *toReturn = runner->pkt;
    node_free(runner);

    return toReturn;
}

pkt_t *stack_send_pkt(stack_t *stack, uint8_t seqnum){

    node_t *runner = stack->first;
    while (runner->seqnum != seqnum) {
        runner = runner->next;
        if (runner == stack->first) {
            fprintf(stderr, "Node to send not in stack.\n");
            return NULL;
        }
    }

    pkt_t *toReturn = runner->pkt;
    if (runner->next != runner && runner->next != stack->first) {
        if (stack->toSend == runner) {
            stack->toSend = runner->next;
        }
    } else {
        stack->toSend = NULL;
    }

    return toReturn;
}

pkt_t stack_get_pkt(stack_t *stack, uint8_t seqnum) {
    // TODO : look at stack_get_toSend_seqnum
}

size_t stack_size(stack_t *stack) {
    return stack->size;
}

void stack_free(stack_t *stack) {
    while(stack->first != NULL) {
        pkt_t *returnValue = stack_remove(stack, stack->first->seqnum);
        if(returnValue == NULL) {
            fprintf(stderr, "Could not free stack cleanly. Forcing free (important data can get lost !) ...\n");
            stack->toSend = NULL;
        }
    }
    if(stack_size(stack) != 0) {
        fprintf(stderr, "Exiting stack_free() with stack_size != 0\n");
    }
}

void node_free(node_t *node){
    free(node);
}
