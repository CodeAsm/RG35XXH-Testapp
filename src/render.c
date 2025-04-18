// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT

#include "render.h"

#include <SDL.h>
#include <stdio.h>

#include "SDL2_inprint.h"
#include "command.h"
#include "fx_cube.h"

#include "font1.h"
#include "font2.h"
#include "font3.h"
#include "font4.h"
#include "font5.h"
#include "inline_font.h"

SDL_Window *win;
SDL_Renderer *rend;
SDL_Texture *maintexture;
SDL_Color background_color = (SDL_Color){.r = 0x00, .g = 0x00, .b = 0x00, .a = 0x00};

static uint32_t ticks_fps;
static int fps;
static int font_mode = -1;
static int m8_hardware_model = 0;
static int screen_offset_y = 0;
static int text_offset_y = 0;
static int waveform_max_height = 24;

static int texture_width = 320;
static int texture_height = 240;

static char input_text[64] = "No input";
static char version_text[64] = "CodeAsm v0.1.0";

struct inline_font *fonts[5] = {&font_v1_small, &font_v1_large, &font_v2_small, &font_v2_large,
                                &font_v2_huge};

uint8_t fullscreen = 0;

static uint8_t dirty = 0;

// Initializes SDL and creates a renderer and required surfaces
int initialize_sdl(const int init_fullscreen, const int init_use_gpu) {

  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "SDL_Init: %s\n", SDL_GetError());
    return -1;
  }

// Open the first available joystick
    if (SDL_NumJoysticks() < 1) {
        printf("No joysticks connected!\n");
         snprintf(input_text, sizeof(input_text), "No joysticks connected!");
               
    } else {
        SDL_Joystick *joystick = SDL_JoystickOpen(0);
        if (joystick == NULL) {
            printf("Failed to open joystick! SDL_Error: %s\n", SDL_GetError());
             snprintf(input_text, sizeof(input_text), "Failed to open joystick! SDL_Error: %s", SDL_GetError());
        } else {
            printf("Joystick connected: %s\n", SDL_JoystickName(joystick));
            snprintf(input_text, sizeof(input_text), "Joystick connected: %s", SDL_JoystickName(joystick));
        }
    }





  // SDL documentation recommends this
  atexit(SDL_Quit);

  win = SDL_CreateWindow(
      "CodeAsm does coding", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, texture_width * 2, texture_height * 2,
      SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | init_fullscreen);

  rend =
      SDL_CreateRenderer(win, -1, init_use_gpu ? SDL_RENDERER_ACCELERATED : SDL_RENDERER_SOFTWARE);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  maintexture = NULL;

  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                  texture_width, texture_height);

  SDL_SetRenderTarget(rend, maintexture);

  SDL_SetRenderDrawColor(rend, background_color.r, background_color.g, background_color.b,
                         background_color.a);

  SDL_RenderClear(rend);

  set_font_mode(0);

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

  dirty = 1;

  return 1;
}

static void change_font(struct inline_font *font) {
  kill_inline_font();
  inrenderer(rend);
  prepare_inline_font(font);
}

static void check_and_adjust_window_and_texture_size(const unsigned int new_width,
                                                     const unsigned int new_height) {

  int h, w;

  texture_width = new_width;
  texture_height = new_height;

  // Query window size and resize if smaller than default
  SDL_GetWindowSize(win, &w, &h);
  if (w < texture_width * 2 || h < texture_height * 2) {
    SDL_SetWindowSize(win, texture_width * 2, texture_height * 2);
  }

  SDL_DestroyTexture(maintexture);

  SDL_RenderSetLogicalSize(rend, texture_width, texture_height);

  maintexture = SDL_CreateTexture(rend, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
                                  texture_width, texture_height);

  SDL_SetRenderTarget(rend, maintexture);
}

// Set M8 hardware model in use. 0 = MK1, 1 = MK2
void set_m8_model(const unsigned int model) {

  switch (model) {
  case 1:
    m8_hardware_model = 1;
    check_and_adjust_window_and_texture_size(480, 320);
    break;
  default:
    m8_hardware_model = 0;
    check_and_adjust_window_and_texture_size(320, 240);
    break;
  }
}

