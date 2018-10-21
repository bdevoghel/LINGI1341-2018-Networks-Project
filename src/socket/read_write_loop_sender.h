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

#ifndef __READ_WRITE_LOOP_H_
#define __READ_WRITE_LOOP_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "../packet/packet.h" // MAX_PAYLOAD_SIZE
#include "../stack/stack.h"

/**
 * Loop sending packets (read from [stack]) on a socket,
 * while reading ACKs and NACKs from the socket
 * @sfd : the socket file descriptor. It is both bound and connected.
 * @stack : stack containing all the packets to send
 * @return : as soon as connexion was terminated or earlier on fail
 */
int read_write_loop_sender(const int sfd, stack_t *stack);

#endif
