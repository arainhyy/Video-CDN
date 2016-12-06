#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>

#include "../../parse.h"

extern node* clients;
extern int client_num;
extern node* servers;
extern int server_num;
extern node* nodes;
extern int total_num;

static void test_parse_LSA_file(void **state) {
  init();
  char file_path[1024] = "../../../bitrate-project/topos/topo1/topo1.lsa";
  char file_path2[1024] = "../../../bitrate-project/topos/topo1/topo1.servers";
  parse_server_file(file_path2);
  int ret = parse_LSA_file(file_path);
  assert_int_equal(ret, 0);
  assert_int_equal(server_num, 2);
  assert_int_equal(total_num, 7);
  assert_int_equal(nodes->seq_num, 9);
  int i = 0;
  node* now = nodes;
  for (; i < 6; i++) {
    printf("node_id: %s\nneighbors:\n", now->ip);
    int k = 0;
    for (; k < now->neighbor_num; k++) {
      printf("%s ", now->neighbors[k]->ip);
    }
    printf("\n\n");
    now = now->next;
  }
}

static void test_parse_server_file(void **state) {
  init();
  char file_path[1024] = "../../../bitrate-project/topos/topo1/topo1.servers";
  int ret = parse_server_file(file_path);
  assert_int_equal(ret, 0);
  assert_int_equal(server_num, 2);
  assert_int_equal(total_num, 2);
  assert_string_equal(nodes->ip, "4.0.0.1");
  assert_string_equal(nodes->next->ip, "3.0.0.1");
  assert_string_equal(servers->ip, "4.0.0.1");
  assert_string_equal(servers->next->ip, "3.0.0.1");
}

int main(void) {
  const UnitTest tests[] = {
      unit_test(test_parse_server_file),
      unit_test(test_parse_LSA_file),
  };
  return run_tests(tests);
}
