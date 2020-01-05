#include "chester.h"

#include <SDL.h>

#include <signal.h>

#include <stdbool.h>
#include <stdlib.h>

bool done = false;
#ifdef CGB
bool toggle_color_correction = false;
#endif

#if defined(WIN32)
#if defined(NDEBUG)
// Hide console for Release builds
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
#else
void sig_handler(int signum __attribute__((unused)))
{
  done = true;
}
#endif

struct sdl_graphics_s {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
};

typedef struct sdl_graphics_s sdl_graphics;

bool init_graphics(gpu *g)
{
  g->app_data = malloc(sizeof(sdl_graphics));
  sdl_graphics *sdl_graphics_ptr = (sdl_graphics*)g->app_data;
  sdl_graphics_ptr->window = NULL;
  sdl_graphics_ptr->renderer = NULL;
  sdl_graphics_ptr->texture = NULL;
  g->pixel_data = NULL;

  SDL_Init(SDL_INIT_VIDEO);

  sdl_graphics_ptr->window = SDL_CreateWindow("ChesterApp",
                                              SDL_WINDOWPOS_UNDEFINED,
                                              SDL_WINDOWPOS_UNDEFINED,
                                              X_RES * WINDOW_SCALE, Y_RES * WINDOW_SCALE,
                                              SDL_WINDOW_OPENGL);

  if (sdl_graphics_ptr->window == NULL)
    {
      gb_log(ERROR, "Could not create a window: %s",
             SDL_GetError());
      return false;
    }

  sdl_graphics_ptr->renderer = SDL_CreateRenderer(sdl_graphics_ptr->window,
                                                  -1,
                                                  SDL_RENDERER_ACCELERATED);

  if (sdl_graphics_ptr->renderer == NULL)
    {
      gb_log(ERROR, "Could not create a renderer: %s",
             SDL_GetError());
      return false;
    }

  sdl_graphics_ptr->texture =
    SDL_CreateTexture(sdl_graphics_ptr->renderer,
                      SDL_GetWindowPixelFormat(sdl_graphics_ptr->window),
                      SDL_TEXTUREACCESS_STREAMING,
                      256, 256);

  if (sdl_graphics_ptr->texture == NULL)
    {
      gb_log(ERROR, "Could not create a background texture: %s",
             SDL_GetError());
      return false;
    }

  return true;
}

void uninit_graphics(gpu *g)
{
  if (g->app_data)
  {
    sdl_graphics *sdl_graphics_ptr = (sdl_graphics*)g->app_data;

    if (g->pixel_data)
      {
        SDL_UnlockTexture(sdl_graphics_ptr->texture);
        g->pixel_data = NULL;
      }

    SDL_DestroyRenderer(sdl_graphics_ptr->renderer);
    SDL_DestroyWindow(sdl_graphics_ptr->window);
    SDL_DestroyTexture(sdl_graphics_ptr->texture);

    free(sdl_graphics_ptr);
    g->app_data = NULL;
  }

  SDL_Quit();
}

bool lock_texture(gpu *g)
{
  if (g->app_data)
  {
    sdl_graphics *sdl_graphics_ptr = (sdl_graphics*)g->app_data;
    int row_length;
    if (SDL_LockTexture(sdl_graphics_ptr->texture, NULL, &g->pixel_data, &row_length))
    {
      gb_log(ERROR, "Could not lock the background texture: %s",
             SDL_GetError());
             return false;
    }
  }

  return true;
}

void render(gpu *g)
{
  if (g->app_data)
  {
    sdl_graphics *sdl_graphics_ptr = (sdl_graphics*)g->app_data;

    static SDL_Rect screen;
    screen.x = 0;
    screen.y = 0;
    screen.w = X_RES;
    screen.h = Y_RES;

    if (g->pixel_data)
      {
        SDL_UnlockTexture(sdl_graphics_ptr->texture);
        g->pixel_data = NULL;
      }

    SDL_RenderCopy(sdl_graphics_ptr->renderer,
                   sdl_graphics_ptr->texture, &screen, NULL);
    SDL_RenderPresent(sdl_graphics_ptr->renderer);
  }
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
#ifdef CGB
      case SDLK_F4:
        toggle_color_correction = true;
        break;
#endif
      default:
        break;
      }
      break;
    }
  }

  return ret;
}

uint32_t get_ticks(void)
{
  return SDL_GetTicks();
}

void delay(uint32_t ms)
{
  SDL_Delay(ms);
}

int main(int argc, char **argv)
{
  chester chester;
  int ret_val = 0;
  const char* bootloader_file = "DMG_ROM.bin";

  if (argc != 2)
    {
      gb_log(ERROR, "Usage: %s rom-file-path", argv[0]);
      return 1;
    }

  register_keys_callback(&chester, &keys_update);
  register_get_ticks_callback(&chester, &get_ticks);
  register_delay_callback(&chester, &delay);
  register_gpu_init_callback(&chester, &init_graphics);
  register_gpu_uninit_callback(&chester, &uninit_graphics);
  register_gpu_alloc_image_buffer_callback(&chester, &lock_texture);
  register_gpu_render_callback(&chester, &render);
  register_serial_callback(&chester, NULL);

  if (!init(&chester, argv[1], NULL, bootloader_file))
    {
      return 2;
    }

#ifndef WIN32
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);
#endif

  while (!done)
    {
      const int ret = run(&chester);
      if (ret)
        {
          // Only return error conditions, return 0
          // if user initiated quit
          if (ret < 0)
            {
              ret_val = ret;
            }
          break;
        }

#ifdef CGB
      if (toggle_color_correction)
        {
          toggle_color_correction = false;
          set_color_correction(&chester, !get_color_correction(&chester));
        }
#endif
    }

  uninit(&chester);

  return ret_val;
}
