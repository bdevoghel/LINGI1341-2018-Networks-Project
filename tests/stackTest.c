/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Tests unitaires
 */

#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>
#include <CUnit/Automated.h>
#include <CUnit/CUnit.h>

#include "../src/stack/stack.h"
#include "../src/packet/packet.h"


// TODO : check for memory leaks ! (use valgrind to check)

void testInsertIntoStack(void) {
    stack_t *stack = stack_init();

    pkt_t *packet = pkt_new();
    stack_enqueue(stack, packet);
    stack_enqueue(stack, packet);

    CU_ASSERT_EQUAL(stack_size(stack), 2);

    stack_free(stack);
}

void testRemoveFromStack(void) {
    stack_t *stack = stack_init();

    pkt_t *packet1 = pkt_new();
    pkt_set_seqnum(packet1, 10);
    pkt_set_timestamp(packet1, 987654321);
    pkt_t *packet2 = pkt_new();
    pkt_set_seqnum(packet2, 12);
    pkt_set_timestamp(packet2, 123456789);

    stack_enqueue(stack, packet1);
    stack_enqueue(stack, packet2);

    pkt_t *pkt = stack_remove(stack, 12);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 123456789);

    stack_free(stack);
}


int init_suite1(void){
    return 0;
}


int clean_suite1(void){
    return 0;
}

int main()
{
    CU_pSuite pSuite = NULL;


    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();


    /* add a suite to the registry */
    pSuite = CU_add_suite("Stack tests", init_suite1, clean_suite1);
    if (NULL == pSuite) {
        CU_cleanup_registry(); //libérer les resources
        return CU_get_error();
    }


    /* add the tests to the suite */
    if  (
            NULL == CU_add_test(pSuite, "Test insert into stack\n", testInsertIntoStack)
            ||
            NULL == CU_add_test(pSuite, "Test remove from stack\n", testRemoveFromStack)
        )
    {
        CU_cleanup_registry();
        return CU_get_error();
    }


    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE); //un maximum de details
    CU_basic_run_tests(); //lancer les tests
    CU_cleanup_registry(); //laver le registre
    return CU_get_error(); //retourne erreurs
}
