#include "sync.h"

#include <SDL2/SDL.h>

#include "logger.h"

void sync_init(sync_timer *s, unsigned int ticks)
{
  s->timing_cumulative_ticks = 0;
  s->timing_ticks = ticks;
  #ifdef ENABLE_DEBUG
  s->timing_debug_ticks = 0;
  #endif
  s->framestarttime = SDL_GetTicks();
  s->waittime = 1000.0f * s->timing_ticks / 4194304;
}

void sync(sync_timer *s, const unsigned int ticks)
{
  if (s->timing_cumulative_ticks > s->timing_ticks)
    {
      int32_t delaytime = s->waittime - (SDL_GetTicks() - s->framestarttime);
      if(delaytime > 0)
        SDL_Delay((Uint32)delaytime);

#ifdef ENABLE_DEBUG
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
      s->framestarttime = SDL_GetTicks();

      s->timing_cumulative_ticks = 0;
    }
  else
    {
      s->timing_cumulative_ticks += ticks;
    }
}
