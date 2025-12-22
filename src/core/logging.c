#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmake_info.h"
#include "intern/logging.h"

unsigned int log_level = LOG_LVL_WARN;

#ifdef WITH_ENVVARS
#ifdef _MSC_VER
#include <stdbool.h>
static bool _loglvl_env_checked = false;
#define strcasecmp _stricmp
#else
int _check_loglvl_env() __attribute__ ((constructor));
#endif

int _check_loglvl_env() {
  const char *hm_loglvl_env = getenv("HM_LOGLVL");
  if (hm_loglvl_env == NULL) {
    goto finish;
  }
  if ((strcasecmp(hm_loglvl_env, "error") == 0) || (strcasecmp(hm_loglvl_env, "1") == 0)) {
    hm_set_loglvl(LOG_LVL_ERR);
  }
  else if ((strcasecmp(hm_loglvl_env, "warn") == 0) || (strcasecmp(hm_loglvl_env, "2") == 0)) {
    hm_set_loglvl(LOG_LVL_WARN);
  }
  else if ((strcasecmp(hm_loglvl_env, "info") == 0) || (strcasecmp(hm_loglvl_env, "3") == 0)) {
    hm_set_loglvl(LOG_LVL_INFO);
  }
  else if ((strcasecmp(hm_loglvl_env, "debug") == 0) || (strcasecmp(hm_loglvl_env, "4") == 0)) {
    hm_set_loglvl(LOG_LVL_DEBUG);
  }
  else if ((strcasecmp(hm_loglvl_env, "trace") == 0) || (strcasecmp(hm_loglvl_env, "5") == 0)) {
    hm_set_loglvl(LOG_LVL_TRACE);
  }
  finish:
#if defined(_MSC_VER)
  _loglvl_env_checked = true;
#endif
  return 0;
}
#endif


unsigned int hm_set_loglvl(unsigned int lvl) {
#if defined(_MSC_VER) && defined(WITH_ENVVARS)
  // discard environment variable since we are manually specifying a log level
  _loglvl_env_checked =  true;
#endif
  log_level = lvl;
  return log_level;
}

unsigned int hm_get_loglvl() {
#if defined(_MSC_VER) && defined(WITH_ENVVARS)
  if (!_loglvl_env_checked) {
    _check_loglvl_env();
  }
#endif
  return log_level;
}

int hm_log_msg(unsigned int lvl, const char *msg, const char *fn_name) {
#if defined(_MSC_VER) && defined(WITH_ENVVARS)
  if (!_loglvl_env_checked) {
    _check_loglvl_env();
  }
#endif
  if (lvl > log_level) return 0;
  if (lvl == LOG_LVL_ERR) fprintf(stderr, "[ERROR] (%s): ", fn_name);
  else if (lvl == LOG_LVL_WARN) fprintf(stderr, "[WARNING] (%s): ", fn_name);
  else if (lvl == LOG_LVL_INFO) fprintf(stderr, "[INFO] (%s): ", fn_name);
  else if (lvl == LOG_LVL_DEBUG)fprintf(stderr, "[DEBUG] (%s): ", fn_name);
  else if (lvl == LOG_LVL_TRACE) fprintf(stderr, "[TRACE] (%s): ", fn_name);
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

int hm_log_trace(const char *msg, const char *fn_name) {
  return hm_log_msg(LOG_LVL_TRACE, msg, fn_name);
}


