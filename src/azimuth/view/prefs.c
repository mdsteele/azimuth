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

#include "azimuth/view/prefs.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/gui/event.h"
#include "azimuth/state/sound.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/prefs.h"
#include "azimuth/view/button.h"
#include "azimuth/view/string.h"

/*===========================================================================*/

#define SLIDER_WIDTH 300
#define SLIDER_THICKNESS 10
#define SLIDER_HANDLE_WIDTH 10
#define SLIDER_HANDLE_HEIGHT 20
#define SLIDERS_TOP 30
#define SLIDER_SPACING 32

static const az_vector_t slider_handle_vertices[] = {
  {0, 0.5 * SLIDER_HANDLE_HEIGHT},
  {-0.5 * SLIDER_HANDLE_WIDTH, 0.5 * SLIDER_HANDLE_HEIGHT - 5.5},
  {-0.5 * SLIDER_HANDLE_WIDTH, -0.5 * SLIDER_HANDLE_HEIGHT + 5.5},
  {0, -0.5 * SLIDER_HANDLE_HEIGHT},
  {0.5 * SLIDER_HANDLE_WIDTH, -0.5 * SLIDER_HANDLE_HEIGHT + 5.5},
  {0.5 * SLIDER_HANDLE_WIDTH, 0.5 * SLIDER_HANDLE_HEIGHT - 5.5}
};
static const az_polygon_t slider_handle_polygon =
  AZ_INIT_POLYGON(slider_handle_vertices);

#define PICKER_WIDTH 30
#define PICKER_HEIGHT 13
#define PICKERS_TOP 85
#define PICKER_HORZ_SPACING 108
#define PICKER_VERT_SPACING 8
#define PICKERS_WEAPON_VERT_SPACING 10
#define PICKER_WEAPON_HORZ_SPACING 80

static const az_vector_t picker_vertices[] = {
  {0.5, 0.5}, {PICKER_WIDTH - 0.5, 0.5},
  {PICKER_WIDTH - 0.5, PICKER_HEIGHT - 0.5}, {0.5, PICKER_HEIGHT - 0.5}
};
static const az_polygon_t picker_polygon =
  AZ_INIT_POLYGON(picker_vertices);

#define CHECKBOX_WIDTH PICKER_HEIGHT
#define CHECKBOX_HEIGHT PICKER_HEIGHT
#define CHECKBOX_SPACING PICKER_VERT_SPACING

static const az_vector_t checkbox_vertices[] = {
  {0.5, 0.5}, {CHECKBOX_WIDTH - 0.5, 0.5},
  {CHECKBOX_WIDTH - 0.5, CHECKBOX_HEIGHT - 0.5}, {0.5, CHECKBOX_HEIGHT - 0.5}
};
static const az_polygon_t checkbox_polygon =
  AZ_INIT_POLYGON(checkbox_vertices);

static void init_slider(az_prefs_slider_t *slider, int x, int y, float value) {
  assert(value >= 0.0f && value <= 1.0f);
  slider->x = x;
  slider->y = y;
  slider->value = value;
  az_init_button(&slider->handle, slider_handle_polygon,
                 value * SLIDER_WIDTH, 0);
}