void set_font_mode(int mode) {
  if (mode < 0 || mode > 2) {
    // bad font mode
    return;
  }
  if (m8_hardware_model == 1) {
    mode += 2;
  }
  if (font_mode == mode)
    return;

  font_mode = mode;
  screen_offset_y = fonts[mode]->screen_offset_y;
  text_offset_y = fonts[mode]->text_offset_y;
  waveform_max_height = fonts[mode]->waveform_max_height;

  change_font(fonts[mode]);
  SDL_LogDebug(SDL_LOG_CATEGORY_RENDER, "Font mode %i, Screen offset %i", mode, screen_offset_y);
}

void close_renderer() {
  kill_inline_font();
  SDL_DestroyTexture(maintexture);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
}

void toggle_fullscreen() {

  const int fullscreen_state = SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN;

  SDL_SetWindowFullscreen(win, fullscreen_state ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_ShowCursor(fullscreen_state);

  dirty = 1;
}

int draw_character(struct draw_character_command *command) {

  const uint32_t fgcolor =
      command->foreground.r << 16 | command->foreground.g << 8 | command->foreground.b;
  const uint32_t bgcolor =
      command->background.r << 16 | command->background.g << 8 | command->background.b;

  /* Notes:
     If large font is enabled, offset the screen elements by a fixed amount.
     If background and foreground colors are the same, draw transparent
     background. Due to the font bitmaps, a different pixel offset is needed for
     both*/

  inprint(rend, (char *)&command->c, command->pos.x,
          command->pos.y + text_offset_y + screen_offset_y, fgcolor,
          bgcolor);

  dirty = 1;

  return 1;
}

void draw_rectangle(struct draw_rectangle_command *command) {

  SDL_Rect render_rect;

  render_rect.x = command->pos.x;
  render_rect.y = command->pos.y + screen_offset_y;
  render_rect.h = command->size.height;
  render_rect.w = command->size.width;

  // Background color changed
  if (render_rect.x == 0 && render_rect.y <= 0 && render_rect.w == texture_width &&
      render_rect.h >= texture_height) {
    SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "BG color change: %d %d %d", command->color.r,
                 command->color.g, command->color.b);
    background_color.r = command->color.r;
    background_color.g = command->color.g;
    background_color.b = command->color.b;
    background_color.a = 0xFF;
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "x:%i, y:%i, w:%i, h:%i", render_rect.x,
                 render_rect.y, render_rect.w, render_rect.h);

#ifdef __ANDROID__
    int bgcolor = (command->color.r << 16) | (command->color.g << 8) | command->color.b;
    SDL_AndroidSendMessage(0x8001, bgcolor);
#endif
  }

  SDL_SetRenderDrawColor(rend, command->color.r, command->color.g, command->color.b, 0xFF);
  SDL_RenderFillRect(rend, &render_rect);

  dirty = 1;
}

void draw_waveform(struct draw_oscilloscope_waveform_command *command) {

  static uint8_t wfm_cleared = 0;
  static int prev_waveform_size = 0;

  // If the waveform is not being displayed and it's already been cleared, skip
  // rendering it
  if (!(wfm_cleared && command->waveform_size == 0)) {

    SDL_Rect wf_rect;
    if (command->waveform_size > 0) {
      wf_rect.x = texture_width - command->waveform_size;
      wf_rect.y = 0;
      wf_rect.w = command->waveform_size;
      wf_rect.h = waveform_max_height + 1;
    } else {
      wf_rect.x = texture_width - prev_waveform_size;
      wf_rect.y = 0;
      wf_rect.w = prev_waveform_size;
      wf_rect.h = waveform_max_height + 1;
    }
    prev_waveform_size = command->waveform_size;

    SDL_SetRenderDrawColor(rend, background_color.r, background_color.g, background_color.b,
                           background_color.a);
    SDL_RenderFillRect(rend, &wf_rect);

    SDL_SetRenderDrawColor(rend, command->color.r, command->color.g, command->color.b, 255);

    // Create a SDL_Point array of the waveform pixels for batch drawing
    SDL_Point waveform_points[command->waveform_size];

    for (int i = 0; i < command->waveform_size; i++) {
      // Limit value because the oscilloscope commands seem to glitch
      // occasionally
      if (command->waveform[i] > waveform_max_height) {
        command->waveform[i] = waveform_max_height;
      }
      waveform_points[i].x = i + wf_rect.x;
      waveform_points[i].y = command->waveform[i];
    }

    SDL_RenderDrawPoints(rend, waveform_points, command->waveform_size);

    // The packet we just drew was an empty waveform
    if (command->waveform_size == 0) {
      wfm_cleared = 1;
    } else {
      wfm_cleared = 0;
    }

    dirty = 1;
  }
}

