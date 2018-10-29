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
uint8_t seqnumToSend;
pkt_t *nextPktToSend;
uint8_t lastEncodedSeqnum; // seqnum of last pkt transmitted successfully
pkt_t *lastPktReceived;
uint8_t lastSeqnumAcked;

int packetsToSend;
int packetsSent;

size_t bufSize;
size_t replySize = 12;
uint8_t receiverWindowSize = 1;
uint32_t RTlength = 4; // == max(RTT) * 2 [s]

uint8_t nextWindow;

// flags and return values of function calls
size_t justWritten;
size_t justRead;
pkt_status_code pktStatusCode;
int statusCode;
int hasRTed = 0;
int hasNACKed = 0;
int waitForLastPktResponse = 0;
int mainBreak = 0;

int count = 0;

/**
 * Goes over all already sent packets and signals if a RT timer has expired
 * @return : 1 if RT has expired, EXIT_SUCCES otherwise
 */
int check_for_RT();

/**
 * Precesses a response (ACK or NACK) that can be read from the socket (sfd)
 * @param sfd : socket file descriptor to read from
 * @return : -1 if response discarded ; -2 if read failed (no connection yet) ; -3 if read failed (connection closed already) ; EXIT_SUCCESS if everything went well
 */
int process_response(const int sfd);

/**
 * checks if [seqnum] is in range of [lastSeqnumAcked] (+31, taking loop around 256 into account)
 * @return : 1 if true, 0 if false
 */
int isInRange(uint8_t seqnum);

/**
 * Updates the window size of the sender to communicate to the receiver
 */
void update_nextWindow();

/**
 * Loop sending packets (read from [stack]) on a socket,
 * while reading ACKs and NACKs from the socket
 * @sfd : the socket file descriptor. It is both bound and connected.
 * @stack : stack containing all the packets to send
 * @return : as soon as connection was terminated or earlier on fail; -2 if read failed
 */
int read_write_loop_sender(const int sfd, stack_t *stack) {
    // init variables
    sendingStack = stack; // for global access
    bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];
    nextWindow = MAX_WINDOW_SIZE;
    packetsToSend = (int) stack_size(sendingStack);
    packetsSent = 0;
    seqnumToSend = 0; // first pkt to send
    lastSeqnumAcked = 0; // none have been ACKed yet

    // variables for synchronous I/O multiplexing
    fd_set fdSet; // file descriptor set for select()
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // min wait time for ACK or NACK [µs]


    while(stack_size(sendingStack) > 0 && !mainBreak ) {
        bufSize = 16 + MAX_PAYLOAD_SIZE; // reset

        if(packetsSent == packetsToSend - 1 && stack_size(sendingStack) > 1 && !pkt_get_timestamp(sendingStack->last->pkt) && !hasNACKed && !hasRTed) {
            //fprintf(stderr, "Wait for last packet response\n");
            waitForLastPktResponse = 1;
        } else {

            // check if seqnumToSend is out of receivers window
            if(seqnumToSend < lastSeqnumAcked && lastSeqnumAcked <= 256 - MAX_WINDOW_SIZE) {
                seqnumToSend = lastSeqnumAcked;
            }
            if(!isInRange(seqnumToSend)) {
                seqnumToSend = (lastSeqnumAcked + MAX_WINDOW_SIZE - 1) % 256;
            }

            nextPktToSend = stack_get_pkt(sendingStack, seqnumToSend); // get next pkt to send
            if(nextPktToSend == NULL) {
                //fprintf(stderr, "Getting next packet to send failed\n");
            } else {

                if(pkt_get_timestamp(nextPktToSend) == 0) {
                    packetsSent++;
                }

                // setting correct timestamp and window of [nextPktToSend]
                pktStatusCode = pkt_set_timestamp(nextPktToSend, (uint32_t) time(NULL)); // set current time
                statusCode = pkt_set_window(nextPktToSend, nextWindow);
                if (statusCode != PKT_OK) {
                    fprintf(stderr, "Error in pkt_set_window()\n");
                }
                update_nextWindow();

                // encoding end sending pkt
                pktStatusCode = pkt_encode(nextPktToSend, buf, &bufSize);
                if (pktStatusCode != PKT_OK) {
                    fprintf(stderr, "Encode failed : status code = %i\n", pktStatusCode);
                    return EXIT_FAILURE; // cannot continue without causing problems later or
                }

                fprintf(stderr, GRN "=> DATA\tSeqnum : %i\tLength : %i\tTimestamp : %i\tWindow : %i" RESET "\n\n",
                        pkt_get_seqnum(nextPktToSend), pkt_get_length(nextPktToSend),
                        pkt_get_timestamp(nextPktToSend), receiverWindowSize);

                justWritten = (size_t) write(sfd, buf, bufSize);
                if ((int) justWritten < 0) {
                    fprintf(stderr, "Write failed\n");
                }
                count++;

                // updating values for next pkt to send
                if(receiverWindowSize > 0) {
                    receiverWindowSize--;
                }
                if (hasRTed || hasNACKed) { // avoid go-back-n if RT has timed out or if pkt was lost
                    seqnumToSend = (lastEncodedSeqnum + 1) % 256;
                    hasRTed = 0; // reset
                    hasNACKed = 0; // reset
                } else {
                    lastEncodedSeqnum = pkt_get_seqnum(nextPktToSend);
                    seqnumToSend = (seqnumToSend + 1) % 256;
                }
            }
        }

        int enteredAtLeastOnce = 0;
        while((receiverWindowSize == 0 && !mainBreak) || enteredAtLeastOnce == 0) { // wait until receiver can receive more packets or flagged
            // reset [fdSet] and see if a ACK or NACK arrived
            FD_ZERO(&fdSet);
            FD_SET(sfd, &fdSet);
            select(sfd + 1, &fdSet, NULL, NULL, &timeout);
            if(FD_ISSET(sfd, &fdSet) /*|| waitForLastPktResponse*/) { // some response received
                statusCode = process_response(sfd);
                if(statusCode == 1) { // last pkt (N)ACKed
                    mainBreak = 1;
                } else if(statusCode == -2) { // read failed
                    return statusCode;
                } else if(statusCode == -3) { // read failed
                    return EXIT_SUCCESS;
                }
                break;
            }

            if(check_for_RT()) {
                // a RT has expired and [seqnumToSend] has been updated accordingly
                break;
            }

            enteredAtLeastOnce = 1;
        }
        fflush(stderr); // TODO remove for fluidity
    } // main while loop
    fprintf(stderr, RED "~ Connection termination ACKed. %i packets were sent." RESET "\n\n", count);

    pkt_del(nextPktToSend);
    pkt_del(lastPktReceived);

    return EXIT_SUCCESS;
}

