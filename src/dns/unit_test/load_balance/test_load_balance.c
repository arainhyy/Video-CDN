#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <netinet/in.h>

#include "../../parse.h"
#include "../../load_balance.h"

extern node* clients;
extern int client_num;
extern node* servers;
extern int server_num;
extern node* nodes;
extern int total_num;

extern routing_table_entry* routing_table;

static void test_queue(void **state) {
  queue_node header;
  memset(&header, sizeof(queue_node), 0);
  queue_node tail;
  memset(&tail, sizeof(queue_node), 0);
  header.next = &tail;
  tail.next = NULL;
  header.prev = NULL;
  tail.prev = &header;

  queue_node one;
  node _one;
  strcpy(_one.ip, "first");
  one.n = &_one;
  push_back(&tail, &one);
  assert_ptr_equal(header.next, &one);
  assert_ptr_equal(one.next, &tail);
  assert_string_equal(header.next->n->ip, "first");

  queue_node two;
  node _two;
  strcpy(_two.ip, "second");
  two.n = &_two;
  push_back(&tail, &two);
  assert_ptr_equal(header.next, &one);
  assert_ptr_equal(header.next->next, &two);
  assert_ptr_equal(two.next, &tail);
  assert_string_equal(header.next->n->ip, "first");
  assert_string_equal(header.next->next->n->ip, "second");
  assert_string_equal(tail.prev->prev->n->ip, "first");
  assert_string_equal(tail.prev->n->ip, "second");
}

static void test_build_routing_table(void **state) {
  init();
  char file_path[1024] = "../../../bitrate-project/topos/topo1/topo1.lsa";
  char file_path2[1024] = "../../../bitrate-project/topos/topo1/topo1.servers";
  parse_server_file(file_path2);
  parse_LSA_file(file_path);
  assert_int_equal(server_num, 2);
  assert_int_equal(total_num, 7);
  assert_int_equal(nodes->seq_num, 9);
  // Test build routing table.
  int ret = build_routing_table();
  assert_int_equal(ret, 0);
  int i = 0;
  for (; i < client_num; i++) {
    printf("--client: %s  server: %s\n", routing_table[i].client_ip, routing_table[i].server_ip);
  }
}

int main(void) {
  const UnitTest tests[] = {
      unit_test(test_queue),
      unit_test(test_build_routing_table),
  };
  return run_tests(tests);
}
