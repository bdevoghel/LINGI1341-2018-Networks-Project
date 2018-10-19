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


#include "read_write_loop_receiver.h"
#include "../packet/packet.h" // MAX_PAYLOAD_SIZE

uint8_t expectedSeqnum;
uint8_t window;

/**
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop_receiver(int sfd, stack_t *receivingStack, int outputFileDescriptor) {
    if(outputFileDescriptor) {
        //TODO REMOVE UNUSED
    }
    int getOut = 0;

    char sfdBuffer[MAX_PAYLOAD_SIZE+16];

    fd_set fdSet;

    struct timeval timeout;
    timeout.tv_sec = 10000;
    timeout.tv_usec = 0;

    //Unuseful but inginious needs to go through the warnings
    int justRead;
    int written;

    pkt_t *packet;
    pkt_status_code decodeResult;

    char *ackBuffer;
    int encodeResult;

    while (getOut == 0) {
        // Reset everything for new iteration of the loop
        memset(sfdBuffer, 0, MAX_PAYLOAD_SIZE+16);

        justRead = 0;
        written = 0;

        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout);

        if (FD_ISSET(sfd, &fdSet)) {
            packet = pkt_new();
            justRead = read(sfd, sfdBuffer, MAX_PAYLOAD_SIZE + 16);
            decodeResult = pkt_decode(sfdBuffer, justRead, packet);

            /*
            fprintf(stderr, "New packet ready to send :\n");
            fprintf(stderr, "   TypeTrWin : %02x\n", (pkt_get_type(packet)<<6)+(pkt_get_tr(packet)<<5)+pkt_get_window(packet));
            fprintf(stderr, "   Seqnum    : %02x\n", pkt_get_seqnum(packet));
            fprintf(stderr, "   Length    : %04x\n", pkt_get_length(packet));
            fprintf(stderr, "   Timestamp : %08x\n", pkt_get_timestamp(packet));
            fprintf(stderr, "   CRC1      : %08x\n", pkt_get_crc1(packet));
            fprintf(stderr, "   Payload   : %s\n", pkt_get_payload(packet));
            fprintf(stderr, "   CRC2      : %08x\n\n", pkt_get_crc2(packet));
            */

            if (decodeResult == PKT_OK) {
                if (pkt_get_seqnum(packet) == expectedSeqnum) {
                    written = write(outputFileDescriptor, pkt_get_payload(packet), pkt_get_length(packet));
                    pkt_del(packet);
                    if (written == -1) {
                        perror("Oooops, received packet but can't write it...");
                    }
                    expectedSeqnum = (expectedSeqnum +1) % 256;

                    packet = pkt_new();
                    pkt_set_type(packet, PTYPE_ACK);
                    pkt_set_window(packet, window);
                    pkt_set_seqnum(packet, expectedSeqnum);
                    pkt_set_timestamp(packet, (uint32_t) time(NULL));

                    ackBuffer = malloc(sizeof(packet));
                    encodeResult = pkt_encode(packet, ackBuffer, (size_t *) sizeof(packet));
                    if (encodeResult == E_NOMEM) {
                        fprintf(stderr, "Unable to encode the ACK with seqnum %i\n", expectedSeqnum);
                    }
                    written = send(sfd, ackBuffer, sizeof(ackBuffer), MSG_CONFIRM);
                    if (written == -1) {
                        fprintf(stderr, "Unable to end the ACK with seqnum %i\n", expectedSeqnum);
                    }

                } else if (pkt_get_seqnum(packet) - expectedSeqnum >= 0 && pkt_get_seqnum(packet) - expectedSeqnum < window) {
                    stack_enqueue(receivingStack, packet);
                    //TODO : send NACK
                }
            } else {

                pkt_del(packet);
                // TODO : send NACK
            }


        }

        if (feof(stdin) != 0) {
            getOut = 1;
        }

    }
}