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

// Global variables and flags
int sfd;
stack_t *sendingStack;
uint8_t seqnumToSend;
uint8_t lastSeqnumAcked;
int lastSeqnumAckedCounter;
uint8_t receiverWindowSize;

int totalPacketsToSend;
int totalPacketsSent;

uint32_t RTlength = 4; // == max(RTT) * 2 [s]

int hasRTed = 0;
int hasNACKed = 0;
int hasFastRetransmitted = 0;
int lastpacketsSentCount = 0;

/**
 * Goes over all already sent packets and signals if a RT timer has expired
 * @return : 1 if RT has expired, EXIT_SUCCES otherwise
 */
int check_for_RT();

/**
 * Precesses a response (ACK or NACK) that can be read from the socket
 * @return : -1 if response discarded ; -2 if read failed (no connection yet) ; -3 if read failed (connection closed already) ; EXIT_SUCCESS if everything went well
 */
int process_response();

/**
 * Checks if [seqnum] is in range of [lastSeqnumAcked] (+31, taking loop around 256 into account)
 * @return : 1 if true, 0 if false
 */
int isInRange(uint8_t seqnum);

/**
 * Sends packets (read from [stack]) on a socket, while reading ACKs and NACKs from the socket
 * @socketFileDescriptor : socket file descriptor, both bound and connected
 * @stack : stack containing all the packets to send
 * @return : EXIT_SUCCESS as soon as connection was terminated with success ; EXIT_FAILURE if read failed (no connection yet)
 */
