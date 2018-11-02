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

#include "socket/real_address.h"
#include "socket/create_socket.h"
#include "socket/read_write_loop_receiver.h"
#include "socket/wait_for_client.h"
#include "packet/packet.h"
#include "stack/stack.h"

char *hostname = NULL;
int port = -1;
char *fileToWrite = NULL;

stack_t *receivingStack = NULL;
extern uint8_t expectedSeqnum;
extern uint8_t window;


int socketFileDescriptor;

int outputFileDescriptor;

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
 * Processes arguments and options received with main
 * @param argc of main
 * @param argv of main
 * @return : 0 if options processed successfully, 1 if something went wrong
 */
int process_options(int argc,char *argv[]);

/**
 * Resolves the hostname & creates a socket
 */
int init_connection();

/**
 * receiver permet de de realiser un transfer de donnees unidirectionnel et fiable
 *
 * utilisation : "./receiver [-f X] hostname port"
 * il accepte la connexion depuis l'adresse [hostname] (:: pour toues les interfaces) avec le port [port] sur lequel il ecoute
 * si -f est present, les donnees seront imprimees sur le fichier X, sinon elles seront affichees sur stdout
 *
 * @param argc : 3 < argc < 5
 * @param argv : receiver [-f X] hostname port
 * @return 0 if everything went well
 */
int main(int argc, char *argv[]) {
    int statusCode;

    /*
     * Reading arguments
     */
    statusCode = process_options(argc, argv);
    if (statusCode != 0) {
        return statusCode;
    }

    /*
     * Resolve the hostname, create socket and initializes connection
     */
    statusCode = init_connection();
    if(statusCode != 0) {
        return statusCode;
    }

    /* initialisation du buffer de reception et du premier numero de sequence attendu*/
    receivingStack = stack_init();
    if(receivingStack == NULL) {
        return ooops("Out of memory at stack creation");
    }

    if (fileToWrite != NULL) {
        outputFileDescriptor = open(fileToWrite, O_RDWR | O_CREAT | O_APPEND, S_IRWXU); // NOLINT
    } else {
        outputFileDescriptor = STDOUT_FILENO;
    }
    expectedSeqnum = 0;
    window = MAX_WINDOW_SIZE;

    read_write_loop_receiver(socketFileDescriptor, receivingStack, outputFileDescriptor);

    fprintf(stderr, GRN "=> CLOSING CONNECTION" RESET "\n\n");

    stack_free(receivingStack);

    close(socketFileDescriptor);
    close(outputFileDescriptor);

    return EXIT_SUCCESS;
}


int process_options(int argc,char *argv[]) {
    int fOption = 0;

    int opt;
    while((opt = getopt(argc, argv, "f:")) != -1) {
        switch(opt) {
            case 'f' :
                fOption = 1;
                fileToWrite = optarg;
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
        fprintf(stderr, "%i option(s) read. Usage : \"receiver [-f X] hostname port\"\n", (1 + fOption*2 + hostnameSet + portSet));
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Options processed.\n"
                    "   Hostname      : %s\n"
                    "   Port          : %i\n"
                    "   File to write : %s\n",
                    hostname, port, fileToWrite);
    return EXIT_SUCCESS;
}


int init_connection() {
    if (hostname == NULL || port < 0) {
        return ooops("Hostname is NULL or destination port is negative");
    }

    struct sockaddr_in6 address;

    const char *realAddressResult = real_address(hostname, &address);
    if (realAddressResult != NULL) {
        return ooops("Unable to resolve hostname");
    }

    // Create a socket
    socketFileDescriptor = create_socket(&address, port, NULL, -1);
    if (socketFileDescriptor == -1) {
        return ooops("Error while creating the socket");
    }
    int waitForClientResult = wait_for_client(socketFileDescriptor);
    if (waitForClientResult == -1) {
        return ooops("Error while waiting for the client");
    }

    return EXIT_SUCCESS;
}