int check_for_RT() {
    node_t *runner = sendingStack->first;
    while(pkt_get_timestamp(runner->pkt) != 0) { // loop trough all sent packets (which already received a timestamp)
        if((time(NULL) - pkt_get_timestamp(runner->pkt)) > RTlength) { // sent longer than [RTlength] seconds ago
            seqnumToSend = runner->seqnum;
            fprintf(stderr, "RT ran out on pkt with seqnum %i\n", runner->seqnum);
            hasRTed = 1;
            return 1;
        } else {
            runner = runner->next;
        }

        if(runner == sendingStack->first) {
            break;
        }
    }
    return EXIT_SUCCESS;
}

int process_response(const int sfd) {
    bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];

    justRead = (size_t) read(sfd, buf, replySize);
    if((int) justRead < 0) {
        if(packetsToSend == packetsSent) {
            fprintf(stderr, "Read failed. Probably because receiver has already closed connection\n");
            return -3;
        } else {
            fprintf(stderr, "Read failed. Possibely because receiver is not launched yet\n");
            return -2;
        }
    }

    if((int) justRead > 0) { // ACK or NACK received

        lastPktReceived = pkt_new();
        pktStatusCode = pkt_decode(buf, justRead, lastPktReceived);
        if(pktStatusCode != PKT_OK) {
            fprintf(stderr, "Decode failed : %i (there is a TODO here)\n", pktStatusCode);
            // TODO if E_UNCONSISTENT, just discard and do not FAIL
            return -1; // response discarded
        }


        if(pkt_get_type(lastPktReceived) == PTYPE_ACK) {

            fprintf(stderr, RED "~ ACK\tSeqnum : %i\tLength : %i\tTimestamp : %i\tWindow : %i\tStack_size : %li" RESET "\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived), pkt_get_window(lastPktReceived), stack_size(sendingStack));

            receiverWindowSize = pkt_get_window(lastPktReceived);

            lastSeqnumAcked = pkt_get_seqnum(lastPktReceived);

            int amountRemoved = stack_remove_acked(sendingStack, lastSeqnumAcked); // remove all nodes prior to [lastSeqnumAcked] (not included) from [sendingStack]
            fprintf(stderr, RED "~ Cummulative ACK for %i packet(s)" RESET "\n\n", amountRemoved);

            if(lastSeqnumAcked == (packetsToSend % 256) && stack_size(sendingStack) == 1) { // ACKed terminating connection packet
                return 1;
            }
        } else if(pkt_get_type(lastPktReceived) == PTYPE_NACK) {

            fprintf(stderr, BLU "~ NACK\tSeqnum : %i\tLength : %i\tTimestamp : %i\tWindow : %i" RESET "\n\n", pkt_get_seqnum(lastPktReceived), pkt_get_length(lastPktReceived), pkt_get_timestamp(lastPktReceived), pkt_get_window(lastPktReceived));

            receiverWindowSize = pkt_get_window(lastPktReceived);

            seqnumToSend = pkt_get_seqnum(lastPktReceived);

            stack_remove_acked(sendingStack, seqnumToSend); // remove all nodes prior to [seqnumToSend] (not included) from [sendingStack]

            if (seqnumToSend == (packetsToSend % 256) && stack_size(sendingStack) == 1) { // NACKed terminating connection packet
                return 1;
            }

            hasNACKed = 1;

        } else {
            fprintf(stderr, "Received something else than ACK or NACK\n");
            return -1; // response discarded
        }

        free(lastPktReceived);

    } else { // justRead == 0
        fprintf(stderr, "Nothing received but something expected\n");
    }
    return EXIT_SUCCESS;
}

int isInRange(uint8_t seqnum) {
    int diff = seqnum - lastSeqnumAcked;
    if (diff < 0) {
        diff += 256;
    }
    return diff < MAX_WINDOW_SIZE;
}

void update_nextWindow() {
    // TODO implementation : set the next size of our senders's receiving buffer
    nextWindow = MAX_WINDOW_SIZE; // always the same (no buffering problem on sender)
}