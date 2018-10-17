/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Tests unitaires
 */

// TODO

#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>
#include <CUnit/Automated.h>
#include <CUnit/CUnit.h>

#include "../src/stack/stack.h"
#include "../src/packet/packet.h"


void testInsertIntoStack(void) {
    stack_t *stack = NULL;
    stack_init(stack);

    pkt_t *packet = pkt_new();
    stack_enqueue(stack, packet);
    stack_enqueue(stack, packet);

    CU_ASSERT_EQUAL(stack_size(stack), 2);
}

void testRemoveFromStack(void) {
    stack_t *stack = NULL;
    stack_init(stack);

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
}

/**
 * TEST FOR PACKET.C
 */
/*
int main(int argc, char* argv[]) {
    fprintf(stderr, "--- TESTING PACKET ---\nUnused parameters : %i %s\n", argc-1, argv[1]);

    pkt_t *pkt = pkt_new();
    pkt_set_type(pkt, PTYPE_DATA);
    pkt_set_tr(pkt, 0);
    pkt_set_window(pkt, 31);
    pkt_set_seqnum(pkt, 255);
    //pkt_set_length(pkt, strlen("zzabcdefzz")); // done in set_payload
    pkt_set_timestamp(pkt, (uint32_t) 4294967294);
    //uint32_t crc1 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *) pkt+0, 8);
    //pkt_set_crc1(pkt, crc1);
    pkt_set_payload(pkt, "zzabcdefzz", strlen("zzabcdefzz")+1);
    //uint32_t crc2 = crc32(crc32(0L, Z_NULL, 0), (const Bytef *) pkt+12, pkt_get_length(pkt));
    //pkt_set_crc2(pkt, crc2);

    size_t len = 512;
    char *buf = malloc(len);
    //buf[511] = '\0';
    pkt_encode(pkt, buf, &len); //len-Post nombre octets écrits

    pkt_del(pkt);
    pkt = pkt_new();

    pkt_decode(buf, len, pkt);
    fprintf(stderr, "TypeTrWin : %02x\n", (pkt_get_type(pkt)<<6)+(pkt_get_tr(pkt)<<5)+pkt_get_window(pkt));
    fprintf(stderr, "Seqnum    : %02x\n", pkt_get_seqnum(pkt));
    fprintf(stderr, "Length    : %04x\n", pkt_get_length(pkt));
    fprintf(stderr, "Timestamp : %08x\n", pkt_get_timestamp(pkt));
    fprintf(stderr, "CRC1      : %08x\n", pkt_get_crc1(pkt));
    fprintf(stderr, "Payload   : %s\n", pkt_get_payload(pkt));
    fprintf(stderr, "CRC2      : %08x\n", pkt_get_crc2(pkt));

    return 0;
}

 WITH AT THE END OF pkt_encode() :

    fprintf(stderr, "HEX ENCODE : " );
    int i;
    for(i = 0 ; i < (int) *len ; i++ ) {
        fprintf(stderr, " %02x ", (unsigned char) *(buf+i));
    }
    fprintf(stderr, "\n" );

WITH AT THE BEGGINING OF pkt_decode() :

    fprintf(stderr, "HEX DECODE : " );
    int i;
    for (i =0 ; i<(int) len; i++ ) {
        fprintf(stderr, " %02x ", (unsigned char) *(data+i));
    }
    fprintf(stderr, "\n" );

 AND MAKEFILE :

# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

# We have no libraries to link against except libc, but we want to keep the symbols for debugging
LDFLAGS= -rdynamic

# Default target
all: clean packet
test : all
	./packet

# If we run `make debug` instead, keep the debug symbols for gdb and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: clean packet

# We use an implicit rule to build an executable named 'chat'
packet: -lz packet.o

.PHONY: clean

clean:
	@rm -vf packet packet.o

 */