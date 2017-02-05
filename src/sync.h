#ifndef SYNC_H
#define SYNC_H

typedef struct sync_timer_s
{
  int timing_cumulative_ticks;
  int timing_ticks;
  unsigned int framestarttime;
  unsigned int waittime;
#ifdef ENABLE_DEBUG
  unsigned int timing_debug_ticks;
#endif
} sync_timer;

void sync_init(sync_timer *s, unsigned int ticks);

void sync(sync_timer *s, const unsigned int ticks);

#endif // SYNC_H
