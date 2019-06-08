#include "sync.h"

#include "logger.h"

void sync_init(sync_timer *s, unsigned int ticks, get_ticks_cb cb)
{
  s->timing_cumulative_ticks = 0;
  s->timing_ticks = ticks;
#ifndef NDEBUG
  s->timing_debug_ticks = 0;
#endif
  s->framestarttime = cb();
  s->waittime = (unsigned int)(1000.0f * s->timing_ticks / 4194304);
}

void sync_time(sync_timer *s, const unsigned int ticks, get_ticks_cb t_cb, delay_cb d_cb)
{
  if (s->timing_cumulative_ticks > s->timing_ticks)
    {
      int32_t delaytime = s->waittime - (t_cb() - s->framestarttime);
      if(delaytime > 0)
        d_cb((uint32_t)delaytime);

#ifndef NDEBUG
      if (++(s->timing_debug_ticks) > 10)
        {
          if (delaytime < 0)
            {
              gb_log (WARNING,
                      "Too slow, %d ms of game time took %d ms too long to "
                      "process",
                      s->waittime,
                      -delaytime);
            }

          s->timing_debug_ticks = 0;
        }
#endif
      s->framestarttime = t_cb();

      s->timing_cumulative_ticks = 0;
    }
  else
    {
      s->timing_cumulative_ticks += ticks;
    }
}
