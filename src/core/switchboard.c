#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern/switchboard.h"


int node_name_is_free(switchboard_node_list_t *list, char *name) {
  switchboard_node_t *node_itr = list->head;
  while (node_itr != NULL) {
    if (strcmp(node_itr->name, name) == 0) {
      return 1;
    }
  }
  return 0;
}

int switchboard_init(switchboard_t **s) {
  *s = (switchboard_t*)malloc(sizeof(switchboard_t));
  (*s)->nodes = (switchboard_node_list_t*)malloc(sizeof(switchboard_node_t));
  (*s)->connections = (switchboard_connection_list_t*)malloc(sizeof(switchboard_connection_list_t));
  (*s)->context_list = (switchboard_context_list_t*)malloc(sizeof(switchboard_context_list_t));
  HM_LIST_INIT((*s)->nodes)
  HM_LIST_INIT((*s)->connections)
  HM_LIST_INIT((*s)->context_list)
  return 0;
}

int switchboard_delete(switchboard_t **s) {
  hm_list_delete((*s)->nodes, NULL);
  hm_list_delete((*s)->connections, NULL);
  hm_list_delete((*s)->context_list, NULL);
  free(*s);
  *s = NULL;
  return 0;
}

int switchboard_add_node(switchboard_t *s, node_type_t node_type, char *node_name, node_resource node_res) {
  switchboard_node_t *new_node = malloc(sizeof(switchboard_node_t));
  new_node->type = node_type;
  new_node->res = node_res;
  new_node->name = strdup(node_name);
  hm_list_append(s->nodes, new_node);
  return 0;
}

int switchboard_add_buffer(switchboard_t *s, buffer_t *buf, char *name) {
  node_resource node_res;
  node_res.buf = buf;
  return switchboard_add_node(s, BUFFER, name, node_res);
}

int switchboard_add_async_loop(switchboard_t *s, async_handle_t *async_handle, char *name) {
  node_resource node_res;
  node_res.async_handle = async_handle;
  return switchboard_add_node(s, ASYNC_LOOP, name, node_res);
}

switchboard_node_t* switchboard_get_node_by_id(switchboard_t *s, unsigned int node_id) {
  switchboard_node_t *node_itr = s->nodes->head;
  while (node_itr != NULL) {
    if (node_itr->node_id == node_id) {
      return node_itr;
    }
    node_itr = node_itr->next;
  }
  return NULL;
}

switchboard_connection_t* switchboard_get_connection(switchboard_node_t *node1, switchboard_node_t *node2) {
  switchboard_connection_t *node1_conn_itr = node1->connections->head;
  switchboard_node_t *node1_conn_endpoint = NULL;
  while (node1_conn_itr != NULL) {
    node1_conn_endpoint = (node1_conn_itr->nodes[0] == node1) ? node1_conn_itr->nodes[1] : node1_conn_itr->nodes[0];
    if (node1_conn_endpoint == node2) {
      return node1_conn_itr;
    }
    node1_conn_itr = node1_conn_itr->next;
  }
  return NULL;
}

switchboard_node_t* common_async_loop(switchboard_node_t *node1, switchboard_node_t *node2) {
  switchboard_connection_t *node1_itr = node1->connections->head;
  switchboard_connection_t *node2_itr = node1->connections->head;
  switchboard_node_t *node1_connection;
  switchboard_node_t *node2_connection;
  while (node1_itr != NULL) {
    while (node2_itr != NULL) {
      node1_connection = (node1_itr->nodes[0] == node1) ? node1_itr->nodes[1] : node1_itr->nodes[0];
      node2_connection = (node2_itr->nodes[0] == node2) ? node2_itr->nodes[1] : node2_itr->nodes[0];
      if (node1_connection == node2_connection && node1_connection->type == ASYNC_LOOP) {
        return node1_connection;
      }
      node2_itr = node2_itr->next;
    }
    node1_itr = node1_itr->next;
  }
  return NULL;
}

bool ctx_same_thread(switchboard_context_t *ctx1, switchboard_context_t *ctx2) {
  return (ctx1->thread_id == ctx2->thread_id);
}

int switchboard_send_buf_buf(switchboard_t *s, buffer_t *src, buffer_t *dest, unsigned int n_bytes) {
  unsigned int n_bytes_transferred = 0;
  unsigned int src_bytes = buffer_n_read_bytes(src);
  unsigned int dest_bytes = buffer_n_write_bytes(src);
  unsigned int bytes_to_transfer = (src_bytes < dest_bytes) ? src_bytes : dest_bytes;
  bytes_to_transfer = (bytes_to_transfer < n_bytes) ? bytes_to_transfer : n_bytes;
  void *tmp_buf = malloc(n_bytes);
  bytes_to_transfer = buffer_read(src, tmp_buf, bytes_to_transfer);
  n_bytes_transferred = buffer_write(src, tmp_buf, bytes_to_transfer);
  free(tmp_buf);
  return n_bytes_transferred;
}

int switchboard_send(switchboard_t *s, switchboard_context_t *src_ctx, unsigned int node_id, unsigned int n_bytes, const int flags) {
  switchboard_node_t *src_node = src_ctx->node;
  switchboard_node_t *dest_node = switchboard_get_node_by_id(s, node_id);
  switchboard_context_t *dest_ctx = dest_node->active_context;
  switchboard_connection_t *sb_connection = switchboard_get_connection(src_node, dest_node);
  if (ctx_same_thread(src_ctx, dest_ctx)) goto send;
  if (sb_connection->dir != FORWARD && HM_FLAG_SET(flags, SB_POLL)) {
    connection_poll(src_node, sb_connection);
    goto send;
  }
  else if (sb_connection->dir == BIDIRECTIONAL) {
    goto send;
  }
  else if (sb_connection->dir == BACKWARD) {
    return -1;
  }
  else {
    goto send;
  }
  send:
  activate_context(src_ctx, flags);
  activate_context(dest_ctx, flags);
  switchboard_send_buf_buf(s, src_node->res.buf, dest_node->res.buf, n_bytes);
  deactivate_context(src_ctx);
  deactivate_context(dest_ctx);
  connection_poll_stop(dest_node, sb_connection);
  return 0;
}

int switchboard_recv(switchboard_t *s, unsigned int node_id) {
  return 0;
}

int node_dump_info(hm_list_node_t *node) {
  switchboard_node_t *_node = (switchboard_node_t*)node;
  printf("    Node %s: <%p>\n", _node->name, _node);
  return 0;
}

int connection_dump_info(hm_list_node_t *connection) {
  switchboard_connection_t *_connection = (switchboard_connection_t*)connection;
  printf("    Connection %u: <%p\n", _connection->connection_id, _connection);
  return 0;
}

int context_dump_info(hm_list_node_t *context) {
  switchboard_context_t *_context = (switchboard_context_t*)context;
  printf("  Context %u: <%p>\n", _context->context_id, _context);
  return 0;
}

int switchboard_dump(switchboard_t *s) {
  printf("Switchboard info: <%p>\n", s);
  printf("  Node list: <%p>\n", s->nodes);
  hm_list_itr(s->nodes, node_dump_info);
  printf("  Connection list: <%p>\n", s->connections);
  hm_list_itr(s->connections, connection_dump_info);
  printf("  Context list: <%p>\n", s->context_list);
  hm_list_itr(s->context_list, context_dump_info);
  return 0;
}
