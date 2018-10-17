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
    /*
     * Reading arguments
     */
    char *hostname;
    int port;
    int fOption = 0;
    char *fileToWrite;

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
        fileToWrite = NULL; // TODO write in stdout
    }

    /* formation du lien entre le sender et le receiver */

    /* initialisation du buffer de reception et du premier numero de sequence attendu*/

    /* Resolve the hostname */

    /* Get a socket */

    /* Process I/O */

    /* a faire a la fin, quand length = 0 recu */

    return EXIT_SUCCESS;
}