void az_init_prefs_pane(az_prefs_pane_t *pane, int x, int y,
                        const az_preferences_t *prefs) {
  AZ_ZERO_OBJECT(pane);
  pane->x = x;
  pane->y = y;
  pane->selected_key_picker_index = -1;

  const int slider_left = (AZ_PREFS_BOX_WIDTH - SLIDER_WIDTH) / 2;
  init_slider(&pane->music_slider, slider_left, SLIDERS_TOP,
              prefs->music_volume);
  init_slider(&pane->sound_slider, slider_left, SLIDERS_TOP + SLIDER_SPACING,
              prefs->sound_volume);

  for (int i = AZ_FIRST_CONTROL; i < AZ_CONTROL_BOMBS; ++i) {
    const int delta = i - AZ_FIRST_CONTROL;
    const int row = delta % 4;
    const int col = delta / 4;
    const int top = PICKERS_TOP + row * (PICKER_HEIGHT + PICKER_VERT_SPACING);
    const int left = AZ_PREFS_BOX_WIDTH / 2 - 222 +
      col * (PICKER_WIDTH + PICKER_HORZ_SPACING);
    az_init_button(&pane->pickers[i].button, picker_polygon, left, top);
    pane->pickers[i].key = prefs->key_for_control[i];
  }

  for (int i = AZ_CONTROL_BOMBS; i < AZ_CONTROL_BOMBS + 10; ++i) {
    const int weapon = i - AZ_CONTROL_BOMBS;
    int row, col;
    if (weapon == 0 || weapon == 9) {
      row = weapon == 0 ? 1 : 0;
      col = 4;
    } else {
      row = (weapon - 1) / 4;
      col = (weapon - 1) % 4;
    }
    const int top = PICKERS_TOP + PICKERS_WEAPON_VERT_SPACING +
      (row + 4) * (PICKER_HEIGHT + PICKER_VERT_SPACING);
    const int left = AZ_PREFS_BOX_WIDTH / 2 - 266 +
      col * (PICKER_WIDTH + PICKER_WEAPON_HORZ_SPACING);
    az_init_button(&pane->pickers[i].button, picker_polygon, left, top);
    pane->pickers[i].key = prefs->key_for_control[i];
  }

  const int checkbox_left = AZ_PREFS_BOX_WIDTH / 2 + 50;
  const int checkbox_top = PICKERS_TOP;
  az_init_button(&pane->speedrun_timer_checkbox.button, checkbox_polygon,
                 checkbox_left, checkbox_top);
  pane->speedrun_timer_checkbox.checked = prefs->speedrun_timer;
  az_init_button(&pane->fullscreen_checkbox.button, checkbox_polygon,
                 checkbox_left,
                 checkbox_top + CHECKBOX_HEIGHT + CHECKBOX_SPACING);
  pane->fullscreen_checkbox.checked = prefs->fullscreen_on_startup;
  az_init_button(&pane->enable_hints_checkbox.button, checkbox_polygon,
                 checkbox_left,
                 checkbox_top + 2 * (CHECKBOX_HEIGHT + CHECKBOX_SPACING));
  pane->enable_hints_checkbox.checked = prefs->enable_hints;
}

/*===========================================================================*/

static void draw_slider(const az_prefs_slider_t *slider, const char *label,
                        bool cyan) {
  glPushMatrix(); {
    glTranslatef(slider->x, slider->y, 0);
    // Slider track:
    glBegin(GL_QUADS); {
      glColor3f(0.5, 0.5, 0.5); // gray
      glVertex2f(0, -2); glVertex2f(SLIDER_WIDTH, -2);
      glColor3f(0.25, 0.25, 0.25); // dark gray
      glVertex2f(SLIDER_WIDTH, 2); glVertex2f(0, 2);
    } glEnd();
    // Label:
    if (cyan) glColor3f(0.75, 1, 1); // cyan
    else glColor3f(1, 0.75, 1); // magenta
    az_draw_string(8, AZ_ALIGN_RIGHT, -12, -4, label);
    // Numeric slider value:
    assert(slider->value >= 0);
    assert(slider->value <= 1);
    glColor3f(sqrt(1.0 - slider->value), sqrt(slider->value), 0);
    if (slider->value > 0) {
      az_draw_printf(8, AZ_ALIGN_RIGHT, SLIDER_WIDTH + 42, -4,
                     "%3d%%", 50 + (int)(100.0f * slider->value - 50.0f));
    } else {
      az_draw_string(8, AZ_ALIGN_RIGHT, SLIDER_WIDTH + 40, -4, "OFF");
    }
    // Slider handle:
    az_draw_standard_button(&slider->handle);
  } glPopMatrix();
}

static void draw_key_picker(const az_prefs_key_picker_t *picker,
                            const char *label) {
  az_draw_standard_button(&picker->button);
  glColor3f(1, 1, 1);
  az_draw_string(8, AZ_ALIGN_CENTER, picker->button.x + PICKER_WIDTH / 2,
                 picker->button.y + 3, az_key_name(picker->key));
  az_draw_string(8, AZ_ALIGN_LEFT, picker->button.x + PICKER_WIDTH + 8,
                 picker->button.y + 3, label);
}