int read_write_loop_sender(const int socketFileDescriptor, stack_t *stack) {
    // Initializing variables
    sfd = socketFileDescriptor;
    sendingStack = stack;

    totalPacketsToSend = (int) stack_size(sendingStack);
    totalPacketsSent = 0;
    int packetsSentCount = 0;

    size_t bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];

    receiverWindowSize = 1; // starts with the assumption receiver can receive 1 packet

    pkt_t *nextPktToSend;
    uint8_t lastEncodedSeqnum = 0; // seqnum of last packet transmitted successfully (has to start with 0)
    seqnumToSend = 0; // first packet to send
    lastSeqnumAcked = 0; // none have been ACKed yet
    lastSeqnumAckedCounter = 0; // for fast retransmit

    // variables for synchronous I/O multiplexing
    fd_set fdSet; // file descriptor set for select()
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000; // min wait time for ACK or NACK [µs]


    int mainBreak = 0;
    while (stack_size(sendingStack) > 0 && !mainBreak) {
        bufSize = 16 + MAX_PAYLOAD_SIZE; // reset

        if (stack_size(sendingStack) < 256 && seqnumToSend != lastSeqnumAcked && stack_get_pkt(sendingStack, seqnumToSend) != NULL && pkt_get_length(stack_get_pkt(sendingStack, seqnumToSend)) == 0) {
            // fprintf(stderr, "Wait to send last packet\n");
        } else {

            if (!isInRange(seqnumToSend)) {
                //fprintf(stderr, "Packet seqnum to send is not in range\n");
            } else {

                if (receiverWindowSize == 0 && !hasRTed && !hasNACKed && !hasFastRetransmitted) {
                    //fprintf(stderr, "Receiver's window is full, not sending next packet yet\n");
                } else {

                    nextPktToSend = stack_get_pkt(sendingStack, seqnumToSend); // get next pkt to send
                    if (nextPktToSend == NULL) {
                        //fprintf(stderr, "Getting next packet to send failed\n");
                    } else {

                        if (pkt_get_timestamp(nextPktToSend) == 0) { // if this is a new packet to send
                            totalPacketsSent++;
                        }
                        
                        pkt_set_timestamp(nextPktToSend, (uint32_t) time(NULL)); // set current time
                        pkt_set_window(nextPktToSend, MAX_WINDOW_SIZE); // always the maximum (no buffering problem on sender));
                        
                        // encoding end sending pkt
                        pkt_status_code pktStatusCode = pkt_encode(nextPktToSend, buf, &bufSize);
                        if (pktStatusCode != PKT_OK) {
                            fprintf(stderr, "Encode failed : status code = %i. Fatal error.\n", pktStatusCode);
                        }

                        fprintf(stderr,
                                GRN "=> DATA\tSeqnum : %i\tLength : %i\tTimestamp : %i\tReceivers window was %i" RESET "\n\n",
                                pkt_get_seqnum(nextPktToSend), pkt_get_length(nextPktToSend),
                                pkt_get_timestamp(nextPktToSend), receiverWindowSize);

                        int justWritten = (int) write(sfd, buf, bufSize);
                        if (justWritten < 0) {
                            fprintf(stderr, "Write failed\n");
                        }

                        packetsSentCount++;

                        // updating values for next packet to send
                        if (hasRTed || hasNACKed || hasFastRetransmitted) { // avoid go-back-n
                            seqnumToSend = (lastEncodedSeqnum + 1) % 256; // restart where we left
                            hasRTed = 0; // reset
                            hasNACKed = 0; // reset
                            hasFastRetransmitted = 0; // reset
                        } else {
                            lastEncodedSeqnum = seqnumToSend; // = lastSeqnumAcked ; seqnum of packet just sent
                            seqnumToSend = (seqnumToSend + 1) % 256;
                            if (receiverWindowSize > 0) {
                                receiverWindowSize--;
                            }
                        }
                    }
                }
            }
        }

        int enteredAtLeastOnce = 0;
        while (receiverWindowSize == 0 || !enteredAtLeastOnce) { // wait until receiver can receive more packets
            // reset [fdSet] and see if a ACK or NACK arrived
            FD_ZERO(&fdSet);
            FD_SET(sfd, &fdSet);
            select(sfd + 1, &fdSet, NULL, NULL, &timeout);
            if (FD_ISSET(sfd, &fdSet)) { // if some response received
                int statusCode = process_response();
                if (statusCode == 1) { // last pkt (N)ACKed
                    mainBreak = 1;
                } else if (statusCode == -2) { // read failed
                    return EXIT_FAILURE;
                } else if (statusCode == -3) { // read failed but success
                    mainBreak = 1;
                }
                break;
            }

            if (check_for_RT()) {
                break; // a RT has expired and [seqnumToSend] has been updated accordingly
            }

            enteredAtLeastOnce = 1;
        }
        fflush(stderr); // being sure everything was printed

        if (lastpacketsSentCount > 5) { // closing connection pkt RT ran out more than 5 times
            break;
        }
        if (mainBreak && (lastSeqnumAcked != totalPacketsToSend % 256 && stack_size(sendingStack) < 256)) { // if wants to quit but last pkt not yet ACKed
            mainBreak = 0;
        }
    } // main while loop
    fprintf(stderr, RED "~ Connection termination ACKed.\n  %i packets were sent for this file of %i packets." RESET "\n\n", packetsSentCount,
            totalPacketsToSend);

    return EXIT_SUCCESS;
}

int check_for_RT() {
    node_t *runner = sendingStack->first;
    while (pkt_get_timestamp(runner->pkt) != 0) { // loop through all already sent packets (which already received a timestamp)
        if ((time(NULL) - pkt_get_timestamp(runner->pkt)) > RTlength) { // if sent longer than [RTlength] seconds ago
            seqnumToSend = runner->seqnum;

            fprintf(stderr, "RT ran out on pkt with seqnum %i\n", runner->seqnum);
            hasRTed = 1;

            if (stack_size(sendingStack) == 1) { // packet that closes connection RT ran out
                lastpacketsSentCount++;
            }

            return 1;
        }

        runner = runner->next;

        if (runner == sendingStack->first) { // looped trough everything
            break;
        }
    }
    return EXIT_SUCCESS;
}

