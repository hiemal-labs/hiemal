#include "test_common.h"
#include "intern/common.h"

#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

TEST(next_arg) {
  bool opt = false;
  int arglen = 0;
  char *test_str = "(a;a_order;)b;b_order;";
  char *str_itr = test_str;
  ASSERT_TRUE((str_itr = next_arg(test_str, ';', NULL, &arglen, &opt)) == (test_str + 1));
  ASSERT_TRUE(arglen == -1);
  ASSERT_TRUE(opt == true);
  
  ASSERT_TRUE((str_itr = next_arg(test_str + 1, ';', NULL, &arglen, &opt)) == (test_str + 3));
  ASSERT_TRUE(arglen == 1);
  ASSERT_TRUE(opt == true);
  
  ASSERT_TRUE((str_itr = next_arg(test_str + 3, ';', NULL, &arglen, &opt)) == (test_str + 12));
  ASSERT_TRUE(arglen == 7);
  ASSERT_TRUE(opt == false);
  
  ASSERT_TRUE((str_itr = next_arg(test_str + 12, ';', NULL, &arglen, &opt)) == (test_str + 14));
  ASSERT_TRUE(arglen == 1);
  ASSERT_TRUE(opt == false);
  
  ASSERT_TRUE((str_itr = next_arg(test_str + 14, ';', NULL, &arglen, &opt)) == NULL);
  ASSERT_TRUE(arglen == 7);
  ASSERT_TRUE(opt == false);
  
  return TEST_SUCCESS;
}

TEST(kwargs_unpack) {
  kwargs_t kwargs;
  double_t _b[] = {1.2, 3.2, -1.4};
  unsigned long int _b_order = 3;
  kwargs.argc = 2;
  char *arg_names[] = {"b", "b_order"};
  void *args[] = {(void*)_b, (void*)_b_order};
  kwargs.arg_names = arg_names;
  kwargs.args = args;

  double_t *a = NULL;
  double_t *b = NULL;
  unsigned long int a_order = 0;
  unsigned long int b_order = 0;
  int rc = kwargs_unpack(&kwargs, "(a;a_order;)b;b_order;", &a, &a_order, &b, &b_order);
  ASSERT_TRUE(rc == 0);
  ASSERT_TRUE(a == NULL);
  ASSERT_TRUE(a_order == 0);
  ASSERT_TRUE(b == _b);
  ASSERT_TRUE(b_order == 3);
  return TEST_SUCCESS;
}

int _node_even(hm_list_node_t *node, void *userdata) {
  struct test_node
  {
    HM_LIST_NODE_HEAD(struct test_node)
    int dat;
  };
  struct test_node *_node = (struct test_node*)node;
  return (_node->dat % 2 == 0) ? 0 : 1;
}

TEST(hm_list) {

  struct test_node
  {
    HM_LIST_NODE_HEAD(struct test_node)
    int dat;
  };

  struct test_list {
    HM_LIST_HEAD(struct test_node)
  };
  
  struct test_node *a, *b, *c, *d, *e;
  struct test_list *l;

  a = (struct test_node*)malloc(sizeof(struct test_node));
  b = (struct test_node*)malloc(sizeof(struct test_node));
  c = (struct test_node*)malloc(sizeof(struct test_node));
  d = (struct test_node*)malloc(sizeof(struct test_node));
  e = (struct test_node*)malloc(sizeof(struct test_node));
  l = (struct test_list*)malloc(sizeof(struct test_list));

  HM_LIST_NODE_INIT(a);
  HM_LIST_NODE_INIT(b);
  HM_LIST_NODE_INIT(c);
  HM_LIST_NODE_INIT(d);
  HM_LIST_NODE_INIT(e);
  HM_LIST_INIT(l);

  a->dat = 3;
  b->dat = 4;
  c->dat = 5;
  d->dat = 6;
  e->dat = 7;

  hm_list_append(l, a);
  hm_list_append(l, b);
  hm_list_insert(l, c, 1);
  hm_list_insert(l, d, 0);
  hm_list_insert(l, e, 0);
  hm_list_remove(l, 0, NULL);
  hm_list_remove(l, -1, NULL);
  hm_list_remove_where(l, _node_even, NULL, NULL);
  

  ASSERT_TRUE(l->n_items == 2);
  ASSERT_TRUE(l->head->dat == 3);
  ASSERT_TRUE(l->head->next->dat == 5);

  hm_list_delete(l);
  
  return TEST_SUCCESS;
}

TEST(hm_array) {
  struct test_array {
    HM_ARRAY_HEAD(int)
  };
  
  int buf1_raw[] = {-2,-1,0,1,2};
  int buf2_raw[] = {3,4,5,6};
  int buf_concat[] = {-2,-1,0,1,2,3,4,5,6};
  unsigned int buf1_items = sizeof(buf1_raw) / sizeof(int);
  unsigned int buf2_items = sizeof(buf2_raw) / sizeof(int);
  unsigned int buf_concat_items = sizeof(buf_concat) / sizeof(int);
  double tol = 1e-5;

  struct test_array *arr1 = (struct test_array*)malloc(sizeof(struct test_array));
  HM_ARRAY_INIT(arr1, int)
  arr1->buf = (int*)malloc(sizeof(int));
  hm_array_copy_raw(arr1, buf1_raw, buf1_items);
  struct test_array *arr2 = (struct test_array*)malloc(sizeof(struct test_array));
  HM_ARRAY_INIT(arr2, int)
  arr2->buf = (int*)malloc(sizeof(int));
  hm_array_copy_raw(arr2, buf2_raw, buf2_items);

  hm_array_concat(arr1, arr2);
  ASSERT_TRUE(arr1->n_items == buf_concat_items)
  ASSERT_TRUE(strncmp((char*)(arr1->buf), (char*)(buf_concat), buf_concat_items*sizeof(int)) == 0)

  hm_array_delete(arr1, NULL);
  hm_array_delete(arr2, NULL);

  return TEST_SUCCESS;
}

