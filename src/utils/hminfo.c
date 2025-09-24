#include <stdio.h>

#include "intern/info.h"
#include "util/str.h"

#include "api/backend.h"
#include "ops.h"

int main() {
  printf("hiemal version: %s\n\n", hm_version_str());

  const char **backend_list = hm_backend_list();
  printf("backends:\n");
  while (*backend_list != NULL) {
    printf("  %s\n", *backend_list);
    backend_list++;
  }
  printf("\n");
  const char **ops_list = hm_op_list();
  printf("ops:\n");
  while (*ops_list != NULL) {
    printf("  %s\n", *ops_list);
    ops_list++;
  }
  return 0;
}