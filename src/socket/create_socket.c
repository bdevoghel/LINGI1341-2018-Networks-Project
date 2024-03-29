/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 * Réalisé avec l'aide des sites suivants : https://github.com/Donaschmi/LINGI1341/blob/master/Inginious/Envoyer_et_recevoir_des_donn%C3%A9es/create_socket.c
 * (Inspiration forte due à une nette incompréhension du non fonctionnement)
 */

#include "create_socket.h"

/**
 * Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dst_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port) {

    int newSocket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (newSocket == -1) {
        fprintf(stderr,"Create socket, newSocket");
        return -1;
    }

    // Server (source) socket
    if (source_addr != NULL && src_port > 0) {
        source_addr->sin6_port = htons(src_port);
        if (bind(newSocket, (struct sockaddr *)source_addr, sizeof(struct sockaddr_in6)) != 0) {
            perror("Create socket, bind");
            return -1;
        }
    }

    // Client (destination) socket
    if (dest_addr != NULL && dst_port > 0) {
        dest_addr->sin6_port = htons(dst_port);
        if (connect(newSocket, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in6)) != 0) {
            perror("Create socket, connect");
            return -1;
        }
    }

    return newSocket;
}
