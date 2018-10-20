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
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

stack_t *sendingStack;

size_t justWritten;
size_t justRead;
pkt_status_code pktStatusCode;
int statusCode;

pkt_t *nextPktToSend;
pkt_t *lastPktReceived;
pkt_t *pktACKed;

size_t bufSize;

int8_t receiverWindowSize = 1;
uint32_t RTlength = 4; // [s] = RTT

uint8_t nextWindow;

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

    struct timeval timeout2;
    timeout2.tv_sec = 0;
    timeout2.tv_usec = 500000;
    int i = 0;

    size_t replySize = 12;

    //int getOut = 0; // flag for loop
    while(stack_size(sendingStack) > 0) {
        bufSize = 16 + MAX_PAYLOAD_SIZE;

        /*
        FD_ZERO(&fdSet);
        FD_SET(0, &fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout1); // wait for sdf to be ready
        */
        nextPktToSend = stack_send_pkt(sendingStack, stack_get_toSend_seqnum(sendingStack));
        if(nextPktToSend == NULL) {
            perror("Next packet to send failed");
            return EXIT_FAILURE;
        }

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

        if(pkt_get_seqnum(nextPktToSend) != 3 || i == 1) { //TODO : JUST FOR TESTING -> REMOVE !!!!!!!
            fprintf(stderr, GRN "=> DATA\tSeqnum : %i\tLength : %i\tTimestamp : %i" RESET "\n\n", pkt_get_seqnum(nextPktToSend), pkt_get_length(nextPktToSend), pkt_get_timestamp(nextPktToSend));
            justWritten = (size_t) send(sfd, buf, bufSize, MSG_CONFIRM);
            if ((int) justWritten < 0) {
                perror("Send failed");
                return EXIT_FAILURE;
            }
        }else {
            i = 1;
        }
        receiverWindowSize--;

        FD_ZERO(&fdSet);
        FD_SET(sfd, &fdSet);

        select(sfd + 1, &fdSet, NULL, NULL, &timeout2);

        if (FD_ISSET(sfd, &fdSet)) {
            justRead = (size_t) read(sfd, buf, replySize);
            if((int) justRead < 0) {
                perror("Recv failed");
                return EXIT_FAILURE;
            }

            if ((int) justRead > 0) { // ACK or NACK received
                lastPktReceived = pkt_new();
                pktStatusCode = pkt_decode(buf, justRead, lastPktReceived);
                if (pktStatusCode != PKT_OK) {
                    perror("Decode failed");
                    return EXIT_FAILURE;
                }

                if (pkt_get_type(lastPktReceived) == PTYPE_ACK) {
                    fprintf(stderr, RED "~ ACK\tSeqnum : %i\tLength : %i\tTime : %i" RESET "\n\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived));

                    // TODO : gérer ACK
                    // TODO : update window (for sender AND receiver : windox in nextPktToSend AND updates # packets that can be send

                    receiverWindowSize = pkt_get_window(lastPktReceived);
                    fprintf(stderr, "Last seq : %i\n", pkt_get_seqnum(lastPktReceived));
                    pktACKed = stack_remove(sendingStack, (uint8_t) (pkt_get_seqnum(lastPktReceived) - 1));

                    /*nextPktToSend = stack_send_pkt(sendingStack, pkt_get_seqnum(lastPktReceived));
                    if (nextPktToSend == NULL) {
                        perror("Next packet to send failed");
                        return EXIT_FAILURE;
                    }*/

                } else if (pkt_get_type(lastPktReceived) == PTYPE_NACK) {
                    fprintf(stderr, BLU "~ NACK\tSeqnum : %i\tLength : %i\tTime : %i" RESET "\n\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived));

                    nextPktToSend = stack_send_pkt(stack, pkt_get_seqnum(lastPktReceived));
                    if(nextPktToSend == NULL) {
                        perror("Next packet to send failed");
                        return EXIT_FAILURE;
                    }

                    pktStatusCode = pkt_set_timestamp(nextPktToSend, (uint32_t) time(NULL));
                    if (pktStatusCode != PKT_OK) {
                        perror("Error in pkt_set_timestamp()");
                        return EXIT_FAILURE;
                    }

                    pktStatusCode = pkt_encode(nextPktToSend, buf, &bufSize);
                    fprintf(stderr, YEL "%i\n" RESET, pktStatusCode);
                    if(pktStatusCode != PKT_OK) {
                        perror("Encode failed");
                        return EXIT_FAILURE;

                    }

                    fprintf(stderr, GRN "=> DATA\tSeqnum : %i\tLength : %i\tTimestamp : %i" RESET "\n\n", pkt_get_seqnum(nextPktToSend), pkt_get_length(nextPktToSend), pkt_get_timestamp(nextPktToSend));
                    justWritten = (size_t) send(sfd, buf, bufSize, MSG_CONFIRM);
                    if ((int) justWritten < 0) {
                        perror("Send failed");
                        return EXIT_FAILURE;
                    }

                    // TODO : gérer NACK
                    // TODO : update window (for sender AND receiver : windox in nextPktToSend AND updates # packets that can be send

                    receiverWindowSize = pkt_get_window(lastPktReceived);

                    /*nextPktToSend = stack_send_pkt(sendingStack, pkt_get_seqnum(lastPktReceived));
                    if (nextPktToSend == NULL) {
                        perror("Next packet to send failed");
                        return EXIT_FAILURE;
                    }*/

                } else {
                    perror("Received something else than ACK or NACK");
                    return EXIT_FAILURE;
                }
            }
        } else { // nothing received yet
                int wait = 1;
                while (wait && receiverWindowSize <= 0) {
                    statusCode = check_for_RT();
                    if (statusCode != 0) {
                        if (statusCode == 101) { // a RT has expired
                            wait = 0;
                        } else {
                            return statusCode;
                        }
                    }

                    // wait until sender can receive something

                } // while(wait && receiverWindowSize <= 0)
            // if justRead

        }
    } // while(!getOut && stack_size(sendingStack) > 0)



    pkt_t *packet = pkt_new();
    pkt_set_type(packet, PTYPE_DATA);
    pkt_set_window(packet, nextWindow);
    pkt_set_seqnum(packet, 0);
    pkt_set_timestamp(packet, 0);
    pkt_set_length(packet, 0);

    char *buffer = malloc(sizeof(packet));

    int twelve = 12;
    int encodeResult = pkt_encode(packet, buffer, (size_t *) &twelve);
    if (encodeResult != PKT_OK) {
        fprintf(stderr, "Unable to encode the stopping packet %i\n", encodeResult);
    }
    //sleep(1);
    int sent = (int) send(sfd, buffer, (size_t) twelve, MSG_CONFIRM);
    if (sent == -1) {
        fprintf(stderr, "Unable to send the stop\n");
    }
    free(buffer);

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