static void draw_checkbox(const az_prefs_checkbox_t *checkbox,
                          const char *label) {
  az_draw_standard_button(&checkbox->button);
  if (checkbox->checked) {
    const GLfloat x = checkbox->button.x, y = checkbox->button.y;
    glColor3f(0, 1, 0);
    glBegin(GL_LINES); {
      glVertex2f(2.5f + x, 2.5f + y);
      glVertex2f(CHECKBOX_WIDTH - 2.5f + x, CHECKBOX_HEIGHT - 2.5f + y);
      glVertex2f(2.5f + x, CHECKBOX_HEIGHT - 2.5f + y);
      glVertex2f(CHECKBOX_WIDTH - 2.5f + x, 2.5f + y);
    } glEnd();
  }
  glColor3f(1, 1, 1);
  az_draw_string(8, AZ_ALIGN_LEFT, checkbox->button.x + CHECKBOX_WIDTH + 8,
                 checkbox->button.y + 3, label);
}

void az_draw_prefs_pane(const az_prefs_pane_t *pane) {
  glPushMatrix(); {
    glTranslatef(pane->x, pane->y, 0);

    draw_slider(&pane->music_slider, "Music", true);
    draw_slider(&pane->sound_slider, "Sound", false);

    draw_key_picker(&pane->pickers[AZ_CONTROL_UP], "Thrust");
    draw_key_picker(&pane->pickers[AZ_CONTROL_DOWN], "Reverse");
    draw_key_picker(&pane->pickers[AZ_CONTROL_RIGHT], "Turn right");
    draw_key_picker(&pane->pickers[AZ_CONTROL_LEFT], "Turn left");
    draw_key_picker(&pane->pickers[AZ_CONTROL_FIRE], "Fire");
    draw_key_picker(&pane->pickers[AZ_CONTROL_ORDN], "Ordnance");
    draw_key_picker(&pane->pickers[AZ_CONTROL_UTIL], "Utility");
    draw_key_picker(&pane->pickers[AZ_CONTROL_PAUSE], "Pause");

    draw_checkbox(&pane->speedrun_timer_checkbox, "Show speedrun timer");
    draw_checkbox(&pane->fullscreen_checkbox, "Fullscreen on startup");
    draw_checkbox(&pane->enable_hints_checkbox, "Enable hint system");

    draw_key_picker(&pane->pickers[AZ_CONTROL_BOMBS], "Bombs");
    draw_key_picker(&pane->pickers[AZ_CONTROL_CHARGE], "Charge");
    draw_key_picker(&pane->pickers[AZ_CONTROL_FREEZE], "Freeze");
    draw_key_picker(&pane->pickers[AZ_CONTROL_TRIPLE], "Triple");
    draw_key_picker(&pane->pickers[AZ_CONTROL_HOMING], "Homing");
    draw_key_picker(&pane->pickers[AZ_CONTROL_PHASE], "Phase");
    draw_key_picker(&pane->pickers[AZ_CONTROL_BURST], "Burst");
    draw_key_picker(&pane->pickers[AZ_CONTROL_PIERCE], "Pierce");
    draw_key_picker(&pane->pickers[AZ_CONTROL_BEAM], "Beam");
    draw_key_picker(&pane->pickers[AZ_CONTROL_ROCKETS], "Rockets");
  } glPopMatrix();
}

/*===========================================================================*/

static bool tick_slider(az_prefs_slider_t *slider, int xoff, int yoff,
                        bool active, double time, az_clock_t clock,
                        az_soundboard_t *soundboard) {
  bool released = false;
  if (slider->grabbed) {
    if (!az_is_mouse_held()) {
      slider->grabbed = false;
      released = true;
    } else if (active) {
      int mouse_x, mouse_y;
      if (az_get_mouse_position(&mouse_x, &mouse_y)) {
        const double rel_x = fmin(fmax(0.0, mouse_x - xoff - slider->x),
                                  SLIDER_WIDTH);
        slider->handle.x = rel_x;
        slider->value = rel_x / SLIDER_WIDTH;
      }
    }
  }
  if (!slider->grabbed) {
    az_tick_button(&slider->handle, xoff + slider->x, yoff + slider->y,
                   active, time, clock, soundboard);
  }
  return released;
}

