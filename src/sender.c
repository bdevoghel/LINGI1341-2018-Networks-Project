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
 * For printing error messages and returning EXIT_FAILURE easily
 * @param message : message to print on stderr
 * @return : EXIT_FAILURE
 */
int ooops(char *message) {
    fprintf(stderr, "%s\n", message);
    return EXIT_FAILURE;
}

/**
 * Processes arguments and options received with main
 * @param argc
 * @param argv
 * @return : 0 if options processed successfully, 1 if something went wrong
 */
int process_options(int argc,char *argv[]);

/**
 * Resolves the hostname & creates a socket
 */
int init_connection();

/**
 * Segmentation of [file] into [MAX_PAYLOAD_SIZE] buffers, stocked on [stack] with increasing seqnum
 * @param file : name of the file to read from
 * @param stack
 * @return : 0 if file read successfully with packets stocked on [stack], 1 if something went wrong
 */
int read_file(char *file, stack_t *stack);

/**
 * Increments [seqNum] without exceeding 255 (256 => 0)
 */
void increment_nextSeqnum();

/**
 * Sender permet de de realiser un transfer de donnees unidirectionnel et fiable
 *
 * Utilisation : "./sender [-f X] hostname port"
 * Il envoie vers l'adresse [hostname] sur laquelle ecoute le receiver avec le port [port] sur lequel le receiver a ete lance
 * Si -f est present, les donnees seront recuperees du fichier X, sinon elles le seront depuis stdin
 *
 * @param argc : 3 < argc < 5
 * @param argv : sender [-f X] hostname port
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
     * Resolve the hostname, create socket, link sender & receiver and initializes connection
     */
    statusCode = init_connection();
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

    statusCode = read_file(fileToRead, sendingStack);
    if(statusCode != 0) {
        return statusCode;
    }


    /* decomment if use with "./sender [...] > log.txt"
    printf("State of sendingStack : \n     size        : %li\n", stack_size(sendingStack));
    node_t *runner = sendingStack->first;
    int looop = 1;
    while(looop) {
        printf("     node seqnum : %i\t pkt length  : %i\t prev seqnum : %i\t next seqnum : %i\n", pkt_get_seqnum(runner->pkt), pkt_get_length(runner->pkt), runner->prev->seqnum, runner->next->seqnum);
        runner = runner->next;
        if(runner == sendingStack->first) {
            looop = 0;
        }
    }
    printf("ALL STACK LOG PRINTED\n");
    fflush(stdout);
    */


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
     * socketFileDescriptor : socket connection on wich messages are written
     * sendingStack         : stack in wich the packets to send are stored in increasing order
     */
    int loop = 0;
    statusCode = EXIT_FAILURE;
    while(loop < 10 && statusCode == EXIT_FAILURE) { // tries at least 10 times
        statusCode = read_write_loop_sender(socketFileDescriptor, sendingStack);
        if(statusCode == EXIT_FAILURE && loop < 9) { // receiver was probably not launched yet
            loop++;
            sleep(1); // wait 1 second before trying again
        }
    }
    fprintf(stderr, "Packets sent successfully.\n");

    fprintf(stderr, GRN"=> CLOSING CONNECTION" RESET "\n\n");

    // TODO : delink, debound and deconnect connection properly ?

    close(socketFileDescriptor);

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
                return ooops("Unknown argument. Usage : \"receiver [-f X] hostname port\"");
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
            return ooops("Unknown argument. Usage : \"receiver [-f X] hostname port\"");
        }
        i++;
    }

    if(!hostnameSet || !portSet) {
        fprintf(stderr, "%i option(s) read. Usage : \"sender hostname port [-f X]\"\n", (1 + fOption*2 + hostnameSet + portSet));
        return EXIT_FAILURE;
    }

    if(!fOption) {
        fileToRead = "stdin";
    }

    fprintf(stderr, "Options processed.\n"
                    "   Hostname      : %s\n"
                    "   Port          : %i\n"
                    "   File to read  : %s\n",
            hostname, port, fileToRead);
    return EXIT_SUCCESS;
}

int init_connection() {
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

int read_file(char *file, stack_t *stack) {
    int fd;

    if (!strcmp(file, "stdin")) {
        fd = open("stdin.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        char buffer[512];
        int justRead = (int) read(STDIN_FILENO, buffer, 512);
        while (justRead != 0) {
            int justWrote = (int) write(fd, buffer, (size_t) justRead);
            if (justWrote == -1) {
                fprintf(stderr, "Couldn't write in temporary file");
            }
            memset(buffer, 0, 512);
            justRead = (int) read(STDIN_FILENO, buffer, 512);
        }
        close(fd);
        fd = open("stdin.txt", O_RDWR);
    } else {
        fd = open(file, O_RDWR);
    }

    if (fd < 0) {
        return ooops("Could not open file to read");
    }

    char buf[MAX_PAYLOAD_SIZE];

    int justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
    if (justRead < 0) {
        close(fd);
        return ooops("Error when reading");
    }

    int count = 0;

    while (justRead != 0) {
        pkt_t *packet = pkt_new();
        if (packet == NULL) {
            close(fd);
            return ooops("Out of memory at packet creation");
        }

        int statusCode;

        statusCode = pkt_set_type(packet, PTYPE_DATA);
        if (statusCode != PKT_OK) {
            close(fd);
            return ooops("Error in pkt_set_type()");
        }

        statusCode = pkt_set_tr(packet, 0);
        if (statusCode != PKT_OK) {
            close(fd);
            return ooops("Error in pkt_set_tr()");
        }

        statusCode = pkt_set_seqnum(packet, nextSeqnum);
        if (statusCode != PKT_OK) {
            close(fd);
            return ooops("Error in pkt_set_seqnum()");
        }
        increment_nextSeqnum();

        statusCode = pkt_set_payload(packet, buf, (uint16_t) justRead);
        if (statusCode != PKT_OK) {
            close(fd);
            return ooops("Error in pkt_set_payload()");
        }

        statusCode = stack_enqueue(stack, packet);
        if (statusCode != 0) {
            close(fd);
            return ooops("Error in stack_enqueue()");
        }
        count++;

        justRead = (int) read(fd, buf, MAX_PAYLOAD_SIZE);
        if (justRead < 0) {
            close(fd);
            return ooops("Error when reading");
        }
    }

    close(fd);
    remove("stdin.txt");

    /*
     * Add terminating connection packet
     */
    pkt_t *terminateconnectionPkt = pkt_new();
    pkt_set_type(terminateconnectionPkt, PTYPE_DATA);
    pkt_set_window(terminateconnectionPkt, 0);
    pkt_set_seqnum(terminateconnectionPkt, nextSeqnum);
    pkt_set_length(terminateconnectionPkt, 0);

    int statusCode = stack_enqueue(stack, terminateconnectionPkt);
    if (statusCode != 0) {
        return ooops("Error in stack_enqueue()");
    }
    count++;

    fprintf(stderr, "File read successfully. %i packet(s) added to the stack. Ready to transmit.\n", count);
    return EXIT_SUCCESS;
}

void increment_nextSeqnum() {
    nextSeqnum = (uint8_t) ((nextSeqnum + 1) % 256);
}
