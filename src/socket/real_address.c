/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 * Réalisé avec l'aide des sites suivants : https://github.com/Donaschmi/LINGI1341/blob/master/Inginious/Envoyer_et_recevoir_des_donn%C3%A9es/real_address.c
 *                                          https://stackoverflow.com/questions/20411223/warning-passing-argument-2-of-getsockname-from-incompatible-pointer-type
 */

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Resolve the resource name to an usable IPv6 address
 * @address: The name to resolve
 * @rval: Where the resulting IPv6 address descriptor should be stored
 * @return: NULL if it succeeded, or a pointer towards
 *          a string describing the error if any.
 *          (const char* means the caller cannot modify or free the return value,
 *           so do not use malloc!)
 */


const char * real_address(const char *address, struct sockaddr_in6 *rval) {
    if (address == NULL) {
        return "NULL";
    }
    if (rval == NULL) {
        return "NULL";
    }

    struct addrinfo addressInfo;

    memset(&addressInfo, 0, sizeof(struct addrinfo));

    addressInfo.ai_family = AF_INET6;
    addressInfo.ai_socktype = SOCK_DGRAM;
    addressInfo.ai_protocol = IPPROTO_UDP;
    addressInfo.ai_flags = 0;


    struct addrinfo *tmp;
    int addrinfo = getaddrinfo(address, NULL, &addressInfo, &tmp);
    if (addrinfo != 0) {
        return "NULL";
    }

    memcpy(rval, (struct sockaddr_in6*) tmp->ai_addr, sizeof(tmp->ai_addr));

    freeaddrinfo(tmp);

    return NULL;


}
