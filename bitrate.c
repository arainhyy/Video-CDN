#include "bitrate.h"

void sorted_insert_bitrate(int bitrate_num, bitrate** head);
/**
 * Pick a bitrate less than throughput / 1.5.
 *
 * @param head Head of bitrate list.
 * @param tpt Thoughput.
 * @return Bitrate to pick.
 */
int select_bitrate(bitrate* head, float tpt) {
  bitrate* pt = head;
  bitrate* pre = NULL;
  float upbound = tpt / 1.5;
  while (pt != NULL) {
    if (pt->bitrate <= upbound) {
      pre = pt;
      pt = pt->next;
    } else break;
  }
  if (pre == NULL) return -1;
  return pre->bitrate;
}

/**
 * Parse bitrate list from xml buffer.
 *
 * @param buf Buffer contains xml of bitrate information.
 * @return Bitrate list.
 */
bitrate* parse_xml_to_list(char* buf) {
  char* pt = NULL;
  bitrate* head = NULL;
  if ((pt = strstr(buf, "<?xml")) == NULL) {
    printf("Error: illegal xml\n");
    return head;
  }
  while ((pt = strstr(pt, "<media")) != NULL) {
    pt++;
    char* bitrate_pt = NULL;
    if ((bitrate_pt = strstr(pt, "bitrate=")) != NULL) {
      int bitrate;
      if (sscanf(bitrate_pt, "bitrate=\"%d\"", &bitrate) < 1) {
        continue;
      }
      sorted_insert_bitrate(bitrate, &head);
    }
  }
  return head;
}

void sorted_insert_bitrate(int bitrate_num, bitrate** head) {
  bitrate* new_node = (bitrate*) malloc(sizeof(bitrate));
  new_node->bitrate = bitrate_num;
  new_node->next = NULL;
  if (*head == NULL) {
    *head = new_node;
    return;
  }
  bitrate* pt = *head;
  bitrate* pre = NULL;
  while (pt != NULL && pt->bitrate < bitrate_num) {
    pre = pt;
    pt = pt->next;
  }
  if (pre == NULL) {
    new_node->next = *head;
    *head = new_node;
  } else {
    new_node->next = pt;
    pre->next = new_node;
  }
}