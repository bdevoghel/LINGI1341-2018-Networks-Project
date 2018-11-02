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

struct __attribute__((__packed__)) pkt {
    uint8_t window : 5;
    uint8_t tr : 1;
    ptypes_t type : 2;
    uint8_t seqnum ;
    uint16_t length;
    uint32_t timestamp;
    char *payload;
    uint32_t crc1;
    uint32_t crc2;
};

pkt_t* pkt_new() {
    return (pkt_t *) calloc(1, sizeof(pkt_t));
}

void pkt_del(pkt_t *pkt) {
    if(pkt == NULL) {
        return;
    }
    if(pkt->payload != NULL) {
        free(pkt->payload);
    }
    free(pkt);
    pkt = NULL;
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt) {
    if(data == NULL || len == 0) {
        return E_UNCONSISTENT;
    }
    if(len < 12) {
        return E_NOHEADER;
    }

    pkt_status_code statusCode;

    /*
     * Lecture du header
     */
    memcpy(pkt, data+0, sizeof(uint8_t));

    memcpy(&(pkt->seqnum), data+1, sizeof(uint8_t));

    uint16_t pktn_length;
    memcpy(&pktn_length, data+2, sizeof(uint16_t));
    statusCode = pkt_set_length(pkt, ntohs(pktn_length));
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    uint32_t pkt_timestamp;
    memcpy(&pkt_timestamp, data+4, sizeof(uint32_t));
    statusCode = pkt_set_timestamp(pkt, pkt_timestamp);
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    uint32_t pktn_crc1;
    memcpy(&pktn_crc1, data+8, sizeof(uint32_t));
    statusCode = pkt_set_crc1(pkt, ntohl(pktn_crc1));
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    /*
     * Lecture du payload
     */
    char payload[pkt_get_length(pkt)];
    memcpy(payload, data+12, pkt_get_length(pkt));
    statusCode = pkt_set_payload(pkt, payload, pkt_get_length(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    /*
     * Lecture du CRC2
     */
    uint32_t pktn_crc2;
    memcpy(&pktn_crc2, data+12+pkt_get_length(pkt), sizeof(uint32_t));
    statusCode = pkt_set_crc2(pkt, ntohl(pktn_crc2));
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    /*
     * Validation des CRC et des coherences
     */
    statusCode = pkt_set_type(pkt, pkt_get_type(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_tr(pkt, pkt_get_tr(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_window(pkt, pkt_get_window(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_seqnum(pkt, pkt_get_seqnum(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_length(pkt, pkt_get_length(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_timestamp(pkt, pkt_get_timestamp(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_crc1(pkt, pkt_get_crc1(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    statusCode = pkt_set_crc2(pkt, pkt_get_crc2(pkt));
    if(statusCode != PKT_OK) {
        return statusCode;
    }

    uint8_t trBeforeCRC1 = pkt_get_tr(pkt);
    statusCode = pkt_set_tr(pkt, 0);
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    uint32_t crc1Calculated = (uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) data + 0, (uInt) sizeof(char) * 8);
    if(pkt_get_crc1(pkt) != crc1Calculated) {
        return E_CRC;
    }
    statusCode = pkt_set_tr(pkt, trBeforeCRC1);
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    if(pkt_get_length(pkt) != 0) {
        uint32_t crc2Calculated = (uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) data + 12,
                                                   (uInt) sizeof(char) * pkt_get_length(pkt));
        if (pkt_get_crc2(pkt) != crc2Calculated) {
            return E_CRC;
        }
    }

    if(pkt_get_type(pkt) == 0) {
        return E_TYPE;
    }
    if((pkt_get_type(pkt) != PTYPE_DATA) && (pkt_get_tr(pkt) != 0)) { // condition sur le champ TR
        return E_UNCONSISTENT;
    }

    return PKT_OK;
}

pkt_status_code pkt_encode(pkt_t *pkt, char *buf, size_t *len) {
    if(pkt == NULL || ((pkt_get_type(pkt) != PTYPE_DATA) && pkt_get_tr(pkt))) { // condition sur le champ TR
        return E_UNCONSISTENT;
    }
    if(buf == NULL) {
        return E_NOMEM;
    }
    if(len == NULL) {
        return E_LENGTH;
    }

    uint16_t pktn_length = htons(pkt_get_length(pkt));
    if(*len < (size_t) (3*4 + pkt_get_length(pkt) + 4*pkt_get_tr(pkt))) { // taille requise dans buffer (header + payload + crc2)
        return E_NOMEM;
    }

    pkt_status_code statusCode;

    /*
     * Ecriture du header dans le buffer
     */
    memcpy(buf+0, pkt, sizeof(uint8_t));

    memcpy(buf+1, &(pkt->seqnum), sizeof(uint8_t));

    memcpy(buf+2, &pktn_length, sizeof(uint16_t));

    uint32_t pkt_timestamp = pkt_get_timestamp(pkt);
    memcpy(buf+4, &pkt_timestamp, sizeof(uint32_t));

    uint32_t pktn_crc1 = htonl((uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) buf + 0, (uInt) sizeof(char) * 8));
    //uint32_t pktn_crc1 = htonl(pkt_get_crc1(pkt));
    statusCode = pkt_set_crc1(pkt, ntohl(pktn_crc1));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    memcpy(buf+8, &pktn_crc1, sizeof(uint32_t));

    int charWritten = 3 * sizeof(uint32_t);

    if(pkt_get_tr(pkt)) {
        if(pkt_get_length(pkt) != 0) {
            return E_UNCONSISTENT;
        }
        *len = (size_t) charWritten; // indiquer le nombre de chars ecrits
        return PKT_OK;
    }

    /*
     * Ecriture du payload dans le buffer
     */
    if(pkt_get_payload(pkt) != NULL && pkt_get_tr(pkt) == 0) {
        memcpy(buf+12, pkt->payload, pkt_get_length(pkt));
    } else {
        //return E_UNCONSISTENT;
    }
    charWritten += pkt_get_length(pkt);

    /*
     * Ecriture du CRC2 dans le buffer
     */
    uint32_t pktn_crc2 = htonl((uint32_t) crc32(crc32(0L, Z_NULL, 0), (Bytef *) buf + 12, (uInt) pkt_get_length(pkt)));
    //uint32_t pktn_crc2 = htonl(pkt_get_crc2(pkt));
    statusCode = pkt_set_crc2(pkt, ntohl(pktn_crc2));
    if(statusCode != PKT_OK) {
        return statusCode;
    }
    if(pktn_crc2 != 0 && pkt_get_tr(pkt) == 0) {
        memcpy(buf+charWritten, &pktn_crc2, sizeof(uint32_t));
        charWritten += sizeof(uint32_t);
    } else {
        //return E_UNCONSISTENT;
    }

    *len = (size_t) charWritten; // indiquer le nombre de chars ecrits

    return PKT_OK;
}

ptypes_t pkt_get_type(const pkt_t *pkt) {
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

uint32_t pkt_get_timestamp(const pkt_t *pkt) {
    return pkt->timestamp;
}

uint32_t pkt_get_crc1(const pkt_t *pkt) {
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
    if(window > MAX_WINDOW_SIZE) {
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
    if(length > MAX_PAYLOAD_SIZE) {
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
    if(data == NULL || length > MAX_PAYLOAD_SIZE){
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

    memcpy(pkt->payload, data, length);

    return PKT_OK;
}