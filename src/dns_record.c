#include <string.h>
#include <arpa/inet.h>
#include "dns_record.h"
//#include <stdint.h>
//#include <inttypes.h>


void dns_header_init(dns_header_t *header, int id) {
    memset(header, 0, sizeof(dns_header_t));
    header->id = htons(id);
}

//void dns_header_set_qr(dns_header_t *record) {
//    int flags = ntohs(record->flags);
//    flags |= DNS_FLAG_QR;
//    record->flags = htons(flags);
//}
//
//void dns_header_clr_qr(dns_header_t *record) {
//    int flags = ntohs(record->flags);
//    flags &= ~(DNS_FLAG_QR);
//    record->flags = htons(flags);
//}
//
//void dns_header_set_aa(dns_header_t *record) {
//    int flags = ntohs(record->flags);
//    flags |= DNS_FLAG_AA;
//    record->flags = htons(flags);
//}
//
//void dns_header_set_opcode(dns_header_t *record, int opcode) {
//    int flags = ntohs(record->flags);
//    opcode <<= 1;
//    opcode &= DNS_MASK_OPCODE;
//    flags &= ~(DNS_MASK_OPCODE);
//    flags |= opcode;
//    record->flags = htons(flags);
//}
//
//int dns_header_get_opcode(dns_header_t *record) {
//    int opcode = ntohs(record->flags) & DNS_MASK_OPCODE;
//    return opcode >> 1;
//}
//
//int dns_header_get_id(dns_header_t *record) {
//    return ntohs(record->id);
//}
//
//int dns_header_get_qdcount(dns_header_t *record) {
//    return ntohs(record->qdcount);
//}

int parse_name(const char *question, int size, char *result) {
	puts("parse name");
    char *pt = question;
    result[0] = '\0';
    while (size > 0) {
        unsigned lable_size = (unsigned) (*pt++);
        if (lable_size == 0) {
            break;
        }
        if (result) {
            strncat(result, pt, lable_size);
            strcat(result, ".");
			printf("label: %s\n", result);
        }
        pt += lable_size;
        size -= lable_size;
    }
    int len = strlen(result);
    result[len - 1] = '\0';
	printf("lable result: %s, pt %p\n", result, result);
    // test QTYPE and QCLASS
    return len;
}

// translate address to format in dns query
int translate_name(const char *addr, char *result) {
    int total = 0, label_len = 0;
    while (addr[total + label_len] != '\0') {
        char curr_char = addr[total + label_len];
        if (curr_char != '.') {
            result[total + label_len + 1] = curr_char;
        } else {
            result[total] = (char) label_len;
            total += label_len + 1;
            label_len = -1;
        }
        label_len++;
    }
    result[total] = (char) label_len;
    total += label_len;
    //result[total + 1] = '\0';
    //return total;
    result[total + 1] = 0;
	printf("generate resource: %s, %p\n", result, result);
    return total + 2;
}

// parse a single question item
int parse_question(const char *buf, int size, char *result) {
	puts("parse question");
    int offset = 0;
    offset += parse_name(buf + offset, size - offset, result);
    dns_question_t *question = buf + offset;
    // check QTYPE and QCLASS
    if (DNS_GET_QCLASS(question) != DNS_CLASS_IP) {
		printf("error qclass: %d\n", DNS_GET_QCLASS(question));
        //return -1; // TODO
    }
    if (DNS_GET_QTYPE(question) != DNS_TYPE_A) {
		printf("error qtype: %d\n", DNS_GET_QTYPE(question));
        //return -1; // TODO
    }
    // return zero
    return offset + sizeof(dns_question_t);
}

// parse a single answer item
int parse_resource(const char *buf, int size, struct in_addr *result) {
    int offset = 0;
    // ignore question name
    offset += parse_name(buf + offset, size - offset, NULL);
    dns_resource_t *resource = buf + offset;
    // check QTYPE and QCLASS
    if (DNS_GET_TYPE(resource) != DNS_TYPE_A) {
        return -1;
    }
    if (DNS_GET_CLASS(resource) != DNS_CLASS_IP) {
        return -1;
    }
    int rdlen = DNS_GET_RDLEN(resource);
    if (rdlen != sizeof(struct in_addr)) {
        return -1;
    }
    offset += sizeof(dns_resource_t);
    result->s_addr = ntohs(buf + offset);
    return offset;
}

int generate_question(const char *query, char *result) {
    int ret = translate_name(query, result);
	printf("translate len: %d\n", ret);
    if (ret < 0) {
        return -1;
    }
    dns_question_t question;
    question.qclass = htons(DNS_CLASS_IP);
    question.qtype = htons(DNS_TYPE_A);
    //ret++;
    memcpy(result + ret, &question, sizeof(dns_question_t));
    ret += sizeof(dns_question_t);
    return ret;
}

int generate_resource(const char *query, const char *ip, char *result) {
    int ret = translate_name(query, result), offset = 0;
    if (ret < 0) {
        return -1;
    }
	offset += ret;
	printf("offset: %d\n", offset);
    dns_resource_t resource;
    resource.type = htons(DNS_TYPE_A);
    resource.class_name = htons(DNS_CLASS_IP);
    resource.ttl = htons(0);
    resource.rdlength = htons(sizeof(struct in_addr));
    memcpy(result + offset, &resource, sizeof(dns_resource_t));
    offset += sizeof(dns_resource_t);
	printf("offset: %d\n", offset);
    struct in_addr ip_addr_temp;
    inet_aton(ip, &ip_addr_temp);
    memcpy(result + offset - 2, &ip_addr_temp.s_addr, 4);
    offset += sizeof(struct in_addr);
	printf("offset: %d\n", offset);
    return offset;
}

