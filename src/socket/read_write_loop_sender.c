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
uint8_t seqnumToSend;
uint8_t lastEncodedSeqnum;

// flags and return values of function calls
size_t justWritten;
size_t justRead;
pkt_status_code pktStatusCode;
int statusCode;
int hasRTed = 0;
int hasNACKed = 0;

pkt_t *nextPktToSend;
pkt_t *lastPktReceived;

size_t bufSize;
size_t replySize = 12;

uint8_t receiverWindowSize = 1;
uint32_t RTlength = 4; // [s] = RTT

uint8_t nextWindow;

/**
 * Goes over all already sent packets and signals if a RT timer has expired
 * @return : 101 if RT has expired, EXIT_SUCCES or EXIT_FAILURE otherwise
 */
int check_for_RT();

/**
 * updates the window size of the sender to communicate to the receiver
 */
void set_nextWindow();

/**
 * Loop sending packets (read from [stack]) on a socket,
 * while reading ACKs and NACKs from the socket
 * @sfd : the socket file descriptor. It is both bound and connected.
 * @stack : stack containing all the packets to send
 * @return : as soon as connexion was terminated or earlier on fail
 */
int read_write_loop_sender(const int sfd, stack_t *stack) {
    sendingStack = stack;
    bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];
    nextWindow = MAX_WINDOW_SIZE;

    fd_set fdSet;

    struct timeval timeout; // should never be obtained
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // TODO smth else ??

    seqnumToSend = 0; // first pkt to send

    while(stack_size(sendingStack) > 0) {
        bufSize = 16 + MAX_PAYLOAD_SIZE;

        nextPktToSend = stack_get_pkt(sendingStack, seqnumToSend);
        if(nextPktToSend == NULL) {
            if(stack_size(sendingStack) == 0) {
                fprintf(stderr, "All packets are sent.\n");
                break;
            }
            fprintf(stderr, "Next packet to send failed\n");
            return EXIT_FAILURE;
        }

        // setting correct timestamp and window of packet [nextPktToSend]
        pktStatusCode = pkt_set_timestamp(nextPktToSend, (uint32_t) time(NULL));
        if (pktStatusCode != PKT_OK) {
            fprintf(stderr, "Error in pkt_set_timestamp()\n");
            return EXIT_FAILURE;
        }
        statusCode = pkt_set_window(nextPktToSend, nextWindow);
        if (statusCode != PKT_OK) {
            fprintf(stderr, "Error in pkt_set_window()\n");
            return EXIT_FAILURE;
        }
        set_nextWindow();

        pktStatusCode = pkt_encode(nextPktToSend, buf, &bufSize);
        if(pktStatusCode != PKT_OK) {
            fprintf(stderr, "Encode failed\n");
            return EXIT_FAILURE;

        }

        fprintf(stderr, GRN "=> DATA\tSeqnum : %i\tLength : %i\tTimestamp : %i" RESET "\n\n",
                pkt_get_seqnum(nextPktToSend), pkt_get_length(nextPktToSend), pkt_get_timestamp(nextPktToSend));

        justWritten = (size_t) write(sfd, buf, bufSize);
        if((int) justWritten < 0) {
            fprintf(stderr, "Write failed\n");
            return EXIT_FAILURE;
        }

        receiverWindowSize--;
        if(hasRTed || hasNACKed) { // avoid go-back-n if RT has timed out for one packet
            seqnumToSend = lastEncodedSeqnum + 1;
            hasRTed = 0; // reset
            hasNACKed = 0;
        } else {
            lastEncodedSeqnum = pkt_get_seqnum(nextPktToSend);
            seqnumToSend++;
        }

        FD_ZERO(&fdSet);
        FD_SET(sfd, &fdSet);
        select(sfd + 1, &fdSet, NULL, NULL, &timeout);
        if(FD_ISSET(sfd, &fdSet)) {
            justRead = (size_t) read(sfd, buf, replySize);
            if((int) justRead < 0) {
                fprintf(stderr, "Read failed\n");
                return EXIT_FAILURE;
            }

            if((int) justRead > 0) { // ACK or NACK received
                lastPktReceived = pkt_new();
                pktStatusCode = pkt_decode(buf, justRead, lastPktReceived);
                if(pktStatusCode != PKT_OK) {
                    fprintf(stderr, "Decode failed : %i (there is a TODO here)\n", pktStatusCode);
                    // TODO if E_UNCONSISTENT, just discard and do not FAIL
                    return EXIT_FAILURE;
                }

                if(pkt_get_type(lastPktReceived) == PTYPE_ACK) {

                    fprintf(stderr, RED "~ ACK\tSeqnum : %i\tLength : %i\tTimestamp : %i\tStack_size : %i" RESET "\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived), (int) stack_size(sendingStack));

                    receiverWindowSize = pkt_get_window(lastPktReceived);

                    uint8_t seqnumAcked = pkt_get_seqnum(lastPktReceived);
                    int amountRemoved = stack_remove_acked(sendingStack, seqnumAcked); // remove all nodes prior to [seqnumAcked] (not included) from [sendingStack]
                    fprintf(stderr, RED "~ Cummulative ACK for %i packet(s)" RESET "\n", amountRemoved);

                } else if(pkt_get_type(lastPktReceived) == PTYPE_NACK) {

                    fprintf(stderr, BLU "~ NACK\tSeqnum : %i\tLength : %i\tTimestamp : %i" RESET "\n\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived));

                    receiverWindowSize = pkt_get_window(lastPktReceived);

                    seqnumToSend = pkt_get_seqnum(lastPktReceived);

                    hasNACKed = 1;

                } else {
                    fprintf(stderr, "Received something else than ACK or NACK\n");
                    return EXIT_FAILURE;
                }
                free(lastPktReceived);
            } else { // justRead == 0
                fprintf(stderr, "Nothing received but something expected\n");
                return EXIT_FAILURE;
            }
        } else { // nothing received yet
            int wait = 1;
            while(receiverWindowSize == 0 && wait) {
                statusCode = check_for_RT();
                // TODO also check for ACK or NACK
                if(statusCode != 0) {
                    if(statusCode == 101) { // a RT has expired
                        wait = 0;
                    } else {
                        return statusCode;
                    }
                }

                // wait until sender can receive something

            } // while(receiverWindowSize == 0 && wait)

            // did not wait
        }
    } // while(stack_size(sendingStack) > 0)


    fprintf(stderr, GRN "=> CLOSING CONNECTION" RESET "\n\n");
    /*
     * Terminating connexion TODO not waiting for ACK !!?? How to end properly the connexion ?
     *
    pkt_t *terminateConnexionPkt = pkt_new();
    pkt_set_type(terminateConnexionPkt, PTYPE_DATA);
    pkt_set_window(terminateConnexionPkt, nextWindow);
    pkt_set_seqnum(terminateConnexionPkt, lastEncodedSeqnum);
    pkt_set_timestamp(terminateConnexionPkt, (uint32_t) time(NULL));
    pkt_set_length(terminateConnexionPkt, 0);

    size_t len = sizeof(terminateConnexionPkt);
    pktStatusCode = pkt_encode(terminateConnexionPkt, buf, &len);
    if(pktStatusCode != PKT_OK) {
        fprintf(stderr, "Unable to encode the terminating connexion packet : %i\n", pktStatusCode);
    }
    pkt_del(terminateConnexionPkt);

    fprintf(stderr, GRN "=> CLOSING CONNECTION" RESET "\n\n");

    justWritten = (size_t) write(sfd, buf, len);
    if((int) justWritten < 0) {
        fprintf(stderr, "Write failed\n");
        return EXIT_FAILURE;
    }*/

    // TODO : delink, debound and deconnect connexion properly ?

    pkt_del(nextPktToSend);
    pkt_del(lastPktReceived);

    return EXIT_SUCCESS;
}

int check_for_RT() {
    node_t *runner = sendingStack->first;
    while(runner->seqnum <= lastEncodedSeqnum) {
        if((uint32_t) (time(NULL) - pkt_get_timestamp(runner->pkt)) > RTlength) {
            seqnumToSend = runner->seqnum;
            fprintf(stderr, "RT ran out on pkt with seqnum %i\n", runner->seqnum);
            hasRTed = 1;
            return 101;
        } else {
            runner = runner->next;
        }
    }
    return EXIT_SUCCESS;
}

void set_nextWindow() {
    // TODO implementation : set the next size of our senders's receiving buffer
    nextWindow = MAX_WINDOW_SIZE; // always the same (no buffering problem on sender)
}