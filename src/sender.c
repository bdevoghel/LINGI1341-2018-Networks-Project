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

#include "socket/real_address.h"
#include "socket/create_socket.h"
#include "socket/read_write_loop_sender.h"
#include "socket/wait_for_client.h"

#include "packet/packet.h"
#include "stack/stack.h"

char *hostname = NULL;
int port = -1;
char *fileToRead = NULL;
int socketFileDescriptor;

int fOption = 0;

uint8_t nextSeqnum;
stack_t *sendingStack;

/**
 * pour facilement expliquer une erreur et quitter le programme
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
    nextSeqnum = 0;
    sendingStack = NULL;

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
     * Add terminating connexion packet
     */
    pkt_t *terminateConnexionPkt = pkt_new();
    pkt_set_type(terminateConnexionPkt, PTYPE_DATA);
    pkt_set_window(terminateConnexionPkt, 0);
    pkt_set_seqnum(terminateConnexionPkt, nextSeqnum);
    pkt_set_length(terminateConnexionPkt, 0);

    statusCode = stack_enqueue(sendingStack, terminateConnexionPkt);
    if (statusCode != 0) {
        return ooops("Error in stack_enqueue()");
    }

    /*
     * Send packets (first 1 and then as much as receiver's window can accept) and wait for their ACK / NACK
     *
     * Algorithm :
     * - send packet(s), start retransmission timer(s) (RT)
     * - wait for ACK                   OR            NACK                           OR            RTO
     * - receive ACK                                  - receive NACK (buffered but not complete)   - RT runs out
     * - adapt window                                 - resend seqnum received by NACK             - resend same packet
     * - if (possible) send next packet                 - <==
     *    else (window==0) wait for a RTO and then send
     *
     * RTO :
     * - init when packet send
     * - reset when packet resend
     * - when run out : resend packet and reset
     */

    /*
     * socketFileDescriptor : socket connexion on wich messages are written
     * sendingStack         : stack in wich the packets to send are stored in increasing order
     */
    int numberOfPackets = (int) stack_size(sendingStack);
    statusCode = read_write_loop_sender(socketFileDescriptor, sendingStack, numberOfPackets);
    if(statusCode != 0) {
        return statusCode;
    }
    fprintf(stderr, "Packets sent successfully.\n");


    close(socketFileDescriptor);
    // TODO : terminate connexion

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
        if(strcmp(argv[i], "-f") == 0) {
            i++;
            if(!fOption) {
                ooops("You need to put the -f option before the other arguments. Reading from stdin.");
            }
        } else if(hostnameSet == 0) {
            hostname = argv[i];
            hostnameSet = 1;
        } else if(portSet == 0) {
            port = atoi(argv[i]); // NOLINT
            portSet = 1;
        } else {
            return ooops("Unknown argument. Usage : \"receiver hostname port [-f X]\"");
        }
        i++;
    }

    if(!hostnameSet || !portSet) {
        fprintf(stderr, "%i option(s) read. Usage : \"sender hostname port [-f X]\"\n", (1 + fOption*2 + hostnameSet + portSet));
        return EXIT_FAILURE;
    }

    if(!fOption) {
        fileToRead = "stdin"; // TODO read stdin
    }

    fprintf(stderr, "Options processed.\n"
                    "   Hostname      : %s\n"
                    "   Port          : %i\n"
                    "   File to read  : %s\n",
            hostname, port, fileToRead);
    return EXIT_SUCCESS;
}

int init_connexion() {
    if(hostname == NULL || port < 0) {
        return ooops("Hostname is NULL or destination port is negative");
    }

    struct sockaddr_in6 address;

    if(real_address(hostname, &address) != NULL) {
        return ooops("Unable to resolve hostname");
    }

    // Create a socket
    socketFileDescriptor = create_socket(NULL, -1, &address, port);
    if(socketFileDescriptor < 0) {
        return ooops("Error while creating socket");
    }

    return EXIT_SUCCESS;
}

int read_file() {
    int fd = open(fileToRead, O_RDWR);
    if (fd < 0) {
        return ooops("Could not open file to read");
    }

    char buf[MAX_PAYLOAD_SIZE];

    int justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
    if (justRead < 0) {
        return ooops("Error when reading");
    }

    while (justRead != 0) {
        pkt_t *packet = pkt_new();
        if (packet == NULL) {
            return ooops("Out of memory at packet creation");
        }

        int statusCode;

        statusCode = pkt_set_type(packet, PTYPE_DATA);
        if (statusCode != PKT_OK) {
            return ooops("Error in pkt_set_type()");
        }

        statusCode = pkt_set_tr(packet, 0);
        if (statusCode != PKT_OK) {
            return ooops("Error in pkt_set_tr()");
        }

        statusCode = pkt_set_seqnum(packet, nextSeqnum);
        if (statusCode != PKT_OK) {
            return ooops("Error in pkt_set_seqnum()");
        }
        increment_nextSeqnum();

        statusCode = pkt_set_payload(packet, buf, (uint16_t) justRead);
        if (statusCode != PKT_OK) {
            return ooops("Error in pkt_set_payload()");
        }

        statusCode = stack_enqueue(sendingStack, packet);
        if (statusCode != 0) {
            return ooops("Error in stack_enqueue()");
        }

        justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
        if (justRead < 0) {
            return ooops("Error when reading");
        }
    }

    close(fd);

    fprintf(stderr, "File read successfully. Ready to transmit.\n");
    return EXIT_SUCCESS;
}

void increment_nextSeqnum() {
    if(nextSeqnum == 255) {
        nextSeqnum = 0;
    } else {
        nextSeqnum += 1;
    }
}
