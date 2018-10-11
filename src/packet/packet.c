/**
 * Projet1 - Reseaux Informatiques
 * Octobre 2018
 *
 * Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
 * NOMA    : 59101600           &   27901600
 *
 * Contenu repris et complete de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/format-des-segments
 */

#include "packet.h"
// autres #includes TODO

struct __attribute__((__packed__)) pkt {
    // TODO
};

pkt_t* pkt_new() {
    // TODO
}

void pkt_del(pkt_t *pkt) {
    // TODO
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
    // TODO
}

pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len) {
    // TODO
}

ptypes_t pkt_get_type  (const pkt_t*) {
    // TODO
}

uint8_t  pkt_get_tr(const pkt_t*) {
    // TODO
}

uint8_t  pkt_get_window(const pkt_t*) {
    // TODO
}

uint8_t  pkt_get_seqnum(const pkt_t*) {
    // TODO
}

uint16_t pkt_get_length(const pkt_t*) {
    // TODO
}

uint32_t pkt_get_timestamp   (const pkt_t*) {
    // TODO
}

uint32_t pkt_get_crc1   (const pkt_t*) {
    // TODO
}

uint32_t pkt_get_crc2   (const pkt_t*) {
    // TODO
}

const char* pkt_get_payload(const pkt_t*) {
    // TODO
}

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type) {
    // TODO
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr) {
    // TODO
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
    // TODO
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum) {
    // TODO
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length) {
    // TODO
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp) {
    // TODO
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1) {
    // TODO
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2) {
    // TODO
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length) {
    // TODO
}