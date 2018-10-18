/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
 * Réalisé avec l'aide des sites suivants : https://github.com/Donaschmi/LINGI1341/blob/master/Inginious/Envoyer_et_recevoir_des_donn%C3%A9es/read_write_loop.c
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "../packet/packet.h" // MAX_PAYLOAD_SIZE
#include "../stack/stack.h"

uint8_t nextSeqnum;
uint8_t nextWindow;

stack_t *sendingStack;

/**
 * TODO
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
int read_write_loop_sender(int sfd) {
    int getOut = 0;

    char stdInBuffer[MAX_PAYLOAD_SIZE];
    char sfdBuffer[MAX_PAYLOAD_SIZE];

    fd_set fdSet;

    struct timeval timeout;
    timeout.tv_sec = 10000;
    timeout.tv_usec = 0;

    //Unuseful but inginious needs to go through the warnings
    int written;
    int justRead;

    while (getOut == 0 && stack_size(sendingStack) > 0) {
        // Reset everything for new iteration of the loop
        memset(stdInBuffer, 0, MAX_PAYLOAD_SIZE);
        memset(sfdBuffer, 0, MAX_PAYLOAD_SIZE);

        justRead = 0;
        written = 0;

        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout);

        //if (FD_ISSET(sfd, &fdSet)) {

        // NEW
        size_t bytesWritten;
        pkt_status_code pktStatusCode;
        pkt_t *nextPktToSend;
        size_t bufSize = 12 + MAX_PAYLOAD_SIZE + 4;
        char pipeBuf[bufSize];

        nextPktToSend = stack_send_pkt(sendingStack, stack_get_toSend_seqnum(sendingStack));
        fprintf(stderr, "TypeTrWin : %02x\n", (pkt_get_type(nextPktToSend)<<6)+(pkt_get_tr(nextPktToSend)<<5)+pkt_get_window(nextPktToSend));
        fprintf(stderr, "Seqnum    : %02x\n", pkt_get_seqnum(nextPktToSend));
        fprintf(stderr, "Length    : %04x\n", pkt_get_length(nextPktToSend));
        fprintf(stderr, "Timestamp : %08x\n", pkt_get_timestamp(nextPktToSend));
        fprintf(stderr, "CRC1      : %08x\n", pkt_get_crc1(nextPktToSend));
        fprintf(stderr, "Payload   : %s\n", pkt_get_payload(nextPktToSend));
        fprintf(stderr, "CRC2      : %08x\n", pkt_get_crc2(nextPktToSend));
        if(nextPktToSend == NULL) {
            perror("Next packet to send failed");
            return EXIT_FAILURE; //ooops("Error at pkt retrieving");
        }

        pktStatusCode = pkt_encode(nextPktToSend, pipeBuf, &bufSize);

        if(pktStatusCode != PKT_OK) {
            perror("Encode failed");
            return EXIT_FAILURE; //ooops("Error at packet encoding");
        }

        bytesWritten = (size_t) send(sfd, pipeBuf, bufSize, MSG_CONFIRM);
        if((int)bytesWritten < 0) {
            perror("Write failed");
            return EXIT_FAILURE; //ooops("Error at writing : bytesWritten < 0");
        }

        if(justRead && written) {
            printf("%i", (int)timeout.tv_sec);
        }
        //TODO remove
        getOut++;
    }
    return EXIT_SUCCESS;
}