void display_keyjazz_overlay(const uint8_t show, const uint8_t base_octave,
                             const uint8_t velocity) {

  const Uint16 overlay_offset_x = texture_width - (fonts[font_mode]->glyph_x * 7 + 1);
  const Uint16 overlay_offset_y = texture_height - (fonts[font_mode]->glyph_y + 1);
  const Uint32 bgcolor =
      background_color.r << 16 | background_color.g << 8 | background_color.b;

  if (show) {
    char overlay_text[7];
    snprintf(overlay_text, sizeof(overlay_text), "%02X %u", velocity, base_octave);
    inprint(rend, overlay_text, overlay_offset_x, overlay_offset_y, 0xC8C8C8, bgcolor);
    inprint(rend, "*", overlay_offset_x + (fonts[font_mode]->glyph_x * 5 + 5), overlay_offset_y,
            0xFF0000, bgcolor);
  } else {
    inprint(rend, "      ", overlay_offset_x, overlay_offset_y, 0xC8C8C8, bgcolor);
  }

  dirty = 1;
}

void render_screen() {
  if (dirty) {
    dirty = 0;
    SDL_SetRenderTarget(rend, NULL);

    SDL_SetRenderDrawColor(rend, background_color.r, background_color.g, background_color.b,
                           background_color.a);

    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, maintexture, NULL, NULL);

    // Display FPS
    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", (float)fps / 5);
    inprint(rend, fps_text, 10, 10, 0xFFFFFF, 0x000000);

    // Display input text
    inprint(rend, input_text, 10, 30, 0xFFFFFF, 0x000000);
    inprint(rend, version_text, 10, 60, 0xFFFFFF, 0x000000);

    SDL_RenderPresent(rend);
    SDL_SetRenderTarget(rend, maintexture);

    fps++;

    if (SDL_GetTicks() - ticks_fps > 5000) {
      ticks_fps = SDL_GetTicks();
      SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "%.1f fps\n", (float)fps / 5);
      fps = 0;
    }
  }
}

void screensaver_init() {
  set_font_mode(1);
  fx_cube_init(rend, (SDL_Color){255, 255, 255, 255}, texture_width, texture_height,
               fonts[font_mode]->glyph_x);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Screensaver initialized");
}

void screensaver_draw() {
  fx_cube_update();
  dirty = 1;
}

void screensaver_destroy() {
  fx_cube_destroy();
  set_font_mode(0);
  SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "Screensaver destroyed");
}