int dns_isinvalid(dns_header_t *header, int is_request) {
    char *pt = header;
    if (is_request) {
        if (DNS_CHECK_FLAG(header, DNS_FLAG_QR) != 0) {
			puts("QR error");
            return -1;
        }
        if (DNS_CHECK_FLAG(header, DNS_FLAG_AA) != 0) {
			puts("AA error");
            //return -1;
        }
        // make sure that no answer and only one question
        if (DNS_GET_QDCOUNT(header) != 1) {
			puts("QD error");
			printf("QD: %d\n", DNS_GET_QDCOUNT(header));
            return -1;
        }
        if (DNS_GET_ANCOUNT(header) < 0) {
			puts("AN error");
            return -1;
        }
    }
    else {
        if (!DNS_CHECK_FLAG(header, DNS_FLAG_QR)) {
            return -1;
        }
        if (!DNS_CHECK_FLAG(header, DNS_FLAG_AA)) {
            return -1;
        }
        // make sure that no question and only one answer
        if (DNS_GET_QDCOUNT(header) < 0) {
            return -1;
        }
        if (DNS_GET_ANCOUNT(header) != 1) {
            return -1;
        }
    }
    // check RD, RA, Z
    if (DNS_CHECK_FLAG(header, DNS_FLAG_RD)) {
		puts("flags error 1");
        return -1;
    }
    if (DNS_CHECK_FLAG(header, DNS_FLAG_RA)) {
		puts("flags error 2");
        return -1;
    }
    if (DNS_CHECK_FLAG(header, DNS_FLAG_Z)) {
		puts("flags error 3");
        // return -1; // TODO: fix this
    }
    if (header->nscount != 0) {
		puts("nscount error");
        return -1;
    }
    if (header->arcount != 0) {
		puts("arcount error");
        return -1;
    }
	puts("the request is valid");
	puts("???!!!???!!!");
    return 0;
	puts("#### not seen");
}

// server part
int dns_parse_request(const char *buf, int size, char *result) {
    // parse header
    dns_header_t *header = buf;
	printf("header pt: %p\n", header);
    // check whether response, if not then return negative
    if (dns_isinvalid(header, 1)) {
		puts("DNS req invalid");
        return -1;
    }
    int offset = 0 + sizeof(dns_header_t);
    // get number of questions, should be 1
    if (DNS_GET_QDCOUNT(header) != 1) {
		printf("qd error\n");
        return -1;
    }
    // parse the question
    //int ret = parse_question(buf + offset, size - offset, result + offset);
    int ret = parse_question(buf + offset, size - offset, result);
	printf("parse request result: %s, pt %p\n", result, result);
	return ret;
}

int dns_gen_response(const char *query, const char *ip_addr, uint16_t dns_id, int rcode, char *result) {
	puts("dns generate response");
	printf("result ip: %s\n", ip_addr);
    int offset = 0;
    // init header
    dns_header_t *header = result;
    dns_header_init(header, dns_id);
    uint16_t flags = DNS_FLAG_QR | DNS_FLAG_AA;
	printf("%04x\n", flags);
	if (rcode != 0) {
		puts("rcode != 0");
		DNS_SET_RCODE(header);
		DNS_SET_FLAG(header, flags);
		return sizeof(dns_header_t);
	}
	puts("set flags");
    DNS_SET_FLAG(header, flags);
    DNS_SET_QDCOUNT(header, 1);
    DNS_SET_ANCOUNT(header, 1);
    offset += sizeof(dns_header_t);
    // generate question
    int ret = generate_question(query, result + offset);
    if (ret < 0) {
        return -1;
    }
    offset += ret;
    ret = generate_resource(query, ip_addr, result + offset);
    if (ret < 0) {
        return -1;
    }
	printf("gen_response ret: %d\n", ret + offset);
    return ret + offset;
}

// client part
// parse response
int dns_parse_response(const char *response, int size, struct in_addr *result) {
    // parse header
    dns_header_t *header = response;
    // check whether response, if not then return negative
    if (dns_isinvalid(header, 0)) {
        return -1;
    }
    if (DNS_CHECK_FLAG(header, DNS_FLAG_RCODE) != 0) {
        // server returned error status
        return -2;
    }
    // get number of questions
    int qnum = DNS_GET_QDCOUNT(header), offset = 0 + sizeof(dns_header_t), ret = 0;
    if (DNS_GET_ANCOUNT(header) != 1) {
        return -1;
    }
    while (qnum > 0) {
        ret = parse_question(response + offset, size - offset, NULL);
        if (ret < 0) {
            break;
        }
        size -= ret;
        qnum--;
    }
    if (ret < 0) {
        return -1;
    }
    // get the ip address in question section
    ret = parse_name(response + offset, size - offset, NULL);
    if (ret < 0) {
        return -1;
    }
    offset += ret;
    // then check the answer
    ret = parse_resource(response + offset, size - offset, result);
    return ret;
}

// generate request
int dns_generate_request(const char *query, int dns_id, char *result) {
    int offset = 0;
    // init header
    dns_header_t *header = result;
    dns_header_init(header, dns_id);
//    uint16_t flags = ;
//    DNS_SET_FLAG(header, flags);
    DNS_SET_QDCOUNT(header, 1);
    DNS_SET_ANCOUNT(header, 0);
    offset += sizeof(dns_header_t);
    // generate question
    int ret = generate_question(query, result + offset);
    if (ret < 0) {
        return -1;
    }
    return ret + sizeof(dns_header_t);
}
