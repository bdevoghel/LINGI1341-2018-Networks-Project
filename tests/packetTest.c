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

#include "../src/packet/packet.h"

void testEncodeAndDecodePacket(void) {

    pkt_t *pkt = pkt_new();
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 31);
    pkt_set_seqnum(pkt, 255);
    pkt_set_timestamp(pkt, (uint32_t) 4294967294);
    pkt_set_payload(pkt, "zzabcdefzz", strlen("zzabcdefzz")+1);

    size_t len = 512;
    char *buf = malloc(len);
    pkt_encode(pkt, buf, &len);

    pkt_del(pkt);
    pkt = pkt_new();

    pkt_decode(buf, len, pkt);

    CU_ASSERT_EQUAL((int)(pkt_get_type(pkt)<<6)+(pkt_get_tr(pkt)<<5)+pkt_get_window(pkt), 95);
    CU_ASSERT_EQUAL((int)pkt_get_seqnum(pkt), 255);
    CU_ASSERT_EQUAL((uint32_t)pkt_get_timestamp(pkt), 4294967294);
    CU_ASSERT_FALSE(strcmp(pkt_get_payload(pkt), "zzabcdefzz"));
    CU_ASSERT_EQUAL((int)pkt_get_timestamp(pkt), -2);
    CU_ASSERT_EQUAL(pkt_get_timestamp(pkt), 4294967294);
    CU_ASSERT_EQUAL((int)pkt_get_crc1(pkt), 1791296164);
    CU_ASSERT_EQUAL((int)pkt_get_crc2(pkt), -1515869241);
    pkt_del(pkt);
    free(buf);
}


int init_suite2(void){
    return 0;
}


int clean_suite2(void){
    return 0;
}

int main()
{
    CU_pSuite pSuite = NULL;


    /* initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();


    /* add a suite to the registry */
    pSuite = CU_add_suite("Packet Tests", init_suite2, clean_suite2);
    if (NULL == pSuite) {
        CU_cleanup_registry(); //libérer les resources
        return CU_get_error();
    }


    /* add the tests to the suite */
    if  (NULL == CU_add_test(pSuite, "Test encode decode packet\n", testEncodeAndDecodePacket))
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
