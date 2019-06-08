#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>

typedef uint32_t (*get_ticks_cb)(void);
typedef void (*delay_cb)(uint32_t);

typedef struct sync_timer_s
{
  int timing_cumulative_ticks;
  int timing_ticks;
  unsigned int framestarttime;
  unsigned int waittime;
#ifndef NDEBUG
  unsigned int timing_debug_ticks;
#endif
} sync_timer;

void sync_init(sync_timer *s, unsigned int ticks, get_ticks_cb cb);

void sync_time(sync_timer *s, const unsigned int ticks, get_ticks_cb t_cb, delay_cb d_cb);

#endif // SYNC_H
