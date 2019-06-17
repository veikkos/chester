#include "rtc.h"

#include "logger.h"

void rtc_update(registers *reg, memory *mem)
{
  if (mem->rtc.run)
    {
      static const unsigned int ticks_per_second = 4194304;
      mem->rtc.tick += reg->clock.last.t << reg->speed_shifter;

      if (mem->rtc.tick >= ticks_per_second)
        {
          mem->rtc.tick -= ticks_per_second;
          mem->rtc.relative++;

          gb_log(DEBUG, "RTC tick");
        }
    }
}
