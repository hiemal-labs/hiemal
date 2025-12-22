#ifndef _INTERN_LOGGING_H
#define _INTERN_LOGGING_H

#define LOG_LVL_ERR 1
#define LOG_LVL_WARN 2
#define LOG_LVL_INFO 3
#define LOG_LVL_DEBUG 4
#define LOG_LVL_TRACE 5

extern unsigned int log_level;

unsigned int hm_set_loglvl(unsigned int lvl);
unsigned int hm_get_loglvl();

int hm_log_err(const char *msg, const char *fn_name);
int hm_log_warn(const char *msg, const char *fn_name);
int hm_log_info(const char *msg, const char *fn_name);
int hm_log_debug(const char *msg, const char *fn_name);
int hm_log_trace(const char *msg, const char *fn_name);

#endif