void az_tick_prefs_pane(az_prefs_pane_t *pane, bool visible, double time,
                        az_clock_t clock, az_soundboard_t *soundboard) {
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    if (i != pane->selected_key_picker_index) {
      az_tick_button(&pane->pickers[i].button, pane->x, pane->y, visible, time,
                     clock, soundboard);
    }
  }
  const bool active = visible && pane->selected_key_picker_index < 0;
  tick_slider(&pane->music_slider, pane->x, pane->y, active, time, clock,
              soundboard);
  if (tick_slider(&pane->sound_slider, pane->x, pane->y, active, time, clock,
                  soundboard)) {
    az_play_sound(soundboard, AZ_SND_BLINK_MEGA_BOMB);
  }
  az_tick_button(&pane->speedrun_timer_checkbox.button,
                 pane->x, pane->y, active, time, clock, soundboard);
  az_tick_button(&pane->fullscreen_checkbox.button,
                 pane->x, pane->y, active, time, clock, soundboard);
  az_tick_button(&pane->enable_hints_checkbox.button,
                 pane->x, pane->y, active, time, clock, soundboard);
}

/*===========================================================================*/

static void slider_on_click(az_prefs_slider_t *slider, int x, int y,
                            az_soundboard_t *soundboard) {
  if (az_button_on_click(&slider->handle, x - slider->x, y - slider->y,
                         soundboard)) {
    slider->grabbed = true;
  }
}

static void checkbox_on_click(az_prefs_checkbox_t *checkbox, int x, int y,
                              az_soundboard_t *soundboard) {
  if (az_button_on_click(&checkbox->button, x, y, soundboard)) {
    checkbox->checked = !checkbox->checked;
  }
}

void az_prefs_pane_on_click(az_prefs_pane_t *pane, int abs_x, int abs_y,
                            az_soundboard_t *soundboard) {
  const int rel_x = abs_x - pane->x, rel_y = abs_y - pane->y;
  for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
    if (az_button_on_click(&pane->pickers[i].button, rel_x, rel_y,
                           soundboard)) {
      pane->selected_key_picker_index = i;
    }
  }
  if (pane->selected_key_picker_index < 0) {
    slider_on_click(&pane->music_slider, rel_x, rel_y, soundboard);
    slider_on_click(&pane->sound_slider, rel_x, rel_y, soundboard);
    checkbox_on_click(&pane->speedrun_timer_checkbox, rel_x, rel_y,
                      soundboard);
    checkbox_on_click(&pane->fullscreen_checkbox, rel_x, rel_y, soundboard);
    checkbox_on_click(&pane->enable_hints_checkbox, rel_x, rel_y, soundboard);
  }
}

void az_prefs_try_pick_key(az_prefs_pane_t *pane, az_key_id_t key_id,
                           az_soundboard_t *soundboard) {
  const int picker_index = pane->selected_key_picker_index;
  if (picker_index < 0 || picker_index >= AZ_NUM_CONTROLS) return;
  az_prefs_key_picker_t *picker = &pane->pickers[picker_index];
  pane->selected_key_picker_index = -1;
  const az_control_id_t control_id = (az_control_id_t)picker_index;
  if (az_is_valid_prefs_key(key_id, control_id)) {
    // Ensure no one else is using the same key_id:
    for (int i = AZ_FIRST_CONTROL; i < AZ_NUM_CONTROLS; ++i) {
      if (i == picker_index) continue;
      if (pane->pickers[i].key == key_id) {
        // Check if we can reassign picker's current key to other control:
        if (az_is_valid_prefs_key(picker->key, (az_control_id_t)i)) {
          pane->pickers[i].key = picker->key;
        } else if (i >= AZ_CONTROL_BOMBS && i <= AZ_CONTROL_ROCKETS) {
          // We were trading keys between weapon slots, but we are
          // not allowed to do so.  Reset weapon slot to the default value:
          pane->pickers[i].key = (az_key_id_t)
            (AZ_KEY_0 + i - AZ_CONTROL_BOMBS);
        } else {
          fprintf(stderr, "Key [%s] already in use.  "
                  "Cannot swap numeric weapon key with a non-weapon control.\n",
                  az_key_name(key_id));
          az_play_sound(soundboard, AZ_SND_KLAXON_SHIELDS_LOW);
          return;
        }
        break;
      }
    }
    picker->key = key_id;
  }
  az_play_sound(soundboard, AZ_SND_MENU_CLICK);
}

/*===========================================================================*/