TEST(hm_list_copy_array) {
  struct test_node
  {
    HM_LIST_NODE_HEAD(struct test_node)
    int dat;
  };

  struct test_list {
    HM_LIST_HEAD(struct test_node)
  };

  struct test_array {
    HM_ARRAY_HEAD(int)
  };

  int arr_buf[] = {-2,-1,0,1,2};
  const unsigned int arr_buf_len = sizeof(arr_buf)/sizeof(int);
  struct test_array test_arr;
  struct test_list test_list1;
  struct test_list test_list2;
  struct test_node node1;

  HM_ARRAY_INIT((&test_arr), int)
  HM_LIST_INIT((&test_list1))
  HM_LIST_INIT((&test_list2))
  HM_LIST_NODE_INIT((&node1))
  
  node1.dat = -3;
  test_list2.head = &node1;
  test_list2.n_items = 1;

  hm_array_copy_raw(&test_arr, arr_buf, arr_buf_len);
  hm_list_copy_array(&test_list1, &test_arr, 0);
  hm_list_copy_array(&test_list2, &test_arr, 0);
  ASSERT_TRUE(test_list1.n_items == arr_buf_len)
  ASSERT_TRUE(test_list2.n_items == arr_buf_len + 1)
  ASSERT_TRUE(test_list2.head->dat == node1.dat);
  int i = 0;
  for (i = 0; i < arr_buf_len; i++) {
    ASSERT_TRUE(strncmp((void*)&(((struct test_node*)hm_list_at(&test_list1, i))->dat), hm_array_at(&test_arr, i), test_arr.item_size) == 0)
    ASSERT_TRUE(strncmp((void*)&(((struct test_node*)hm_list_at(&test_list2, i+1))->dat), hm_array_at(&test_arr, i), test_arr.item_size) == 0)
  }

  /*int clear_free_fn(hm_list_node_t *node) {
    struct test_node* _node = (struct test_node*)node;
    _node->free_fn = NULL;
    return 0;
  }
  hm_list_itr(&test_list2, clear_free_fn);*/
  test_list2.head->free_fn = NULL;
  hm_list_copy_array(&test_list2, &test_arr, COPY_OVERWRITE);
  ASSERT_TRUE(test_list2.n_items == sizeof(arr_buf)/sizeof(int))
  
  for (i = 0; i < arr_buf_len; i++) {
    ASSERT_TRUE(strncmp((void*)&(((struct test_node*)hm_list_at(&test_list2, i))->dat), hm_array_at(&test_arr, i), test_arr.item_size) == 0)
  }
  hm_list_clear(&test_list1);
  hm_list_clear(&test_list2);
  hm_array_clear(&test_arr);
  free(test_arr.buf);
  return TEST_SUCCESS;
}

TEST(hm_array_copy_list) {
  struct test_node
  {
    HM_LIST_NODE_HEAD(struct test_node)
    int dat;
  };

  struct test_list {
    HM_LIST_HEAD(struct test_node)
  };

  struct test_array {
    HM_ARRAY_HEAD(int)
  };
  int arr_ref[] = {-2,-1,0,1,2};
  const unsigned int arr_ref_len = sizeof(arr_ref)/sizeof(int);
  struct test_list list;
  struct test_node nodes[arr_ref_len];
  struct test_array arr1;
  struct test_array arr2;
  HM_LIST_INIT((&list))
  HM_ARRAY_INIT((&arr1), int)
  HM_ARRAY_INIT((&arr2), int)

  int i = 0;
  struct test_node *node_ptr = NULL;
  for (i = 0; i < arr_ref_len; i++) {
    node_ptr = &(nodes[i]);
    HM_LIST_NODE_INIT(node_ptr)
    node_ptr->dat = arr_ref[i];
    node_ptr->data_size = sizeof(int);
    hm_list_append(&list, node_ptr);
  }
  hm_array_copy_list(&arr1, &list, 0);
  hm_array_resize(&arr2, 1);
  arr2.buf[0] = -3;
  arr2.n_items = 1;
  hm_array_copy_list(&arr2, &list, 0);
  ASSERT_TRUE(arr1.n_items == arr_ref_len)
  ASSERT_TRUE(memcmp(arr1.buf, arr_ref, sizeof(arr_ref)) == 0)
  ASSERT_TRUE(arr2.n_items == arr_ref_len + 1)
  ASSERT_TRUE(arr2.buf[0] == -3)
  ASSERT_TRUE(memcmp(arr2.buf +1, arr_ref, sizeof(arr_ref)) == 0)
  
  hm_array_copy_list(&arr2,&list, COPY_OVERWRITE);
  ASSERT_TRUE(arr2.n_items == arr_ref_len)
  ASSERT_TRUE(memcmp(arr2.buf, arr_ref, sizeof(arr_ref)) == 0)

  hm_array_clear(&arr1);
  free(arr1.buf);
  hm_array_clear(&arr2);
  free(arr2.buf);
  return TEST_SUCCESS;
}