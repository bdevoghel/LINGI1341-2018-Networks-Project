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

/**
 * main function
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {

    /*
     * Recuperation des arguments
     */
    int fOption = 0;
    int opt;
    int nOpt = 0;
    while((opt = getopt(argc, argv, "f:")) != -1) { // voir man getopt TODO verifier options
        switch(opt) {
            case 'f' :
                fOption = 1;
                nOpt++;
                break;
            default : // option inconnue
                fprintf(stderr, "Unknown argument detected\n");
                return EXIT_FAILURE;
        }
    }
    if(argc - 1 == nOpt) { // default
        fprintf(stderr, "%i option(s) read\n", nOpt);
    } else {
        fprintf(stderr, "%i option(s) read\n", argc - 1);
        // TODO handle other arguments
    }


    fprintf(stderr, "fOption : %i\n", fOption);

    /* ouverture du fichier */

    /* formation du lien entre le sender et le receiver */

    /* nombre de places initial dans le buffer d'envoi*/

    /* Resolve the hostname */

    /* Get a socket */

    /* Process I/O */

    /* a faire a la fin des data apres avoir envoye un packet avec une longueur 0 ==> verification que receiver a bien recu le packet ??? */

    return EXIT_SUCCESS;
}
