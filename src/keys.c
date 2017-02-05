#include "keys.h"

#include <SDL2/SDL.h>

#include <string.h>
#include <stdint.h>

void keys_reset(keys *k)
{
  memset(k, 0, sizeof(keys));
}

int keys_update(keys *k)
{
  SDL_Event event;
  int ret = 0;

  while(SDL_PollEvent(&event)) {
    switch(event.type) {
    case SDL_QUIT:
      ret = -1;
      break;
    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
      case SDLK_UP:
        k->up = true;
        ret = 1;
        break;
      case SDLK_DOWN:
        k->down = true;
        ret = 1;
        break;
      case SDLK_LEFT:
        k->left = true;
        ret = 1;
        break;
      case SDLK_RIGHT:
        k->right = true;
        ret = 1;
        break;
      case SDLK_a:
        k->a = true;
        ret = 1;
        break;
      case SDLK_z:
        k->b = true;
        ret = 1;
        break;
      case SDLK_n:
        k->start = true;
        ret = 1;
        break;
      case SDLK_m:
        k->select = true;
        ret = 1;
        break;
      default:
        break;
      }
      break;
    case SDL_KEYUP:
      switch(event.key.keysym.sym) {
      case SDLK_UP:
        k->up = false;
        break;
      case SDLK_DOWN:
        k->down = false;
        break;
      case SDLK_LEFT:
        k->left = false;
        break;
      case SDLK_RIGHT:
        k->right = false;
        break;
      case SDLK_a:
        k->a = false;
        break;
      case SDLK_z:
        k->b = false;
        break;
      case SDLK_n:
        k->start = false;
        break;
      case SDLK_m:
        k->select = false;
        break;
      default:
        break;
      }
      break;
    }
  }

  return ret;
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
