/*=============================================================================
| Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    |
|                                                                             |
| This file is part of Azimuth.                                               |
|                                                                             |
| Azimuth is free software: you can redistribute it and/or modify it under    |
| the terms of the GNU General Public License as published by the Free        |
| Software Foundation, either version 3 of the License, or (at your option)   |
| any later version.                                                          |
|                                                                             |
| Azimuth is distributed in the hope that it will be useful, but WITHOUT      |
| ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       |
| FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   |
| more details.                                                               |
|                                                                             |
| You should have received a copy of the GNU General Public License along     |
| with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  |
=============================================================================*/

#include "zfxr/view.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/sound.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define BUTTON_WIDTH 100
#define BUTTON_HEIGHT 18
#define WAVE_BUTTON_WIDTH 70
#define SLIDER_LEFT 300
#define SLIDER_WIDTH 200
#define SLIDER_CENTER (SLIDER_LEFT + SLIDER_WIDTH/2)
#define SLIDER_RIGHT (SLIDER_LEFT + SLIDER_WIDTH)
#define SLIDER_HEIGHT 12

static void do_play_sound(az_zfxr_state_t *state) {
  state->request_play = true;
}

static void do_print_sound(az_zfxr_state_t *state) {
  const az_sound_spec_t *spec = &state->sound_spec;
  const char* wave_name = NULL;
  switch (spec->wave_kind) {
    case AZ_NOISE_WAVE:    wave_name = "AZ_NOISE_WAVE";    break;
    case AZ_SAWTOOTH_WAVE: wave_name = "AZ_SAWTOOTH_WAVE"; break;
    case AZ_SINE_WAVE:     wave_name = "AZ_SINE_WAVE";     break;
    case AZ_SQUARE_WAVE:   wave_name = "AZ_SQUARE_WAVE";   break;
    case AZ_TRIANGLE_WAVE: wave_name = "AZ_TRIANGLE_WAVE"; break;
    case AZ_WOBBLE_WAVE:   wave_name = "AZ_WOBBLE_WAVE";   break;
  }
  assert(wave_name != NULL);
  printf("  [AZ_SND_] = {\n");
  printf("    .wave_kind = %s,\n", wave_name);

#define PRINT_FIELD(fieldname) do { \
    if (spec->fieldname != 0.0f) { \
    printf("    .%s = %g,\n", #fieldname, (double)spec->fieldname); } \
  } while (0)

  PRINT_FIELD(env_attack);
  PRINT_FIELD(env_sustain);
  PRINT_FIELD(env_punch);
  PRINT_FIELD(env_decay);
  PRINT_FIELD(start_freq);
  PRINT_FIELD(freq_limit);
  PRINT_FIELD(freq_slide);
  PRINT_FIELD(freq_delta_slide);
  PRINT_FIELD(vibrato_depth);
  PRINT_FIELD(vibrato_speed);
  PRINT_FIELD(arp_mod);
  PRINT_FIELD(arp_speed);
  if (spec->wave_kind == AZ_SQUARE_WAVE) {
    PRINT_FIELD(square_duty);
    PRINT_FIELD(duty_sweep);
  }
  PRINT_FIELD(repeat_speed);
  PRINT_FIELD(phaser_offset);
  PRINT_FIELD(phaser_sweep);
  PRINT_FIELD(lpf_cutoff);
  PRINT_FIELD(lpf_ramp);
  PRINT_FIELD(lpf_resonance);
  PRINT_FIELD(hpf_cutoff);
  PRINT_FIELD(hpf_ramp);
  PRINT_FIELD(volume_adjust);

#undef PRINT_FIELD

  printf("  },\n");
}

/*===========================================================================*/

static const struct {
  int left, top;
  const char *name;
  void (*action)(az_zfxr_state_t*);
} buttons[] = {
  { .left = 10, .top = 10, .name = "Play sound", .action = &do_play_sound },
  { .left = 10, .top = 32, .name = "Save sound", .action = &do_print_sound },
  { .left = 10, .top = 70, .name = "Pickup/Coin",
    .action = &az_zfxr_random_pickup },
  { .left = 10, .top = 92, .name = "Laser/Shoot",
    .action = &az_zfxr_random_laser },
  { .left = 10, .top = 114, .name = "Explosion",
    .action = &az_zfxr_random_explosion },
  { .left = 10, .top = 136, .name = "Powerup",
    .action = &az_zfxr_random_powerup },
  { .left = 10, .top = 158, .name = "Hit/Hurt",
    .action = &az_zfxr_random_hurt },
  { .left = 10, .top = 180, .name = "Jump", .action = &az_zfxr_random_jump },
  { .left = 10, .top = 202, .name = "Blip/Select",
    .action = &az_zfxr_random_blip },
  { .left = 10, .top = 224, .name = "Random", .action = &az_zfxr_randomize },
};

static const struct {
  int left, top;
  const char *name;
  az_sound_wave_kind_t kind;
} wave_buttons[] = {
  { .left = 150, .top = 10, .name = "Noise", .kind = AZ_NOISE_WAVE },
  { .left = 230, .top = 10, .name = "Sawtooth", .kind = AZ_SAWTOOTH_WAVE },
  { .left = 310, .top = 10, .name = "Sine", .kind = AZ_SINE_WAVE },
  { .left = 390, .top = 10, .name = "Square", .kind = AZ_SQUARE_WAVE },
  { .left = 470, .top = 10, .name = "Triangle", .kind = AZ_TRIANGLE_WAVE },
  { .left = 550, .top = 10, .name = "Wobble", .kind = AZ_WOBBLE_WAVE },
};

typedef struct {
  int y_index;
  const char *name;
  size_t field_offset;
  bool allow_negative;
} az_zfxr_slider_t;

#define MAKE_SLIDER(y, name_string, fieldname, negative) \
  { .y_index = (y), .name = (name_string), .allow_negative = (negative), \
    .field_offset = offsetof(az_sound_spec_t, fieldname) }

static const az_zfxr_slider_t sliders[] = {
  MAKE_SLIDER(0, "Square duty", square_duty, false),
  MAKE_SLIDER(1, "Duty sweep", duty_sweep, true),
  MAKE_SLIDER(3, "Volume adjust", volume_adjust, true),
  MAKE_SLIDER(4, "Attack time", env_attack, false),
  MAKE_SLIDER(5, "Sustain time", env_sustain, false),
  MAKE_SLIDER(6, "Sustain punch", env_punch, false),
  MAKE_SLIDER(7, "Decay time", env_decay, false),
  MAKE_SLIDER(9, "Start frequency", start_freq, false),
  MAKE_SLIDER(10, "Frequency limit", freq_limit, false),
  MAKE_SLIDER(11, "Frequency slide", freq_slide, true),
  MAKE_SLIDER(12, "Delta slide", freq_delta_slide, true),
  MAKE_SLIDER(13, "Vibrato depth", vibrato_depth, false),
  MAKE_SLIDER(14, "Vibrato speed", vibrato_speed, false),
  MAKE_SLIDER(15, "Change amount", arp_mod, true),
  MAKE_SLIDER(16, "Change speed", arp_speed, false),
  MAKE_SLIDER(17, "Repeat speed", repeat_speed, false),
  MAKE_SLIDER(19, "Phaser offset", phaser_offset, true),
  MAKE_SLIDER(20, "Phaser sweep", phaser_sweep, true),
  MAKE_SLIDER(21, "LPF cutoff", lpf_cutoff, false),
  MAKE_SLIDER(22, "LPF sweep", lpf_ramp, true),
  MAKE_SLIDER(23, "LPF resonance", lpf_resonance, true),
  MAKE_SLIDER(24, "HPF cutoff", hpf_cutoff, false),
  MAKE_SLIDER(25, "HPF sweep", hpf_ramp, true)
};

static int current_slider = -1;

static float get_slider_value(const az_zfxr_state_t *state,
                          const az_zfxr_slider_t *slider) {
  return *(float*)((char*)(&state->sound_spec) + slider->field_offset);
}

static float *mutable_slider_value(az_zfxr_state_t *state,
                                   const az_zfxr_slider_t *slider) {
  return (float*)((char*)(&state->sound_spec) + slider->field_offset);
}

/*===========================================================================*/

void az_zfxr_draw_screen(const az_zfxr_state_t *state) {
  AZ_ARRAY_LOOP(button, buttons) {
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP); {
      const GLfloat left = button->left + 0.5f;
      const GLfloat right = left + BUTTON_WIDTH;
      const GLfloat top = button->top + 0.5f;
      const GLfloat bottom = top + BUTTON_HEIGHT;
      glVertex2f(left, top); glVertex2f(left, bottom);
      glVertex2f(right, bottom); glVertex2f(right, top);
    } glEnd();
    az_draw_string(8, AZ_ALIGN_CENTER, button->left + BUTTON_WIDTH/2,
                   button->top + 6, button->name);
  }

  AZ_ARRAY_LOOP(button, wave_buttons) {
    if (state->sound_spec.wave_kind == button->kind) glColor3f(0, 1, 0);
    else glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP); {
      const GLfloat left = button->left + 0.5f;
      const GLfloat right = left + WAVE_BUTTON_WIDTH;
      const GLfloat top = button->top + 0.5f;
      const GLfloat bottom = top + BUTTON_HEIGHT;
      glVertex2f(left, top); glVertex2f(left, bottom);
      glVertex2f(right, bottom); glVertex2f(right, top);
    } glEnd();
    az_draw_string(8, AZ_ALIGN_CENTER, button->left + WAVE_BUTTON_WIDTH/2,
                   button->top + 6, button->name);
  }

  int slider_index = 0;
  AZ_ARRAY_LOOP(slider, sliders) {
    const float value = get_slider_value(state, slider);
    const int top = 40 + 16 * slider->y_index;
    const GLfloat x_0 = (slider->allow_negative ? 400.0f : 300.0f);
    const GLfloat value_x =
      x_0 + (slider->allow_negative ? 100.0f : 200.0f) * value;
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0, 0.5, 0);
      glVertex2f(x_0, top + 1); glVertex2f(x_0, top + SLIDER_HEIGHT);
      glVertex2f(value_x, top + 1); glVertex2f(value_x, top + SLIDER_HEIGHT);
    } glEnd();
    glColor3f(1, 1, 1);
    glBegin(GL_LINES); {
      glVertex2f(value_x, top + 1); glVertex2f(value_x, top + SLIDER_HEIGHT);
    } glEnd();
    glBegin(GL_LINE_LOOP); {
      glVertex2f(SLIDER_LEFT - 0.5, top +  0.5);
      glVertex2f(SLIDER_LEFT - 0.5, top + SLIDER_HEIGHT + 0.5);
      glVertex2f(SLIDER_RIGHT + 0.5, top + SLIDER_HEIGHT + 0.5);
      glVertex2f(SLIDER_RIGHT + 0.5, top +  0.5);
    } glEnd();
    if (current_slider == slider_index) glColor3f(0, 1, 1);
    az_draw_string(8, AZ_ALIGN_RIGHT, 290, top + 2, slider->name);
    az_draw_printf(8, AZ_ALIGN_LEFT, 510, top + 2, "%.03f", (double)value);
    ++slider_index;
  }

  glColor3f(1, 1, 1);
  az_draw_string(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT - 12,
                 "Based on \"sfxr\" by DrPetter "
                 "(http://www.drpetter.se/project_sfxr.html)");

  az_draw_cursor();
}

