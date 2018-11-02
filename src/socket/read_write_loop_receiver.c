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

int send_reply(int sfd, ptypes_t type, uint32_t previousTimestamp, uint8_t seqnum) {

    pkt_t *packet = pkt_new();
    pkt_set_type(packet, type);
    pkt_set_window(packet, window);
    pkt_set_seqnum(packet, seqnum);
    pkt_set_timestamp(packet, previousTimestamp);
    pkt_set_length(packet, 0);

    char *ackBuffer = malloc(sizeof(packet));

    size_t written = 12;
    pkt_status_code encodeResult = pkt_encode(packet, ackBuffer, (size_t *) &written);
    if (encodeResult == E_NOMEM) {
        fprintf(stderr, "Unable to encode the (N)ACK with seqnum %i\n", seqnum);

        free(ackBuffer);
        pkt_del(packet);
        return EXIT_FAILURE;
    }

    if (type == PTYPE_ACK) {
        fprintf(stderr, RED"Sent ACK with\tSEQNUM : %i\tWINDOW : %i\n"RESET, expectedSeqnum, window);
    } else {
        fprintf(stderr, RED"Sent NACK with\tSEQNUM : %i\tWINDOW : %i\n"RESET, expectedSeqnum,window);
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
    return EXIT_SUCCESS;
}

/**
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_loop_receiver(int sfd, stack_t *receivingStack, int outputFileDescriptor) {
    char sfdBuffer[MAX_PAYLOAD_SIZE+16];

    fd_set fdSet;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    //Unuseful but inginious needs to go through the warnings
    int justRead;
    int written;

    pkt_t *packet = NULL;
    pkt_status_code decodeResult;

    int replyResult;
    uint32_t previousTimestamp;

    struct timespec start, end;
    int trStarted = 0;

    while (1) {
        // Reset everything for new iteration of the loop
        memset(sfdBuffer, 0, MAX_PAYLOAD_SIZE+16);

        FD_ZERO(&fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout);

        if (FD_ISSET(sfd, &fdSet)) {
            if (trStarted == 0) {
                clock_gettime(CLOCK_MONOTONIC_RAW, &start);
                trStarted = 1;
                fprintf(stderr,"Starting transmission.\n");
            }

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
                    send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);

                    pkt_del(packet);
                    break;
                }
                if (pkt_get_tr(packet) == 1) {
                    send_reply(sfd, PTYPE_NACK, previousTimestamp, pkt_get_seqnum(packet));
                    pkt_del(packet);
                }
                if (pkt_get_seqnum(packet) == expectedSeqnum) {
                    written = (int) write(outputFileDescriptor, pkt_get_payload(packet), pkt_get_length(packet));
                    if (written == -1) {
                        perror("Ooops, received packet but can't write it...");
                    }

                    pkt_del(packet);
                    incrementSeqnum();

                    packet = stack_remove(receivingStack, expectedSeqnum);

                    while (packet != NULL) {
                        written = (int) write(outputFileDescriptor, pkt_get_payload(packet), pkt_get_length(packet));
                        previousTimestamp = pkt_get_timestamp(packet);
                        if (written == -1) {
                            perror("Can't print packet from stack");
                        }

                        pkt_del(packet);
                        incrementSeqnum();

                        window = (uint8_t) (MAX_WINDOW_SIZE - stack_size(receivingStack));
                        packet = stack_remove(receivingStack, expectedSeqnum);
                    }


                    replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    if (replyResult == EXIT_FAILURE) {
                        fprintf(stderr, "Couldn't send ACK\n");
                    }

                } else if (isExpected(pkt_get_seqnum(packet))/*pkt_get_seqnum(packet) - expectedSeqnum > 0 && pkt_get_seqnum(packet) - expectedSeqnum < MAX_WINDOW_SIZE*/) {
                    if (!is_in_stack(receivingStack, pkt_get_seqnum(packet))) {
                        stack_enqueue(receivingStack, packet);
                        window = (uint8_t) (MAX_WINDOW_SIZE - stack_size(receivingStack));
                        replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                        if (replyResult == EXIT_FAILURE) {
                            fprintf(stderr, "Couldn't send ACK when enqueued\n");
                        }
                    }
                } else {
                    fprintf(stderr, YEL"Packet %i out of sequence and not stored\n"RESET, pkt_get_seqnum(packet));
                    replyResult = send_reply(sfd, PTYPE_ACK, previousTimestamp, expectedSeqnum);
                    if (replyResult == EXIT_FAILURE) {
                        fprintf(stderr, "Couldn't send NACK when out of sequence\n");
                    }
                }
            } else {
                if(decodeResult == E_CRC) {
                    fprintf(stderr, YEL "Packet %i was corrupted\n" RESET, expectedSeqnum);
                } else {
                    fprintf(stderr, "PACKET NOT OK : %i\n", decodeResult);
                }
                send_reply(sfd, PTYPE_NACK, previousTimestamp, expectedSeqnum);

                pkt_del(packet);
            }
            fprintf(stderr, "AFTER : \tExpect : %i\tWindow : %i\tStack : %i\n", expectedSeqnum, window,
                    (int) receivingStack->size);

            fprintf(stderr, "---------------------------------------------------------\n");

        }

    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_us = (uint64_t) (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    double deltaT = ((double) delta_us)/1000000.0;

    fprintf(stderr, "\nPackets received successfully in %f seconds.\n", deltaT);
}