#ifndef __DNS_RECORD_H__
#define __DNS_RECORD_H__

#include <stdint.h>
//typedef unsigned short uint16_t;
//typedef unsigned int uint32_t;


// struct for the header section
typedef struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

// struct for the question section, skip name
typedef struct dns_question {
    uint16_t qtype;
    uint16_t qclass;
} dns_question_t;

// struct for the answer section, skip name
typedef struct dns_resource {
    uint16_t type;
    uint16_t class_name;
    uint32_t ttl;
    uint16_t rdlength;
} dns_resource_t;

#define DNS_MSG_MAX_LEN    (65535)
/* HEADER */
// mask in flag field
#define DNS_FLAG_QR        (1 << 0)
#define DNS_MASK_OPCODE    (0xF << 1)
#define DNS_FLAG_AA        (1 << 5)
#define DNS_FLAG_TC        (1 << 6)
#define DNS_FLAG_RD        (1 << 7)
#define DNS_FLAG_RA        (1 << 8)
#define DNS_FLAG_Z         (0b111 << 9)
#define DNS_FLAG_RCODE     (0xF << 12)

#define DNS_SET_FLAG(header, flag) ({\
        uint16_t flags = ntohs(header->flags); \
        flags |= flag; \
        (header)->flags = htons(flags);})
#define DNS_CLR_FLAG(header, flag) ({\
        uint16_t flags = ntohs((header)->flags); \
        flags &= ~(flag); \
        header->flags = htons(flags);})
#define DNS_CHECK_FLAG(request, mask) (ntohs((header)->flags) & mask)
#define DNS_GET_ID(header) (ntohs((header)->id))
#define DNS_GET_QDCOUNT(header) (ntohs((header)->qdcount)) // number of questions
#define DNS_GET_ANCOUNT(header) (ntohs((header)->ancount)) // number of answers

/* QUESTION */
#define DNS_GET_QTYPE(question) (ntohs(question->qtype))
#define DNS_GET_QCLASS(question) (ntohs(question->qtype))

/* RESOURCE */
#define DNS_GET_TYPE(resource) (ntohs(resource->type))
#define DNS_GET_CLASS(resource) (ntohs(resource->class_name))
#define DNS_GET_TTL(resource) (ntohs(resource->ttl))
#define DNS_GET_RDLEN(resource) (ntohs(resource->rdlength))

#define DNS_TYPE_A      (1) // type - A record, QTYPE
#define DNS_CLASS_IP    (1) // class - ip, QTYPE / TYPE


//void dns_header_set_qr(dns_header_t *record);
//
//void dns_header_clr_qr(dns_header_t *record);
//
//void dns_header_set_aa(dns_header_t *record);
//
//void dns_header_set_opcode(dns_header_t *record, int opcode);
//
//int dns_header_get_opcode(dns_header_t *record);
//
//int dns_header_get_id(dns_header_t *record);
//
//int dns_header_get_qdcount(dns_header_t *record);

/**
 * @brief Initiate dns record struct.
 * @param record Pointer to the record structure.
 * @return Void.
 */
void dns_header_init(dns_header_t *header, int id);


int parse_name(const char *question, int size, char *result);
int translate_name(const char *addr, char *result);
int parse_question(const char *buf, int size, char *result);
int parse_resource(const char *buf, int size, struct in_addr *result);
int dns_isinvalid(dns_header_t *header, int is_request);
int dns_parse_request(const char *buf, int size, char *result);
int dns_parse_response(const char *response, int size, struct in_addr *result);



int dns_gen_response(const char *qname, const char *ip_addr, uint16_t dns_id, int rcode, char *result);

#endif /* !__DNS_RECORD_H__ */
