/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 * Réalisé avec l'aide des sites suivants : https://github.com/Donaschmi/LINGI1341/blob/master/Inginious/Envoyer_et_recevoir_des_donn%C3%A9es/wait_for_client.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "../packet/packet.h" // MAX_PAYLOAD_SIZE

/**
 * Block the caller until a message is received on sfd,
 * and connect the socket to the source address of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd) {
    if (sfd < 0) {
        return -1;
    }

    int bufferLength = MAX_PAYLOAD_SIZE;

    char buffer[bufferLength];
    struct sockaddr_in6 clientAddress;
    socklen_t addressSize = sizeof(struct sockaddr_in6);

    memset(&clientAddress, 0, sizeof(struct sockaddr_in6));
    if(recvfrom(sfd, buffer, bufferLength, MSG_PEEK, (struct sockaddr *)&clientAddress, &addressSize) == -1) {
        return -1;
    }

    if (connect(sfd, (struct sockaddr *)&clientAddress, addressSize) == -1) {
        return -1;
    }

    return 0;
}