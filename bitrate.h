//#include <libxml/parser.h>
//#include <libxml/xmlIO.h>
//#include <libxml/xinclude.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _bitrate {
  int bitrate;
  struct _bitrate* next;
} bitrate;

int select_bitrate(bitrate* head, float tpt);
bitrate* parse_xml_to_list(char* buf);
bitrate* dup_bitrate_list(bitrate *list);
