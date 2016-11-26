#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>

#include "../../bitrate.h"

// extern map_t receiver_connection_map;
extern struct receiver_connection* receiver_connections;
extern struct sender_connection* sender_connections;

static void test_select_bitrate(void **state) {
  bitrate* head = malloc(sizeof(bitrate) * 6);
  int i = 0;
  for (; i < 5; i++) {
    head[i].next = &head[i + 1];
  }
  head[5].next = NULL;

  head[0].bitrate = 100;
  head[1].bitrate = 200;
  head[2].bitrate = 400;
  head[3].bitrate = 800;
  head[4].bitrate = 1200;
  head[5].bitrate = 1600;
  assert_int_equal(select_bitrate(head, 99.99999 * 1.5), -1);
  assert_int_equal(select_bitrate(head, 100 * 1.5), 100);
  assert_int_equal(select_bitrate(head, 100.001 * 1.5), 100);
  assert_int_equal(select_bitrate(head, 399.99 * 1.5), 200);
  assert_int_equal(select_bitrate(head, 10000000 * 1.5), 1600);
}

static void test_parse_xml(void **state) {
  char xml[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<manifest xmlns=\"http://ns.adobe.com/f4m/1.0\">\n"
      "</manifest>";
  bitrate* head = parse_xml_to_list(xml);
  assert_null(head);

  char xml2[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<manifest xmlns=\"http://ns.adobe.com/f4m/1.0\">\n"
      "<media url=\"/myvideo/high\" bitrate=\"1708\" width=\"1920\" height=\"1080\"/>\n"
      "</manifest>";
  head = parse_xml_to_list(xml2);
  assert_int_equal(head->bitrate, 1708);
  assert_null(head->next);

  char xml3[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<manifest xmlns=\"http://ns.adobe.com/f4m/1.0\">\n"
      "<media url=\"/myvideo/high\" bitrate=\"200\" width=\"1920\" height=\"1080\"/>\n"
      "<media url=\"/myvideo/high\" bitrate=\"400\" width=\"20\" height=\"1080\"/>\n"
      "<media url=\"/myvideo/high\" bitrate=\"100\" width=\"19\" height=\"1080\"/>\n"
      "<media url=\"/myvideo/high\" bitrate=\"800\" width=\"19\" height=\"1080\"/>\n"
      "</manifest>";
  head = parse_xml_to_list(xml3);
  int i = 100;
  for (; i <= 800; i*=2) {
    assert_int_equal(head->bitrate, i);
    head = head->next;
  }
}

int main(void) {
  const UnitTest tests[] = {
      unit_test(test_select_bitrate),
      unit_test(test_parse_xml),
  };
  return run_tests(tests);
}