/*===========================================================================*/

void az_zfxr_on_click(az_zfxr_state_t *state, int x, int y) {
  current_slider = -1;

  AZ_ARRAY_LOOP(button, buttons) {
    if (x >= button->left && x <= button->left + BUTTON_WIDTH &&
        y >= button->top && y <= button->top + BUTTON_HEIGHT) {
      button->action(state);
      return;
    }
  }

  AZ_ARRAY_LOOP(button, wave_buttons) {
    if (x >= button->left && x <= button->left + WAVE_BUTTON_WIDTH &&
        y >= button->top && y <= button->top + BUTTON_HEIGHT) {
      state->sound_spec.wave_kind = button->kind;
      state->request_play = true;
      return;
    }
  }

  int slider_index = 0;
  AZ_ARRAY_LOOP(slider, sliders) {
    const int top = 40 + 16 * slider->y_index;
    if (x >= SLIDER_LEFT && x <= SLIDER_RIGHT &&
        y >= top && y <= top + SLIDER_HEIGHT) {
      float *value = mutable_slider_value(state, slider);
      if (slider->allow_negative) {
        *value = (float)(x - SLIDER_CENTER) / (float)(SLIDER_WIDTH/2);
        assert(*value >= -1.0f);
        assert(*value <= 1.0f);
      } else {
        *value = (float)(x - SLIDER_LEFT) / (float)(SLIDER_WIDTH);
        assert(*value >= 0.0f);
        assert(*value <= 1.0f);
      }
      state->request_play = true;
      current_slider = slider_index;
      return;
    }
    ++slider_index;
  }
}

void az_zfxr_on_keypress(az_zfxr_state_t *state, az_key_id_t key_id,
                         bool shift) {
  switch (key_id) {
    case AZ_KEY_SPACE:
      state->request_play = true;
      break;
    case AZ_KEY_LEFT_ARROW:
      if (current_slider >= 0) {
        assert(current_slider < AZ_ARRAY_SIZE(sliders));
        const az_zfxr_slider_t *slider = &sliders[current_slider];
        float *value = mutable_slider_value(state, slider);
        *value = fmax((slider->allow_negative ? -1.0 : 0.0),
                      *value - (shift ? 0.001 : 0.01));
      }
      break;
    case AZ_KEY_RIGHT_ARROW:
      if (current_slider >= 0) {
        assert(current_slider < AZ_ARRAY_SIZE(sliders));
        const az_zfxr_slider_t *slider = &sliders[current_slider];
        float *value = mutable_slider_value(state, slider);
        *value = fmin(1.0, *value + (shift ? 0.001 : 0.01));
      }
      break;
    default: break;
  }
}

/*===========================================================================*/
