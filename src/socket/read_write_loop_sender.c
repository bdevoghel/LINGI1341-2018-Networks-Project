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

#include "read_write_loop_sender.h"

stack_t *sendingStack;

size_t justWritten;
size_t justRead;
pkt_status_code pktStatusCode;
int statusCode;

pkt_t *nextPktToSend;
pkt_t *lastPktReceived;
pkt_t *pktACKed;

size_t bufSize;

int8_t senderWindowSize = 1;
uint32_t RTlength = 3; // [s]

/**
 * Goes over all already sent packets and signals if a RT timer has expired
 * @return : 101 if RT has expired, EXIT_SUCCES or EXIT_FAILURE otherwise
 */
int check_for_RT();

/**
 * TODO correct description
 * Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
int read_write_loop_sender(int sfd, stack_t *stack) {
    sendingStack = stack;
    bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];

    fd_set fdSet;

    struct timeval timeout;
    timeout.tv_sec = 10000;
    timeout.tv_usec = 0;

    nextPktToSend = stack_send_pkt(sendingStack, stack_get_toSend_seqnum(sendingStack));
    if(nextPktToSend == NULL) {
        perror("Next packet to send failed");
        return EXIT_FAILURE;
    }

    int getOut = 0; // flag for loop
    while(!getOut && stack_size(sendingStack) > 0) {
        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout); // wait for sdf to be ready

        pktStatusCode = pkt_set_timestamp(nextPktToSend, (uint32_t) time(NULL));
        if (pktStatusCode != PKT_OK) {
            perror("Error in pkt_set_timestamp()");
            return EXIT_FAILURE;
        }

        pktStatusCode = pkt_encode(nextPktToSend, buf, &bufSize);
        if(pktStatusCode != PKT_OK) {
            perror("Encode failed");
            return EXIT_FAILURE;
        }

        /*
        fprintf(stderr, "New packet ready to send :\n");
        fprintf(stderr, "   TypeTrWin : %02x\n", (pkt_get_type(nextPktToSend)<<6)+(pkt_get_tr(nextPktToSend)<<5)+pkt_get_window(nextPktToSend));
        fprintf(stderr, "   Seqnum    : %02x\n", pkt_get_seqnum(nextPktToSend));
        fprintf(stderr, "   Length    : %04x\n", pkt_get_length(nextPktToSend));
        fprintf(stderr, "   Timestamp : %08x\n", pkt_get_timestamp(nextPktToSend));
        fprintf(stderr, "   CRC1      : %08x\n", pkt_get_crc1(nextPktToSend));
        fprintf(stderr, "   Payload   : %s\n", pkt_get_payload(nextPktToSend));
        fprintf(stderr, "   CRC2      : %08x\n", pkt_get_crc2(nextPktToSend));
        */

        justWritten = (size_t) send(sfd, buf, bufSize, MSG_CONFIRM);
        if((int)justWritten < 0) {
            perror("Send failed");
            return EXIT_FAILURE;
        }
        senderWindowSize --;

        justRead = read(sfd, buf, bufSize);
        if((int) justRead < 0) {
            perror("Read failed");
            return EXIT_FAILURE;
        }
        if((int) justRead > 0) { // ACK or NACK received
            pktStatusCode = pkt_decode(buf, justRead, lastPktReceived);
            if(pktStatusCode != PKT_OK) {
                perror("Decode failed");
                return EXIT_FAILURE;
            }

            if(pkt_get_type(lastPktReceived) == PTYPE_ACK) {
                // TODO : gérer ACK
                // TODO : update window (for sender AND receiver : windox in nextPktToSend AND updates # packets that can be send

                pktACKed = stack_remove(sendingStack, pkt_get_seqnum(lastPktReceived));

                nextPktToSend = stack_send_pkt(sendingStack, stack_get_toSend_seqnum(sendingStack));
                if(nextPktToSend == NULL) {
                    perror("Next packet to send failed");
                    return EXIT_FAILURE;
                }

            } else if(pkt_get_type(lastPktReceived) == PTYPE_NACK) {
                // TODO : gérer NACK
                // TODO : update window (for sender AND receiver : windox in nextPktToSend AND updates # packets that can be send

                nextPktToSend = stack_send_pkt(sendingStack, pkt_get_seqnum(lastPktReceived));
                if(nextPktToSend == NULL) {
                    perror("Next packet to send failed");
                    return EXIT_FAILURE;
                }

            } else {
                perror("Received something else than ACK or NACK");
                return EXIT_FAILURE;
            }
        } else { // nothing received yet
            int wait = 1;
            while(wait && senderWindowSize <= 0) {
                statusCode = check_for_RT();
                if(statusCode != 0) {
                    if(statusCode == 101) { // a RT has expired
                        wait = 0;
                    } else {
                        return statusCode;
                    }
                }

                // wait until sender can receive something

            } // while(wait && senderWindowSize <= 0)
        } // if justRead
    } // while(!getOut && stack_size(sendingStack) > 0)

    pkt_del(nextPktToSend);
    pkt_del(lastPktReceived);
    pkt_del(pktACKed);

    stack_free(sendingStack);

    return EXIT_SUCCESS;
}

int check_for_RT() {
    node_t *runner = sendingStack->first;
    while(runner != sendingStack->toSend) {
        if ((uint32_t) time(NULL) - pkt_get_timestamp(runner->pkt) > RTlength) {
            nextPktToSend = stack_send_pkt(sendingStack, runner->seqnum);
            if (nextPktToSend == NULL) {
                perror("Next packet to send failed");
                return EXIT_FAILURE;
            }
            return 101;
        } else {
            runner = runner->next;
        }
    }
    return EXIT_SUCCESS;
}