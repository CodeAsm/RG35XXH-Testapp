// Copyright 2021 Jonne Kokkonen
// Copyright (C) 2025 CodeAsm - Nico Vijlbrief
// Released under the MIT licence, https://opensource.org/licenses/MIT

/* Uncomment this line to enable debug messages or call make with `make
   CFLAGS=-DDEBUG_MSG` */
// #define DEBUG_MSG

#include <SDL.h>
#include <signal.h>

#include "SDL2_inprint.h"
#include "config.h"
//TODO #include "input.h"
//#include "gamecontrollers.h" //TODO
#include "render.h"

enum state { QUIT, WAIT_FOR_DEVICE, RUN };

enum state run = WAIT_FOR_DEVICE;
uint8_t need_display_reset = 0;

// Handles CTRL+C / SIGINT
void intHandler() { run = QUIT; }

//void close_serial_port() { disconnect(); }

char input_text[256];

int main(const int argc, char *argv[]) {


  char *preferred_device = NULL;
  if (argc == 3 && SDL_strcmp(argv[1], "--dev") == 0) {
    preferred_device = argv[2];
    SDL_Log("Using preferred device %s.\n", preferred_device);
  }

  // Initialize the config to defaults read in the params from the
  // configfile if present
  config_params_s conf = init_config();

  // TODO: take cli parameter to override default configfile location
  read_config(&conf);

  uint8_t prev_input = 0;
  uint8_t prev_note = 0;
 // uint16_t zerobyte_packets = 0; // used to detect device disconnection

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);
#ifdef SIGQUIT
  signal(SIGQUIT, intHandler);
#endif

  // initialize all SDL systems
  if (initialize_sdl(conf.init_fullscreen, conf.init_use_gpu) == -1)
    run = QUIT;

  // initial scan for (existing) game controllers
  //gamecontrollers_initialize(); TODO


#ifdef DEBUG_MSG
  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
#endif

  // main loop begin
  do {
      SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Device not detected on begin loop.");
      if (conf.wait_for_device == 1) {
        run = WAIT_FOR_DEVICE;
      } else {
        run = QUIT;
      }
    
    // wait until device is connected
    if (conf.wait_for_device == 1) {
      //static uint32_t ticks_poll_device = 0;
      static uint32_t ticks_update_screen = 0;

        screensaver_init();
    

      while (run == WAIT_FOR_DEVICE) {
        // get current input
        

        if (SDL_GetTicks() - ticks_update_screen > 16) {
          ticks_update_screen = SDL_GetTicks();
          screensaver_draw();
          handle_input(); //TODO
          render_screen();
        }


        SDL_Delay(conf.idle_ms);
      }
    } else {
      // classic startup behaviour, exit if device is not found
      
     
       //TODO gamecontrollers_close();
        close_renderer();
        kill_inline_font();
        SDL_Quit();
        return -1;
    }
    

    // main loop
    /*
    while (run == RUN) {
        //TODO
      handle_input();
      render_screen();
      SDL_Delay(conf.idle_ms);
    }*/
  } while (run > QUIT);
  // main loop end

  // exit, clean up
  SDL_Log("Shutting down\n");\
  close_renderer();
//TODO  gamecontrollers_close();
//  close_serial_port();
  SDL_Quit();
  return 0;
}
