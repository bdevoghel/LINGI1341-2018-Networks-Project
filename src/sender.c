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
 *           TODO enum erreurs possibles
 */
int main(int argc, char *argv[]) {
    /*
     * Reading arguments
     */
    char *hostname;
    int port;
    int fOption = 0;
    char *fileToRead;

    int opt;
    while((opt = getopt(argc, argv, "f:")) != -1) {
        switch(opt) {
            case 'f' :
                fOption = 1;
                fileToRead = optarg;
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
            port = atoi(argv[i]);
            portSet = 1;
        }
        i++;
    }

    if(argc > (3 + fOption*2) || !hostnameSet || !portSet) {
        fprintf(stderr, "%i option(s) read. Usage : \"receiver hostname port [-f X]\"\n", (3 + fOption*2));
        return EXIT_FAILURE;
    }

    if(!fOption) {
        fileToRead = NULL; // TODO read stdin
    }


    /* ouverture du fichier */

    /* formation du lien entre le sender et le receiver */

    /* nombre de places initial dans le buffer d'envoi*/

    /* Resolve the hostname */

    /* Get a socket */

    /* Process I/O */

    /* a faire a la fin des data apres avoir envoye un packet avec une longueur 0 ==> verification que receiver a bien recu le packet ??? */

    return EXIT_SUCCESS;
}
