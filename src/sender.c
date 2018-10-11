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
     * TODO
     */
    int opt;
    int nopt = 0;
    while((opt = getopt(argc, argv, "f:")) != -1) { // voir man getopt TODO verifier options
        switch(opt) {
            case 'f' :
                arg = strtod(optarg, &strdouble);
                if(optarg == strdouble) {
                    fprintf(stderr, "%s", "-a optarg is not a number.\n");
                    return EXIT_FAILURE;
                }
                nopt++;
                break;
            default : // option inconnue
                fprintf(stderr, "Unknown argument detected\n");
                return 1;
        }
    }
    if(argc - 1 == argc) { // default
        printf("%i\n", nopt);
    } else {
        printf("%.*f\n", atoi(argv[argc-1]), nopt);
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
