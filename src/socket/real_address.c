/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 * Réalisé avec l'aide des sites suivants : https://github.com/Donaschmi/LINGI1341/blob/master/Inginious/Envoyer_et_recevoir_des_donn%C3%A9es/real_address.c

 */

#include <netinet/in.h> // sockaddr_in6
#include <sys/types.h> // sockaddr_in6
// autres #includes TODO

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
        return "Parameter \"address\" should not be NULL";
    }
    if (rval == NULL) {
        return "Parameter \"rval\" should not be NULL";
    }

    struct addrinfo addressInfo;

    memset(&addressInfo, 0, sizeof(struct addrinfo));

    addressInfo->ai_family = AF_INET6;
    addressInfo->ai_socktype = SOCK_DGRAM;
    addressInfo->ai_protocol = IPPROTO_UDP;
    addressInfo->ai_flags = 0;


    struct addrinfo *tmp;
    int addrinfo = getaddrinfo(address, NULL, &addressInfo, &tmp);
    if (addrinfo != 0) {
        return "getaddrinfo returned a non NULL value";
    }

    memcpy(rval, (struct sockaddr_in6*) tmp->ai_addr, sizeof(struct sockaddr_in6*));

    freeaddrinfo(tmp);

    return NULL;


}
