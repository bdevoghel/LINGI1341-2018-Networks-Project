/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

// TODO : ensure no memory leakage !!

#include "socket/real_address.h"
#include "socket/create_socket.h"
#include "socket/read_write_loop.h"
#include "socket/wait_for_client.h"
#include "packet/packet.h"
#include "stack/stack.h"

char *hostname = NULL;
int port = -1;
char *fileToRead = NULL;

int fOption = 0;
uint8_t nextSeqnum = 0;
uint8_t nextWindow = 31;

stack_t *sendingStack = NULL;

int socketFileDescriptor;

/**
 * expliquer une erreur facilement et quitter le programme
 * @param message
 * @return
 */
int ooops(char *message) {
    fprintf(stderr, "%s\n", message);
    return EXIT_FAILURE;
}

/**
 * TODO : description
 * @param argc
 * @param argv
 * @return
 */
int process_options(int argc,char *argv[]);

/**
 * TODO : description
 */
int init_connexion();

/**
 * TODO : description
 * @return
 */
int read_file();

/**
 * TODO : description
 */
void increment_nextSeqnum();

/**
 * TODO : description
 */
void set_nextWindow();

/**
 * sender permet de de realiser un transfer de donnees unidirectionnel et fiable
 *
 * utilisation : "sender hostname port [-f X]"
 * il envoie vers l'adresse [hostname] sur laquelle ecoute le receiver avec le port [port] sur lequel le receiver a ete lance
 * si -f est present, les donnees seront recuperees du fichier X, sinon elles le seront depuis stdin
 *
 * @param argc : 3 < argc < 5
 * @param argv : sender hostname port [-f X]
 * @return : EXIT_SUCCESS si success
 *           EXIT_FAILURE si erreur d'execution OU arguments non coherents
 */
int main(int argc, char *argv[]) {
    int statusCode;

    /*
     * Reading arguments
     */
    statusCode = process_options(argc, argv);
    if(statusCode != 0) {
        return statusCode;
    }

    /*
     * Resolve the hostname, create socket, link sender & receiver and initializes connexion
     */
    statusCode = init_connexion();
    if(statusCode != 0) {
        return statusCode;
    }

    /*
     * Segmentation of file into [MAX_PAYLOAD_SIZE] buffers, stocked on [sendingStack] with increasing seqnum
     */
    sendingStack = stack_init();
    if(sendingStack == NULL) {
        return ooops("Out of memory at stack creation");
    }

    statusCode = read_file();
    if(statusCode != 0) {
        return statusCode;
    }

    /*
     * Send packets (first 1 and then as much as receiver's window can accept) and wait for their ACK / NACK
     *
     * Algorithm :
     * - send packet(s), start retransmission timer(s) (RT)
     * - wait for ACK
     * - receive ACK                                  - receive NACK (buffered but not complete)   - RT runs out
     * - adapt window                                 - resend seqnum received by NACK             - resend same packet
     * - if possible send next packet                 - <==
     *   else (window==0) wait for 1 RT and then send
     * - <==
     *
     * RT :
     * - init when packet send
     * - reset when packet resend
     * - when run out : resend packet and reset
     */


    stack_free(sendingStack);
    // TODO other things to free ? memory leaks !!!

    return EXIT_SUCCESS;
}

int process_options(int argc,char *argv[]) {
    int opt;
    while((opt = getopt(argc, argv, "f:")) != -1) {
        switch(opt) {
            case 'f' :
                fOption = 1;
                fileToRead = optarg;
                break;
            default : // unknown option
                return ooops("Unknown argument. Usage : \"receiver hostname port [-f X]\"");
        }
    }

    int i = 1;
    int hostnameSet = 0;
    int portSet = 0;
    while(i < argc) {
        if(!strcmp(argv[i], "-f")) {
            i++;
        } else if(!hostnameSet) {
            hostname = argv[i];
            hostnameSet = 1;
        } else {
            port = atoi(argv[i]); // NOLINT
            portSet = 1;
        }
        i++;
    }

    if(argc > (3 + fOption*2) || !hostnameSet || !portSet) {
        fprintf(stderr, "%i option(s) read. Usage : \"receiver hostname port [-f X]\"\n", (1 + fOption*2 + hostnameSet + portSet));
        return EXIT_FAILURE;
    }

    if(!fOption) {
        fileToRead = NULL; // TODO read stdin
    }

    fprintf(stderr, "Options processed.\n   Hostname     : %s\n   Port         : %i\n   File to read : %s\n", hostname, port, fileToRead);
    return EXIT_SUCCESS;
}

int init_connexion() { // TODO return value ? what is the result ?
    if (hostname == NULL || port < 0) {
        return ooops("Hostname is NULL or destination port is negative");
    }

    struct sockaddr_in6 address;

    const char *realAddressResult = real_address(hostname, &address);
    if (realAddressResult != NULL) {
        return ooops("Unable to resolve hostname");
    }

    // Create a socket
    socketFileDescriptor = create_socket(NULL, -1, &address, port);
    if (socketFileDescriptor == -1) {
        return ooops("Error while creating the socket");
    }
    fprintf(stderr, "I wait here !! 1\n");

    int waitForClientResult = wait_for_client(socketFileDescriptor);
    if (waitForClientResult == -1) {
        return ooops("Error while waiting for the client");
    }
    fprintf(stderr, "I was here !! 2\n");


    return EXIT_SUCCESS; // TODO check value to return
}

int read_file() {
    if (fileToRead) {
        int fd = open(fileToRead, O_RDWR);
        if (fd == -1) {
            return ooops("Couldn't open file to read");
        }

        char buf[MAX_PAYLOAD_SIZE];

        int justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
        if(justRead == -1) {
            ooops("Error when reading");
        }
        while (justRead != 0) {
            pkt_t *packet = pkt_new();
            if (packet == NULL) {
                ooops("Out of memory at packet creation");
            }

            int statusCode;

            statusCode = pkt_set_type(packet, PTYPE_DATA);
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_type()");
            }

            statusCode = pkt_set_tr(packet, 0);
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_tr()");
            }

            statusCode = pkt_set_window(packet, nextWindow);
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_window()");
            }
            set_nextWindow();

            statusCode = pkt_set_seqnum(packet, nextSeqnum);
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_seqnum()");
            }
            increment_nextSeqnum();

            statusCode = pkt_set_payload(packet, buf, (const uint16_t) justRead);
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_payload()");
            }

            statusCode = pkt_set_timestamp(packet, (const uint32_t) time(NULL));
            if(statusCode != PKT_OK) {
                ooops("Error in pkt_set_timestamp()");
            }

            statusCode = stack_enqueue(sendingStack, packet);
            if(statusCode != 0) {
                ooops("Error in stack_enqueue()");
            }

            justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
            if(justRead == -1) {
                ooops("Error when reading");
            }
        }

        close(fd);
    }

    return EXIT_SUCCESS; // TODO check value to return
}

void increment_nextSeqnum() {
    if(nextSeqnum == 255) {
        nextSeqnum = 0;
    } else {
        nextSeqnum += 1;
    }
}

void set_nextWindow() {
    // TODO implementation : get next window size based on how many packets were send and are awaiting ACK
}
