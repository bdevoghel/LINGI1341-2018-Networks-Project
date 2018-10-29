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

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

uint8_t expectedSeqnum;
uint8_t window;

uint8_t lastNACKseqnum = 0;
uint32_t lastNACKTimestamp = 0;
int lastNACKtries = 0;

void incrementSeqnum() {
    expectedSeqnum = (uint8_t) ((expectedSeqnum + 1) % 256);
}

int isExpected(uint8_t receivedSeqnum) {
    int diff = receivedSeqnum - expectedSeqnum;
    if (diff < 0) {
        diff += 256;
    }
    return diff < MAX_WINDOW_SIZE;
}

int send_reply(int sfd, ptypes_t type, uint32_t previousTimestamp) {
    if(expectedSeqnum == lastNACKseqnum && lastNACKTimestamp == time(NULL) && lastNACKtries > 1 && type == PTYPE_NACK) {
        fprintf(stderr, "Not sending NACK for seqnum %i. Waiting max 1 sec.\n", expectedSeqnum);
    } else {
        pkt_t *packet = pkt_new();
        pkt_set_type(packet, type);
        pkt_set_window(packet, window);
        pkt_set_seqnum(packet, expectedSeqnum);
        pkt_set_timestamp(packet, previousTimestamp);
        pkt_set_length(packet, 0);

        char *ackBuffer = malloc(sizeof(packet));

        size_t written = 12;
        pkt_status_code encodeResult = pkt_encode(packet, ackBuffer, (size_t *) &written);
        if (encodeResult == E_NOMEM) {
            fprintf(stderr, "Unable to encode the (N)ACK with seqnum %i\n", expectedSeqnum);
            free(ackBuffer);
            pkt_del(packet);
            return EXIT_FAILURE;
        }
        if (type == PTYPE_ACK) {
            fprintf(stderr, MAG"Sent ACK with\tSEQNUM : %i\tWINDOW : %i\n"RESET, expectedSeqnum, window);
        }else{
            fprintf(stderr, MAG"Sent NACK with\tSEQNUM : %i\tWINDOW : %i\n"RESET, expectedSeqnum,window);
        }
        ssize_t wrote = send(sfd, ackBuffer, (size_t) written, MSG_CONFIRM);
        if (wrote == -1) {
            fprintf(stderr, "Unable to end the (N)ACK with seqnum %i\n", expectedSeqnum);
            free(ackBuffer);
            pkt_del(packet);
            return EXIT_FAILURE;
        }
        free(ackBuffer);
        pkt_del(packet);

        if(expectedSeqnum == lastNACKseqnum && lastNACKTimestamp == time(NULL)) {
            lastNACKtries++;
        } else {
            lastNACKtries = 0;
        }
        lastNACKseqnum = expectedSeqnum;
        lastNACKTimestamp = (uint32_t) time(NULL);
    }
    return EXIT_SUCCESS;
}

/**
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop_receiver(int sfd, stack_t *receivingStack, int outputFileDescriptor) {

    int getOut = 0;

    char sfdBuffer[MAX_PAYLOAD_SIZE+16];

    fd_set fdSet;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    //Unuseful but inginious needs to go through the warnings
    int justRead;
    int written;

    pkt_t *packet;
    pkt_status_code decodeResult;

    int replyResult;
    uint32_t previousTimestamp;

    while (getOut == 0) {
        // Reset everything for new iteration of the loop
        memset(sfdBuffer, 0, MAX_PAYLOAD_SIZE+16);

        FD_ZERO(&fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout);

        if (FD_ISSET(sfd, &fdSet)) {
            fprintf(stderr,"BEFORE : \tExpect : %i\tWindow : %i\n",expectedSeqnum,window);

            packet = pkt_new();
            justRead = (int) read(sfd, sfdBuffer, MAX_PAYLOAD_SIZE + 16);
            decodeResult = pkt_decode(sfdBuffer, (const size_t) justRead, packet);

            previousTimestamp = pkt_get_timestamp(packet);
            fprintf(stderr, CYN"Received %i\tTimestamp : %i\n"RESET, pkt_get_seqnum(packet), pkt_get_timestamp(packet));
            if (decodeResult == PKT_OK) {
                if (
                        pkt_get_type(packet) == PTYPE_DATA &&
                        pkt_get_length(packet) == 0 &&
                        pkt_get_tr(packet) == 0 &&
                        pkt_get_seqnum(packet) == expectedSeqnum
                        ) {
                    incrementSeqnum();
                    send_reply(sfd, PTYPE_ACK, previousTimestamp);
                    break;
                }
                if (pkt_get_tr(packet) == 1) {
                    send_reply(sfd, PTYPE_NACK, previousTimestamp);
                }
                if (pkt_get_seqnum(packet) == expectedSeqnum) {
                    written = (int) write(outputFileDescriptor, pkt_get_payload(packet), pkt_get_length(packet));
                    pkt_del(packet);
                    if (written == -1) {
                        perror("Ooops, received packet but can't write it...");
                    }

                    incrementSeqnum();
                    packet = stack_remove(receivingStack, expectedSeqnum);

                    while (packet != NULL) {
                        written = (int) write(outputFileDescriptor, pkt_get_payload(packet), pkt_get_length(packet));
                        previousTimestamp = pkt_get_timestamp(packet);
                        pkt_del(packet);
                        if (written == -1) {
                            perror("Can't print packet from stack");
                        }
                        incrementSeqnum();

                        window = (uint8_t) (MAX_WINDOW_SIZE - stack_size(receivingStack));
                        packet = stack_remove(receivingStack, expectedSeqnum);
                    }

                    replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp);
                    if (replyResult == EXIT_FAILURE) {
                        fprintf(stderr, "Couldn't send ACK\n");
                    }

                } else if (isExpected(pkt_get_seqnum(packet))/*pkt_get_seqnum(packet) - expectedSeqnum > 0 && pkt_get_seqnum(packet) - expectedSeqnum < MAX_WINDOW_SIZE*/) {
                    stack_enqueue(receivingStack, packet);
                    window = (uint8_t) (MAX_WINDOW_SIZE - stack_size(receivingStack));
                    replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp);
                    if (replyResult == EXIT_FAILURE) {
                        fprintf(stderr, "Couldn't send ACK when enqueued\n");
                    }
                } else {
                    fprintf(stderr, YEL"Packet %i out of sequence and not stored\n"RESET, pkt_get_seqnum(packet));
                    replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp);
                    if (replyResult == EXIT_FAILURE) {
                        fprintf(stderr, "Couldn't send NACK when out of sequence\n");
                    }
                }
            } else {
                pkt_del(packet);
                fprintf(stderr, "PACKET NOT OK : %i (4 == E_CRC)\n", decodeResult); // TODO : ca c'est ma faute --Brieuc
                send_reply(sfd, PTYPE_NACK, previousTimestamp);
                if(pkt_get_seqnum(packet) < expectedSeqnum) { // TODO : legit ?? modif apportee apres infinite de " Expect : 25	\n Received 0   \n PACKET NOT OK : 4 (4 == E_CRC)" recus
                    break;
                }
            }
            fprintf(stderr, "AFTER : \tExpect : %i\tWindow : %i\tStack : %i\n", expectedSeqnum, window,
                    (int) receivingStack->size);

            fprintf(stderr, "---------------------------------------------------------\n");

        }

        if (feof(stdin) != 0) {
            getOut = 1;
        }

    }
    fprintf(stderr, GRN "=> CLOSING CONNECTION" RESET "\n\n");

}