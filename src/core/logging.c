#include <stdio.h>
#include <string.h>

#include "intern/logging.h"

unsigned int log_level = LOG_LVL_WARN;



unsigned int hm_set_loglvl(unsigned int lvl) {
  log_level = lvl;
  return log_level;
}

unsigned int hm_get_loglvl() { 
  return log_level;
}

int hm_log_msg(unsigned int lvl, const char *msg, const char *fn_name) {
  if (lvl > log_level) return 0;
  if (lvl == LOG_LVL_ERR) fprintf(stderr, "[ERROR] (%s): ", fn_name);
  else if (lvl == LOG_LVL_WARN) fprintf(stderr, "[WARNING] (%s): ", fn_name);
  else if (lvl == LOG_LVL_INFO) fprintf(stderr, "[INFO] (%s): ", fn_name);
  else if (lvl == LOG_LVL_DEBUG)fprintf(stderr, "[DEBUG] (%s): ", fn_name);
  else if (lvl == LOG_LVL_VERBOSE) fprintf(stderr, "[VERBOSE] (%s): ", fn_name);
  else return -1;
  fprintf(stderr, "%s\n", msg);
  return 0;
}

int hm_log_err(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_ERR, msg, fn_name);
}

int hm_log_warn(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_WARN, msg, fn_name);
}

int hm_log_info(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_INFO, msg, fn_name);
}

int hm_log_debug(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_DEBUG, msg, fn_name);
}

int hm_log_verbose(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_VERBOSE, msg, fn_name);
}


