/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/format-des-segments
 */

#ifndef PROJECT1_VANDEWALLE_DEVOGHEL_PACKET_H
#define PROJECT1_VANDEWALLE_DEVOGHEL_PACKET_H

#include <stddef.h> //size_t
#include <stdint.h> // uintx_t
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h> // memcpy
#include <zlib.h> // CRC32
#include <arpa/inet.h> // htons / ntohs

/* Raccourci pour struct pkt */
typedef struct pkt pkt_t;

/* Types de paquets */
typedef enum {
    PTYPE_DATA = 1,
    PTYPE_ACK = 2,
    PTYPE_NACK = 3,
} ptypes_t;

/* Taille maximale permise pour le payload */
#define MAX_PAYLOAD_SIZE 512
/* Taille maximale de Window */
#define MAX_WINDOW_SIZE 31

/* Valeur de retours des fonctions */
typedef enum {
    PKT_OK = 0,     /* Le paquet a ete traite avec succes */
    E_TYPE,         /* Erreur liee au champs Type */
    E_TR,           /* Erreur liee au champ TR */
    E_LENGTH,       /* Erreur liee au champs Length  */
    E_CRC,          /* CRC invalide */
    E_WINDOW,       /* Erreur liee au champs Window */
    E_SEQNUM,       /* Numero de sequence invalide */
    E_NOMEM,        /* Pas assez de memoire */
    E_NOHEADER,     /* Le paquet n'a pas de header (trop court) */
    E_UNCONSISTENT, /* Le paquet est incoherent */
} pkt_status_code;

/**
 * Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur
 */
pkt_t *pkt_new();

/**
 * Libere le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associees
 */
void pkt_del(pkt_t *pkt);

/**
 * Decode des donnees recues et cree une nouvelle structure pkt.
 * Le paquet recu est en network byte-order.
 * La fonction verifie que:
 * - Le CRC32 du header recu est le meme que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet recu
 * @len: Le nombre de bytes recus
 * @pkt: Une struct pkt valide
 * @post: pkt est la representation du paquet recu
 *
 * @return: Un code indiquant si l'operation a reussi ou representant
 *         l'erreur rencontree.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt);

/**
 * Encode une struct pkt dans un buffer, pret a etre envoye sur le reseau
 * (c-a-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 *
 * @pkt: La structure a encoder
 * @buf: Le buffer dans lequel la structure sera encodee
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets ecrit dans le buffer
 * @return: Un code indiquant si l'operation a reussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(pkt_t *pkt, char *buf, size_t *len);

/**
 * Accesseurs pour les champs toujours presents du paquet.
 * Les valeurs renvoyees sont toutes dans l'endianness native
 * de la machine!
 */
ptypes_t pkt_get_type     (const pkt_t *pkt);
uint8_t  pkt_get_tr       (const pkt_t *pkt);
uint8_t  pkt_get_window   (const pkt_t *pkt);
uint8_t  pkt_get_seqnum   (const pkt_t *pkt);
uint16_t pkt_get_length   (const pkt_t *pkt);
uint32_t pkt_get_timestamp(const pkt_t *pkt);
uint32_t pkt_get_crc1     (const pkt_t *pkt);

/**
 * Renvoie un pointeur vers le payload du paquet, ou NULL s'il n'y
 * en a pas.
 */
const char* pkt_get_payload(const pkt_t *pkt);

/**
 * Renvoie le CRC2 dans l'endianness native de la machine. Si
 * ce field n'est pas present, retourne 0.
 */
uint32_t pkt_get_crc2(const pkt_t *pkt);

/**
 * Setters pour les champs obligatoires du paquet. Si les valeurs
 * fournies ne sont pas dans les limites acceptables, les fonctions
 * doivent renvoyer un code d'erreur adapte.
 * Les valeurs fournies sont dans l'endianness native de la machine!
 */
pkt_status_code pkt_set_type     (pkt_t *pkt, const ptypes_t type);
pkt_status_code pkt_set_tr       (pkt_t *pkt, const uint8_t tr);
pkt_status_code pkt_set_window   (pkt_t *pkt, const uint8_t window);
pkt_status_code pkt_set_seqnum   (pkt_t *pkt, const uint8_t seqnum);
pkt_status_code pkt_set_length   (pkt_t *pkt, const uint16_t length);
pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp);
pkt_status_code pkt_set_crc1     (pkt_t *pkt, const uint32_t crc1);

/**
 * Defini la valeur du champs payload du paquet.
 * @data: Une succession d'octets representants le payload
 * @length: Le nombre d'octets composant le payload
 * @POST: pkt_get_length(pkt) == length */
pkt_status_code pkt_set_payload(pkt_t*, const char *data, const uint16_t length);

/**
 * Setter pour CRC2. Les valeurs fournies sont dans l'endianness
 * native de la machine!
 */
pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2);

#endif //PROJECT1_VANDEWALLE_DEVOGHEL_PACKET_H
