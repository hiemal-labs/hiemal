#ifndef HM_UTIL_TIME_H
#define HM_UTIL_TIME_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef FILETIME hm_time_ms_t;
typedef FILETIME hm_time_us_t;
#elif defined(__unix__)
#include <time.h>
#include <unistd.h>
typedef struct timespec hm_time_ms_t;
typedef struct timespec hm_time_us_t;
#endif

int sleep_ms(unsigned int msec);
hm_time_ms_t get_current_time_ms();
unsigned long elapsed_time_ms(const hm_time_ms_t *start);

hm_time_us_t get_current_time_us();
unsigned long elapsed_time_us(const hm_time_us_t *start);
#endif