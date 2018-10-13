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
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

struct __attribute__((__packed__)) pkt {
    ptypes_t type;
    uint8_t tr : 1;
    uint8_t window : 5;
    uint8_t seqnum : 8;
    uint16_t length : 16;
    uint32_t timestamp : 32;
    char *payload;
    uint32_t crc1 : 32;
    uint32_t crc2 : 32;
};

pkt_t* pkt_new() {
    pkt_t *newPkt = (pkt_t *) calloc(1, sizeof(pkt_t));
    if(newPkt == NULL) {
        fprintf(stderr, "Out of memory at pkt creation\n");
    } else {
        newPkt->payload = NULL;
    }

    return newPkt;
}

void pkt_del(pkt_t *pkt) {
    free(pkt->payload);
    free(pkt);
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
    if(data == NULL) {
        return E_UNCONSISTENT;
    }
    if(len < 12) {
        return E_NOHEADER;
    }

    // Lecture du header
    ptypes_t pkt_type = (ptypes_t) ((data[0] & 0xC0) >> 6);
    if(pkt_type == 0) {
        return E_TYPE;
    }
    pkt_set_type(pkt, pkt_type);

    uint8_t pkt_tr = (uint8_t) ((data[0] & 0x20) >> 5);
    if((pkt_type != PTYPE_DATA) && (pkt_get_tr(pkt) != 0)) { // condition sur le camp TR
        return E_UNCONSISTENT;
    }
    pkt_set_tr(pkt, pkt_tr);

    uint8_t pkt_window = (uint8_t) (data[0] & 0x1F);
    pkt_set_window(pkt, pkt_window);

    uint8_t pkt_seqnum = (uint8_t) data[1];
    pkt_set_seqnum(pkt, pkt_seqnum);

    uint16_t pkt_length = (uint16_t) ((data[2] << 8) | data[3]);
    pkt_set_length(pkt, pkt_length);

    uint32_t pkt_timestamp = (uint32_t) ((data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7]);
    pkt_set_timestamp(pkt, pkt_timestamp);

    uint32_t pkt_crc1 = (uint32_t) ((data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11]);
    pkt_set_crc1(pkt, pkt_crc1);

    // TODO check CRC and coherence
    /*
    pkt_status_code code = PKT_OK;

    pkt_set_window(pkt, (const uint8_t) data[0] & 31);
    pkt_set_seqnum(pkt, (const uint8_t) data[1]);

    uint16_t L = ((data[2] & 255) << 8) | (data[3] & 255) ;

    uint16_t Lp = (4 - (L%4))%4; // longueur du padding

    if (((L + 8 + Lp) > (uint16_t) len) && code != E_NOPAYLOAD){
        code = E_UNCONSISTENT;
    }
    if(code == PKT_OK){
        code = pkt_set_length(pkt, L);
    }
    else{
        pkt_set_length(pkt, L);
    }
    // retourne l'erreur une fois le header forme
    if(code != PKT_OK) {

        return code;
    }
*/

    // Lecture du payload
    char payload[pkt_length];
    int i;
    for(i=0 ; i<pkt_length ; i++) {
        payload[i] = data[12 + i];
    }
    pkt_set_payload(pkt, payload, pkt_length);

    // Lecture du CRC2
    uint32_t pkt_crc2 = (uint32_t) ((data[12 + i + 1] << 24) | (data[12 + i + 2] << 16) | (data[12 + i + 3] << 8) | data[12 + i + 4]);
    pkt_set_crc2(pkt, pkt_crc2);

    // TODO check CRC and coherence
    /*
    uint32_t a = (uint8_t) data[4+L+Lp];
    uint32_t b = (uint8_t) data[5+L+Lp];
    uint32_t c = (uint8_t) data[6+L+Lp];
    uint32_t d = (uint8_t) data[7+L+Lp];
    uLong crc = (uLong) (((a&255)<<24)|((b&255)<<16)|((c&255)<<8)|(d&255));
    uLong crcOld = crc32(0L, Z_NULL, 0);
    crcOld = crc32(crcOld, (const Bytef*) data, (uint) (4+L+Lp));
    if(crc != crcOld) {
        return E_CRC;
    }
    pkt_set_crc(pkt, crc);
     */

    return PKT_OK;
}