int process_response() {
    size_t bufSize = 16 + MAX_PAYLOAD_SIZE;
    char buf[bufSize];
    size_t replySize = 12;

    int justRead = (int) read(sfd, buf, replySize);
    if (justRead < 0) {
        if (totalPacketsToSend == totalPacketsSent) {
            fprintf(stderr, "Read failed. Probably because receiver has already closed connection\n");
            return -3;
        } else {
            fprintf(stderr, "Read failed. Possibely because receiver is not launched yet\n");
            return -2;
        }
    }

    if (justRead > 0) { // ACK or NACK received

        pkt_t *lastPktReceived = pkt_new();
        if (pkt_decode(buf, justRead, lastPktReceived) != PKT_OK) {
            fprintf(stderr, RED"~ Decode of response failed (packet was probably corrupted). Ignoring packet.\n\n"RESET);
            pkt_del(lastPktReceived);
            return -1; // response discarded
        }

        if (pkt_get_type(lastPktReceived) == PTYPE_ACK) {
            fprintf(stderr, RED "~ ACK\tSeqnum : %i\tTimestamp : %i\tWindow : %i\n" RESET, pkt_get_seqnum(lastPktReceived), pkt_get_timestamp(lastPktReceived), pkt_get_window(lastPktReceived));

            receiverWindowSize = pkt_get_window(lastPktReceived);
            if (lastSeqnumAcked == pkt_get_seqnum(lastPktReceived)) {
                lastSeqnumAckedCounter++;
                fprintf(stderr, "lastSeqnumAckedCounter++ = %i\n", lastSeqnumAckedCounter);
                if (lastSeqnumAckedCounter == 2) { // same ACK received 3 times
                    seqnumToSend = lastSeqnumAcked; // fast retransmit

                    fprintf(stderr, "hasFastRetransmitted\n");
                    hasFastRetransmitted = 1;
                }
            } else {
                lastSeqnumAckedCounter = 0;
                lastSeqnumAcked = pkt_get_seqnum(lastPktReceived);
            }

            int amountRemoved = 0;
            pkt_t * toCheck = stack_get_pkt(sendingStack, lastSeqnumAcked);
            if (lastSeqnumAcked == (totalPacketsToSend % 256) || (toCheck != NULL && pkt_get_timestamp(toCheck) != 0)) {
                amountRemoved = stack_remove_acked(sendingStack, lastSeqnumAcked); // remove all nodes prior to [lastSeqnumAcked] (not included) from [sendingStack]
            }
            fprintf(stderr, RED "~ Cummulative ACK for %i packet(s)\t\tStack size : %li" RESET "\n\n", amountRemoved, stack_size(sendingStack));

            if (lastSeqnumAcked == (totalPacketsToSend % 256) && stack_size(sendingStack) == 1) { // ACKed terminating connection packet
                fprintf(stderr, "Last packet ACKed\n");
                pkt_del(lastPktReceived);
                return 1;
            }

        } else if (pkt_get_type(lastPktReceived) == PTYPE_NACK) {
            fprintf(stderr, BLU "~ NACK\tSeqnum : %i\tTimestamp : %i\tWindow : %i" RESET "\n\n", pkt_get_seqnum(lastPktReceived), pkt_get_timestamp(lastPktReceived), pkt_get_window(lastPktReceived));

            receiverWindowSize = pkt_get_window(lastPktReceived);
            seqnumToSend = pkt_get_seqnum(lastPktReceived);

            hasNACKed = 1;

            if(seqnumToSend == (totalPacketsToSend % 256) && stack_size(sendingStack) == 1) { // NACKed terminating connection packet
                fprintf(stderr, "Last packet NACKed\n");
                pkt_del(lastPktReceived);
                return 1;
            }

        } else {
            fprintf(stderr, "Received something else than ACK or NACK\n");
            pkt_del(lastPktReceived);
            return -1; // response discarded
        }

        pkt_del(lastPktReceived);

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