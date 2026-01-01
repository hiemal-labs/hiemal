#include "util/time.h"

int sleep_ms(unsigned int msec) {
#ifdef WIN32
return Sleep(msec);
#elif defined(__unix__)
return usleep(1000*msec);
#endif
}

#ifdef __unix__
clock_t get_suitable_clock() {
#ifdef __linux__
  struct timespec t;
  if ((clock_getres(CLOCK_REALTIME_COARSE, &t) == 0) &&
      (t.tv_nsec < 5e6)) { // make sure resolution is not too coarse (<5ms)
    return CLOCK_MONOTONIC_COARSE;
  }
  else {
    return CLOCK_MONOTONIC;
  }
#else
  return CLOCK_MONOTONIC;
#endif

}
#endif

hm_time_ms_t get_current_time_ms() {
  hm_time_ms_t t;
#ifdef WIN32
  GetSystemTimeAsFileTime(&t);
#elif defined(__unix__)
  clock_t clock = get_suitable_clock();
  clock_gettime(clock, &t);
#endif
  return t;
}

unsigned long elapsed_time_ms(const hm_time_ms_t *start) {
  unsigned long time_elapsed = 0;
  hm_time_ms_t current_time = get_current_time_ms();
#ifdef WIN32
  unsigned long start_time_l = ((start->dwHighDateTime) << 32) + start->dwLowDateTime;
  unsigned long end_time_l = ((current_time.dwHighDateTime) << 32) + current_time.dwLowDateTime;
  // Convert WIN32 100-ns resolution to ms
  time_elapsed = (end_time_l - start_time_l) / 10000;
#elif defined(__unix__)
  if (current_time.tv_nsec < start->tv_nsec) {
    current_time.tv_sec -= 1;
    current_time.tv_nsec += 1e9;
  }
  time_elapsed += ((current_time.tv_nsec - start->tv_nsec )/ 1e6);
  time_elapsed += ((current_time.tv_sec - start->tv_sec )/ 1e-3);
#endif
  return time_elapsed;
}

hm_time_us_t get_current_time_us() {
  hm_time_us_t t;
#ifdef WIN32
  GetSystemTimeAsFileTime(&t);
#elif defined(__unix__)
  clock_t clock = CLOCK_MONOTONIC;
  clock_gettime(clock, &t);
#endif
  return t;
}

unsigned long elapsed_time_us(const hm_time_us_t *start) {
  unsigned long time_elapsed = 0;
  hm_time_us_t current_time = get_current_time_us();
#ifdef WIN32
  unsigned long start_time_l = ((start->dwHighDateTime) << 32) + start->dwLowDateTime;
  unsigned long end_time_l = ((current_time.dwHighDateTime) << 32) + current_time.dwLowDateTime;
  // Convert WIN32 100-ns resolution to us
  time_elapsed = (end_time_l - start_time_l) / 10;
#elif defined(__unix__)
  if (current_time.tv_nsec < start->tv_nsec) {
    current_time.tv_sec -= 1;
    current_time.tv_nsec += 1e9;
  }
  time_elapsed += ((current_time.tv_nsec - start->tv_nsec )/ 1e3);
  time_elapsed += ((current_time.tv_sec - start->tv_sec )/ 1e-6);
#endif
  return time_elapsed;
}