void handle_input() {
   SDL_Event e;
    
    snprintf(version_text, sizeof(version_text), "ProFishinh");
               
    // Poll SDL events
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                printf("Quit event detected!\n");
                exit(0);
                break;
            case SDL_KEYDOWN:
                printf("Key %d pressed\n", e.key.keysym.sym);
                snprintf(input_text, sizeof(input_text), "Key %d pressed", e.key.keysym.sym);
                break;
            case SDL_KEYUP:
                printf("Key %d released\n", e.key.keysym.sym);
                snprintf(input_text, sizeof(input_text), "Key %d released", e.key.keysym.sym);
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                printf("Dpad down pressed\n");
                snprintf(input_text, sizeof(input_text), "Dpad down pressed");
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                printf("Dpad up pressed\n");
                snprintf(input_text, sizeof(input_text), "Dpad up pressed");
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                printf("Dpad left pressed\n");
                snprintf(input_text, sizeof(input_text), "Dpad left pressed");
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                printf("Dpad right pressed\n");
                snprintf(input_text, sizeof(input_text), "Dpad right pressed");
                break;
            
            case SDL_JOYBUTTONDOWN:
                printf("Button %d pressed\n", e.jbutton.button);
                snprintf(input_text, sizeof(input_text), "Button %d pressed", e.jbutton.button);
                break;

            case SDL_JOYBUTTONUP:
                printf("Button %d released\n", e.jbutton.button);
                snprintf(input_text, sizeof(input_text), "Button %d depressed", e.jbutton.button);
                if (e.jbutton.button == 11 || e.jbutton.button == 16) {
                    printf("We are going to quit\n");
                    snprintf(input_text, sizeof(input_text), "Quit event detected!");
                    SDL_Delay(5000); //#TODO, yeah, we never print the above.
                    exit(0);
                }
                break;
            case SDL_JOYDEVICEADDED:
                printf("Joystick device added\n");
                snprintf(input_text, sizeof(input_text), "Joystick device added");
                break;
            case SDL_JOYDEVICEREMOVED:
                printf("Joystick device removed\n");
                snprintf(input_text, sizeof(input_text), "Joystick device removed");
                break;

            case SDL_JOYAXISMOTION:
                printf("Joystick axis %d moved to %d\n", e.jaxis.axis, e.jaxis.value);
                snprintf(input_text, sizeof(input_text), "Axis %d moved to %d", e.jaxis.axis, e.jaxis.value);
                break;
            case SDL_JOYHATMOTION:
                printf("Joystick hat %d moved to %d\n", e.jhat.hat, e.jhat.value);
                snprintf(input_text, sizeof(input_text), "Hat %d moved to %d", e.jhat.hat, e.jhat.value);
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    printf("Window resized to %dx%d\n", e.window.data1, e.window.data2);
                    snprintf(input_text, sizeof(input_text), "Window resized to %dx%d", e.window.data1, e.window.data2);
                    check_and_adjust_window_and_texture_size(e.window.data1, e.window.data2);
                }
                break;
            case SDL_MOUSEBUTTONDOWN:

                printf("Mouse button %d pressed at (%d, %d)\n", e.button.button, e.button.x, e.button.y);
                snprintf(input_text, sizeof(input_text), "Mouse button %d pressed at (%d, %d)", e.button.button, e.button.x, e.button.y);
                break;
            case SDL_MOUSEBUTTONUP:
                printf("Mouse button %d released at (%d, %d)\n", e.button.button, e.button.x, e.button.y);
                snprintf(input_text, sizeof(input_text), "Mouse button %d released at (%d, %d)", e.button.button, e.button.x, e.button.y);
                break;
            case SDL_MOUSEMOTION:
                printf("Mouse moved to (%d, %d)\n", e.motion.x, e.motion.y);
                snprintf(input_text, sizeof(input_text), "Mouse moved to (%d, %d)", e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEWHEEL:
                printf("Mouse wheel scrolled %d\n", e.wheel.y);
                snprintf(input_text, sizeof(input_text), "Mouse wheel scrolled %d", e.wheel.y);
                break;
            case SDL_TEXTINPUT:
                printf("Text input: %s\n", e.text.text);
                snprintf(input_text, sizeof(input_text), "Text input: %s", e.text.text);
                break;
            case SDL_TEXTEDITING:
                printf("Text editing: %s\n", e.edit.text);
                snprintf(input_text, sizeof(input_text), "Text editing: %s", e.edit.text);
                break;
            case SDL_CONTROLLERDEVICEADDED:
                printf("Controller device added\n");
                snprintf(input_text, sizeof(input_text), "Controller device added");
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                printf("Controller device removed\n");
                snprintf(input_text, sizeof(input_text), "Controller device removed");
                break;
            case SDL_CONTROLLERDEVICEREMAPPED:
                printf("Controller device remapped\n");
                snprintf(input_text, sizeof(input_text), "Controller device remapped");
                break;
            case SDL_CONTROLLERAXISMOTION:
                printf("Controller axis %d moved to %d\n", e.caxis.axis, e.caxis.value);
                snprintf(input_text, sizeof(input_text), "Controller axis %d moved to %d", e.caxis.axis, e.caxis.value);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                printf("Controller button %d pressed\n", e.cbutton.button);
                snprintf(input_text, sizeof(input_text), "Controller button %d pressed", e.cbutton.button);
                break;
            case SDL_CONTROLLERBUTTONUP:
                printf("Controller button %d released\n", e.cbutton.button);
                snprintf(input_text, sizeof(input_text), "Controller button %d released", e.cbutton.button);
                break;
            case SDL_CONTROLLERTOUCHPADDOWN:
                printf("Controller touchpad %d pressed at (%f, %f)\n", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                snprintf(input_text, sizeof(input_text), "Controller touchpad %d pressed at (%f, %f)", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                break;
            case SDL_CONTROLLERTOUCHPADUP:
                printf("Controller touchpad %d released at (%f, %f)\n", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                snprintf(input_text, sizeof(input_text), "Controller touchpad %d released at (%f, %f)", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                break;
            case SDL_CONTROLLERTOUCHPADMOTION:
                printf("Controller touchpad %d moved to (%f, %f)\n", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                snprintf(input_text, sizeof(input_text), "Controller touchpad %d moved to (%f, %f)", e.ctouchpad.touchpad,
                       e.ctouchpad.x, e.ctouchpad.y);
                break;
            default:
                break;
        }
    }
}
