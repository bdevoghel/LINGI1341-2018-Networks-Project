/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dst_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dst_addr, int dst_port) {

    if (source_addr == NULL || src_port < 0 || dst_addr == NULL || dst_port < 0) {
        return -1;
    }

    int newSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (newSocket == -1) {
        return -1;
    }

    struct sockaddr_in6 *srcAddress = malloc(sizeof(struct sockaddr_in6));
    if (srcAddress == NULL) {
        return -1;
    }
    memcpy(source_addr, srcAddress, sizeof(struct sockaddr_in6));
    srcAddress->sin6_port = htons(src_port);
    if (bind(newSocket, (struct sockaddr *)srcAddress, sizeof(*srcAddress)) == -1) {
        free(srcAddress);
        return -1;
    }


    struct sockaddr_in6 *dstAddress = malloc(sizeof(struct sockaddr_in6));
    if (dstAddress == NULL) {
        return -1;
    }
    memcpy(dst_addr, dstAddress, sizeof(struct sockaddr_in6));
    dstAddress->sin6_port = htons(dst_port);
    if (connect(newSocket, (struct sockaddr *)dstAddress, sizeof(*dstAddress)) == -1) {
        free(srcAddress);
        free(dstAddress);
        return -1;
    }

    free(srcAddress);
    free(dstAddress);
    return newSocket;
}