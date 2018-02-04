#include "keys.h"

#include <string.h>
#include <stdint.h>

void keys_reset(keys *k)
{
  memset(k, 0, sizeof(keys));
}

void key_get_raw_output(keys *k, uint8_t *key_in, uint8_t *key_out)
{
  if (!(*key_in & P14))
    {
      if(k->right)
        {
          *key_out &= ~P10;
        }

      if(k->left)
        {
          *key_out &= ~P11;
        }

      if(k->up)
        {
          *key_out &= ~P12;
        }

      if(k->down)
        {
          *key_out &= ~P13;
        }
    }
  else if (!(*key_in & P15))
    {
      if(k->a)
        {
          *key_out &= ~P10;
        }

      if(k->b)
        {
          *key_out &= ~P11;
        }

      if(k->select)
        {
          *key_out &= ~P12;
        }

      if(k->start)
        {
          *key_out &= ~P13;
        }
    }
}
