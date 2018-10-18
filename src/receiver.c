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

// TODO ensure no memory leakage !!

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
 * receiver permet de de realiser un transfer de donnees unidirectionnel et fiable
 *
 * utilisation : "receiver hostname port [-f X]"
 * il accepte la connexion depuis l'adresse [hostname] (:: pour toues les interfaces) avec le port [port] sur lequel il ecoute
 * si -f est present, les donnees seront imprimees sur le fichier X, sinon elles seront affichees sur stdout
 *
 * @param argc : 3 < argc < 5
 * @param argv : receiver hostname port [-f X]
 * @return : EXIT_SUCCESS si success
 *           EXIT_FAILURE si erreur d'execution OU arguments non coherents
 *           TODO enum erreurs possibles
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
     * Resolve the hostname, create socket and initializes connexion
     */
    statusCode = init_connexion();
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
        outputFileDescriptor = stdout;
    }

    read_write_loop_receiver(socketFileDescriptor, , outputFileDescriptor);

    /* a faire a la fin, quand length = 0 recu */
    if (hostname || port || fileToWrite) {}

    stack_free(receivingStack);

    return EXIT_SUCCESS;
}


int process_options(int argc,char *argv[]) {
    /*
     * Reading arguments
     */

    int fOption = 0;

    int opt;
    while((opt = getopt(argc, argv, "f:")) != -1) {
        switch(opt) {
            case 'f' :
                fOption = 1;
                fileToWrite = optarg;
                break;
            default : // unknown option
                fprintf(stderr, "Unknown argument detected : %s\n", optarg);
                return EXIT_FAILURE;
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
        fprintf(stderr, "%i option(s) read. Usage : \"receiver hostname port [-f X]\"\n", (3 + fOption*2));
        return EXIT_FAILURE;
    }

    if(!fOption) {
        fileToWrite = NULL; // TODO write in stdout
    }

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
    socketFileDescriptor = create_socket(&address, port, NULL, -1);
    if (socketFileDescriptor == -1) {
        return ooops("Error while creating the socket");
    }

    int waitForClientResult = wait_for_client(socketFileDescriptor);
    if (waitForClientResult == -1) {
        return ooops("Error while waiting for the client");
    }

    return EXIT_SUCCESS; // TODO check value to return
}