pkt_status_code pkt_encode(const pkt_t *pkt, char *buf, size_t *len) {
    if(pkt == NULL || ((pkt_get_type(pkt) != PTYPE_DATA) && (pkt_get_tr(pkt) != 0)) // condition sur le camp TR
                   // FAUX (voir enonce) : || pkt_get_length(pkt) > pkt_get_tr(pkt) // tronque mais avec payload
                   ) {
        return E_UNCONSISTENT;
    }
    if(buf == NULL) {
        return E_NOMEM;
    }
    if(len == NULL) {
        return E_LENGTH;
    }

    uint16_t pkt_length = pkt_get_length(pkt);
    if(*len < (size_t) (3*4 + pkt_length + 4*pkt_get_tr(pkt))) { // taille requise dans buffer (header + payload + crc2)
        return E_NOMEM;
    }

    // Ecriture du header dans le buffer
    buf[0] = (char) (((uint8_t) pkt_get_type(pkt)) << 6) | ((pkt_get_tr(pkt)) << 5) | pkt_get_window(pkt);
    buf[1] = (char) pkt_get_seqnum(pkt);

    buf[2] = (char) ((pkt_length & 0xF0) >> 8);
    buf[3] = (char) (pkt_length & 0x0F);

    uint32_t pkt_timestamp = pkt_get_timestamp(pkt);
    buf[4] = (char) ((pkt_timestamp & 0xF000) >> 24);
    buf[5] = (char) ((pkt_timestamp & 0x0F00) >> 16);
    buf[6] = (char) ((pkt_timestamp & 0x00F0) >> 8);
    buf[7] = (char) (pkt_timestamp & 0x000F);

    uint32_t pkt_crc1 = pkt_get_crc1(pkt);
    buf[8] = (char) ((pkt_crc1 & 0xF000) >> 24);
    buf[9] = (char) ((pkt_crc1 & 0x0F00) >> 16);
    buf[10] = (char) ((pkt_crc1 & 0x00F0) >> 8);
    buf[11] = (char) (pkt_crc1 & 0x000F);

    size_t charWritten = 12;

    // Ecriture du payload dans le buffer
    int i = 0;
    if(pkt_get_payload(pkt) != NULL) {
        for(i=0; i <= pkt_length; i++) {
            buf[8 + i] = pkt_get_payload(pkt)[i];
            charWritten++;
        }
    } else {
        // TODO ?
    }

    // Ecriture du CRC2 dans le buffer
    uint32_t pkt_crc2 = pkt_get_crc2(pkt);
    if(pkt_crc2 != 0) {
        buf[8 + i + 1] = (char) ((pkt_crc2 & 0xF000) >> 24);
        buf[8 + i + 2] = (char) ((pkt_crc2 & 0x0F00) >> 16);
        buf[8 + i + 3] = (char) ((pkt_crc2 & 0x00F0) >> 8);
        buf[8 + i + 4] = (char) (pkt_crc2 & 0x000F);
        charWritten += 4;
    }

    // indiquer le nombre de chars ecrits
    *len = (size_t) charWritten;

    return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t *pkt) {
    return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t *pkt) {
    return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t *pkt) {
    return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t *pkt) {
    return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t *pkt) {
    return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t *pkt) {
    return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t *pkt) {
    return pkt->crc1;
}

const char* pkt_get_payload(const pkt_t *pkt) {
    if(pkt->tr) {
        return NULL;
    }

    return pkt->payload;
}

uint32_t pkt_get_crc2(const pkt_t *pkt) {
    if(pkt->tr) {
        return 0;
    }

    return pkt->crc2;
}

pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    pkt->type = type;
    return PKT_OK;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    if(tr > 1) {
        return E_TR;
    }
    pkt->tr = tr;
    return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    if(window > 31) {
        return E_WINDOW;
    }
    pkt->window = window;
    return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    pkt->seqnum = seqnum;
    return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    if(length > 512) {
        return E_LENGTH;
    }
    pkt->length = length;
    return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    pkt->timestamp = timestamp;
    return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    pkt->crc1 = crc1;
    return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    pkt->crc2 = crc2;
    return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length) {
    if(pkt == NULL) {
        return E_UNCONSISTENT;
    }
    if(data == NULL || length > 512){
        return E_LENGTH;
    }
    pkt_status_code code = pkt_set_length(pkt, length);
    if(code) {
        return code;
    }

    pkt->payload = (char *) malloc(sizeof(char)*length);
    if(pkt->payload == NULL) {
        fprintf(stderr, "Out of memory at payload setting\n");
        return E_NOMEM;
    }

    int i;
    for(i=0 ; i<length ; i++) {
        pkt->payload[i] = data[i];
    }

    return PKT_OK;
}