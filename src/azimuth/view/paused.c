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

#include "azimuth/view/paused.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/gui/event.h"
#include "azimuth/state/camera.h"
#include "azimuth/state/planet.h"
#include "azimuth/state/player.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/upgrade.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/button.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/minimap.h"
#include "azimuth/view/node.h"
#include "azimuth/view/prefs.h"
#include "azimuth/view/ship.h"
#include "azimuth/view/string.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_bezel_box(double bezel, double x, double y,
                           double w, double h) {
  x += 0.5; y += 0.5; w -= 1.0; h -= 1.0;
  glBegin(GL_LINE_LOOP); {
    glVertex2d(x + bezel, y); glVertex2d(x + w - bezel, y);
    glVertex2d(x + w, y + bezel); glVertex2d(x + w, y + h - bezel);
    glVertex2d(x + w - bezel, y + h); glVertex2d(x + bezel, y + h);
    glVertex2d(x, y + h - bezel); glVertex2d(x, y + bezel);
  } glEnd();
}

static void draw_weapon_box(double x, double y, double w, double h,
                            int digit, bool has, bool selected, bool label) {
  assert(digit >= 0 && digit <= 9);
  assert(has || !selected);
  if (has && label) {
    glPushMatrix(); {
      glTranslated(x + 2.5, y + 5.5, 0);
      glColor3f(0.5, 1, 0.5);
      switch (digit) {
        case 0:
          glBegin(GL_LINE_LOOP); {
            glVertex2f(0, 0); glVertex2f(2, 0); glVertex2f(2, 4);
            glVertex2f(0, 4);
          } glEnd();
          break;
        case 1:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(0, 1); glVertex2f(1, 0); glVertex2f(1, 4);
          } glEnd();
          glBegin(GL_LINES); {
            glVertex2f(-0.5, 4); glVertex2f(2.5, 4);
          } glEnd();
          break;
        case 2:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(-0.5, 0); glVertex2f(2, 0); glVertex2f(2, 2);
            glVertex2f(0, 2); glVertex2f(0, 4); glVertex2f(2.5, 4);
          } glEnd();
          break;
        case 3:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(-0.5, 0); glVertex2f(2, 0); glVertex2f(2, 4);
            glVertex2f(-0.5, 4);
          } glEnd();
          glBegin(GL_LINES); {
            glVertex2f(0, 2); glVertex2f(2, 2);
          } glEnd();
          break;
        case 4:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(0, -0.5); glVertex2f(0, 2); glVertex2f(2, 2);
          } glEnd();
          glBegin(GL_LINES); {
            glVertex2f(2, -0.5); glVertex2f(2, 4.5);
          } glEnd();
          break;
        case 5:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(2.5, 0); glVertex2f(0, 0); glVertex2f(0, 2);
            glVertex2f(2, 2); glVertex2f(2, 4); glVertex2f(-0.5, 4);
          } glEnd();
          break;
        case 6:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(2.5, 0); glVertex2f(0, 0); glVertex2f(0, 4);
            glVertex2f(2, 4); glVertex2f(2, 2); glVertex2f(0, 2);
          } glEnd();
          break;
        case 7:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(-0.5, 0); glVertex2f(2, 0); glVertex2f(1, 4);
          } glEnd();
          break;
        case 8:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(0, 2); glVertex2f(0, 4); glVertex2f(2, 4);
            glVertex2f(2, 0); glVertex2f(0, 0); glVertex2f(0, 2);
            glVertex2f(2, 2);
          } glEnd();
          break;
        case 9:
          glBegin(GL_LINE_STRIP); {
            glVertex2f(-0.5, 4); glVertex2f(2, 4); glVertex2f(2, 0);
            glVertex2f(0, 0); glVertex2f(0, 2); glVertex2f(2, 2);
          } glEnd();
          break;
      }
    } glPopMatrix();
    glBegin(GL_LINE_STRIP); {
      if (selected) glColor3f(0.5, 0.5, 0.5);
      else glColor3f(0.5, 0, 0.5);
      glVertex2d(x + 4.5, y + 0.5);
      glVertex2d(x + 6.5, y + 2.5);
      glVertex2d(x + 6.5, y + h - 2.5);
      glVertex2d(x + 4.5, y + h - 0.5);
    } glEnd();
  }
  if (selected) glColor3f(1, 1, 1);
  else if (has) glColor3f(1, 0, 1);
  else glColor3f(0.5, 0, 0.5);
  draw_bezel_box(2, x, y, w, h);
}

/*===========================================================================*/
// Minimap geometry:

#define MINIMAP_ZOOM 75.0
#define SCROLL_Y_MAX (AZ_PLANETOID_RADIUS - 180 * MINIMAP_ZOOM)
#define SCROLL_SPEED (AZ_PLANETOID_RADIUS)

// Return the minimap screen coordinates for a given absolute position.
static az_vector_t minimap_position(const az_paused_state_t *state,
                                    az_vector_t pos) {
  pos.y -= state->scroll_y;
  pos.x /= MINIMAP_ZOOM;
  pos.y /= MINIMAP_ZOOM;
  pos.x += AZ_SCREEN_WIDTH/2;
  pos.y = AZ_SCREEN_HEIGHT/2 - pos.y;
  return pos;
}

static az_vector_t minimap_room_center(const az_paused_state_t *state,
                                       const az_room_t *room) {
  return minimap_position(state, az_room_center(room));
}

/*===========================================================================*/
// Drawing minimap:

static void draw_minimap_rooms(const az_paused_state_t *state,
                               az_room_flags_t *room_flags_out) {
  // Draw planet surface outline:
  glColor3f(1, 1, 0); // yellow
  glBegin(GL_LINE_STRIP); {
    for (int i = 45; i <= 135; i += 3) {
      glVertex2d(AZ_PLANETOID_RADIUS * cos(AZ_DEG2RAD(i)),
                 AZ_PLANETOID_RADIUS * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();

  const az_planet_t *planet = state->planet;
  const az_ship_t *ship = state->ship;
  const az_player_t *player = &ship->player;

  // Draw rooms that are mapped or explored:
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    if (!az_test_room_mapped(player, i, room)) continue;
    const bool visited = az_test_room_visited(player, i);
    az_draw_minimap_room(planet, room, visited, false, AZ_VZERO);
    *room_flags_out |= room->properties & AZ_ROOMF_WITH_SAVE;
    if (visited) {
      *room_flags_out |= room->properties & (AZ_ROOMF_WITH_COMM |
                                             AZ_ROOMF_WITH_REFILL);
    }
  }
}

static void draw_map_markers(const az_paused_state_t *state) {
  const az_planet_t *planet = state->planet;
  const az_player_t *player = &state->ship->player;
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    if (az_should_mark_room(player, i, room)) {
      az_draw_map_marker(minimap_room_center(state, room), state->clock);
    }
  }
}

static void draw_minimap_room_labels(const az_paused_state_t *state) {
  const az_planet_t *planet = state->planet;
  const az_player_t *player = &state->ship->player;
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    if (!az_test_room_mapped(player, i, room)) continue;
    if (room->properties & AZ_ROOMF_WITH_SAVE) {
      az_draw_save_room_label(minimap_room_center(state, room));
    } else if (az_test_room_visited(player, i)) {
      if (room->properties & AZ_ROOMF_WITH_REFILL) {
        az_draw_refill_room_label(minimap_room_center(state, room));
      } else if (room->properties & AZ_ROOMF_WITH_COMM) {
        az_draw_comm_room_label(minimap_room_center(state, room));
      }
    }
  }
}

static void draw_minimap_hint_label(const az_paused_state_t *state) {
  assert(state->hint.active);
  const az_planet_t *planet = state->planet;
  const az_room_t *room = &planet->rooms[state->hint.target_room];
  const az_vector_t center = minimap_room_center(state, room);
  const double offset = AZ_SCREEN_WIDTH * (1 - state->hint.progress) +
    az_clock_zigzag(3, 10, state->clock) + 3;
  glPushMatrix(); {
    glTranslated(round(center.x), round(center.y), 0);
    for (int i = 0; i < 4; ++i) {
      glPushMatrix(); {
        glTranslated(offset, offset, 0);
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(1, 1, 1);
          glVertex2f(4, 4); glVertex2f(0, 4); glVertex2f(0, 7);
          glVertex2f(7, 7); glVertex2f(7, 0); glVertex2f(4, 0);
        } glEnd();
        glBegin(GL_LINE_STRIP); {
          glColor3f(1, 0, 0);
          glVertex2f(1.5, 5.5); glVertex2f(5.5, 5.5); glVertex2f(5.5, 1.5);
        } glEnd();
      } glPopMatrix();
      az_gl_rotated(AZ_HALF_PI);
    }
  } glPopMatrix();
}

static void draw_minimap_ship_label(const az_paused_state_t *state) {
  const az_vector_t center = minimap_position(state, state->ship->position);
  glPushMatrix(); {
    glTranslatef((int)center.x, (int)center.y, 0);
    glRotatef(-90 * round(state->ship->angle / AZ_HALF_PI), 0, 0, 1);
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.75, 0.75, 0.75);
      glVertex2f(-6, -6); glVertex2f(1, -6);
      glVertex2f(-7, -2); glVertex2f(7, -2);
      glVertex2f(-7,  0); glVertex2f(9,  0);
      glVertex2f(-7,  2); glVertex2f(7,  2);
      glVertex2f(-6,  6); glVertex2f(1,  6);
    } glEnd();
    // Struts:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25);
      glVertex2f(-4, -4); glVertex2f( 0, -4);
      glVertex2f(-4,  4); glVertex2f( 0,  4);
    } glEnd();
    // Port engine:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-5, -5); glVertex2f(1, -5);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-6, -3); glVertex2f(2, -3);
    } glEnd();
    // Starboard engine:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-6,  3); glVertex2f(2,  3);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-5,  5); glVertex2f(1,  5);
    } glEnd();
    // Main body:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-6, -2); glVertex2f(6, -2);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-6,  0); glVertex2f(8,  0);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-6,  2); glVertex2f(6,  2);
    } glEnd();
    // Windshield:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0, 0.50, 0.50); glVertex2f(5,  1);
      glColor3f(0, 0.75, 0.75); glVertex2f(7,  0); glVertex2f(4, 0);
      glColor3f(0, 0.50, 0.50); glVertex2f(5, -1);
    } glEnd();
  } glPopMatrix();
}

static void draw_minimap(const az_paused_state_t *state) {
  az_room_flags_t room_flags_union = 0;
  glEnable(GL_SCISSOR_TEST); {
    glScissor(30, 50, 580, 400);
    glPushMatrix(); {
      // Make positive Y be up instead of down.
      glScaled(1, -1, 1);
      // Center the screen on position (0, 0).
      glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
      // Zoom out.
      glScaled(1.0 / MINIMAP_ZOOM, 1.0 / MINIMAP_ZOOM, 1);
      // Move the screen to the camera pose.
      glTranslated(0, -state->scroll_y, 0);
      // Draw what the camera sees.
      draw_minimap_rooms(state, &room_flags_union);
    } glPopMatrix();
    if (state->current_drawer == AZ_PAUSE_DRAWER_MAP) {
      draw_map_markers(state);
      if (az_clock_mod(2, 30, state->clock) == 0) {
        draw_minimap_room_labels(state);
      }
      if (state->hint.active) {
        draw_minimap_hint_label(state);
      }
      if (az_clock_mod(2, 12, state->clock) == 0) {
        draw_minimap_ship_label(state);
      }
    }
  } glDisable(GL_SCISSOR_TEST);

  glColor3f(0.75, 0.75, 0.75);
  draw_bezel_box(2, 30, 30, 580, 400);

  const az_planet_t *planet = state->planet;
  const az_room_key_t current_room_key = state->ship->player.current_room;
  assert(current_room_key < planet->num_rooms);
  const az_room_t *current_room = &planet->rooms[current_room_key];
  assert(current_room->zone_key < planet->num_zones);
  const az_zone_t *current_zone = &planet->zones[current_room->zone_key];
  glColor3f(1, 1, 1);
  az_draw_string(8, AZ_ALIGN_CENTER, 320, 17, "Zenith Planetoid");
  az_draw_string(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 46 -
                 8 * strlen(current_zone->name), 17, "Location:");
  az_gl_color(current_zone->color);
  az_draw_string(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 38, 17,
                 current_zone->name);

  if (state->scroll_y_min < SCROLL_Y_MAX) {
    const az_color_t normal = {64, 192, 255, 255};
    const az_color_t hilight = {255, 255, 255, 255};
    const az_color_t dark = {32, 32, 32, 255};
    const bool disabled = (state->current_drawer != AZ_PAUSE_DRAWER_MAP);
    const bool blink = az_clock_mod(2, 20, state->clock);
    if (disabled || state->scroll_y >= SCROLL_Y_MAX) az_gl_color(dark);
    else if (blink) az_gl_color(hilight);
    else az_gl_color(normal);
    az_draw_printf(8, AZ_ALIGN_CENTER, 78, 367, "[%s]",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_UP]));
    if (disabled) az_gl_color(dark);
    else az_gl_color(normal);
    az_draw_string(8, AZ_ALIGN_CENTER, 78, 380, "SCROLL");
    if (disabled || state->scroll_y <= state->scroll_y_min) az_gl_color(dark);
    else if (blink) az_gl_color(hilight);
    else az_gl_color(normal);
    az_draw_printf(8, AZ_ALIGN_CENTER, 78, 393, "[%s]",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_DOWN]));
  }

  if (room_flags_union & AZ_ROOMF_WITH_SAVE) {
    az_draw_save_room_label((az_vector_t){541, 370});
    glColor3f(0.75, 0.75, 0.75);
    az_draw_string(8, AZ_ALIGN_LEFT, 550, 367, "Save");
  }
  if (room_flags_union & AZ_ROOMF_WITH_COMM) {
    az_draw_comm_room_label((az_vector_t){541, 383});
    glColor3f(0.75, 0.75, 0.75);
    az_draw_string(8, AZ_ALIGN_LEFT, 550, 380, "Comm");
  }
  if (room_flags_union & AZ_ROOMF_WITH_REFILL) {
    az_draw_refill_room_label((az_vector_t){541, 396});
    glColor3f(0.75, 0.75, 0.75);
    az_draw_string(8, AZ_ALIGN_LEFT, 550, 393, "Repair");
  }
}

/*===========================================================================*/
// Drawing schematic:

static void draw_ship_schematic(void) {
  glColor3f(0, 1, 0);
  // Struts:
  glBegin(GL_LINES); {
    glVertex2f(1, 4); glVertex2f(1, 7);
    glVertex2f(-7, 4); glVertex2f(-7, 7);
    glVertex2f(1, -4); glVertex2f(1, -7);
    glVertex2f(-7, -4); glVertex2f(-7, -7);
  } glEnd();
  // Port engine:
  glBegin(GL_LINE_LOOP); {
    glVertex2f(-10, 12); glVertex2f(6, 12);
    glVertex2f(8, 7); glVertex2f(-11, 7);
  } glEnd();
  // Starboard engine:
  glBegin(GL_LINE_LOOP); {
    glVertex2f(8, -7); glVertex2f(-11, -7);
    glVertex2f(-10, -12); glVertex2f(6, -12);
  } glEnd();
  // Main body:
  glBegin(GL_LINE_LOOP); {
    glVertex2f(20, 0); glVertex2f(15, -4); glVertex2f(-14, -4);
    glVertex2f(-14, 4); glVertex2f(15, 4);
  } glEnd();
  // Windshield:
  glBegin(GL_LINE_LOOP); {
    glVertex2f(18, 0); glVertex2f(15, -2);
    glVertex2f(12, 0); glVertex2f(15, 2);
  } glEnd();
}

static const az_vector_t gun_line_vertices[] = {
  {320, 65}, {320, 95}, {301, 95}, {301, 129}
};
static const az_vector_t armor_line_vertices[] = {
  {176, 123}, {255, 123}, {255, 185}, {310, 185}
};
static const az_vector_t reactor_line_vertices[] = {
  {176, 203}, {230, 203}, {230, 260}, {290, 260}
};
static const az_vector_t util_line_vertices[] = {
  {176, 273}, {205, 273}, {205, 338}, {330, 338}, {330, 290}
};
static const az_vector_t ordnance_line_vertices[] = {
  {464, 123}, {320, 123}, {320, 145}
};
static const az_vector_t engines_line_vertices[] = {
  {464, 213}, {430, 213}, {430, 245}, {380, 245}
};
static const az_vector_t scan_line_vertices[] = {
  {464, 283}, {357, 283}, {357, 255}
};

static void vertex_between(az_vector_t v0, az_vector_t v1, double amount) {
  az_gl_vertex(az_vadd(az_vmul(v0, 1.0 - amount), az_vmul(v1, amount)));
}

static void draw_schematic_line(az_clock_t clock, const az_vector_t *vertices,
                                int num_vertices) {
  assert(num_vertices > 0);
  const double span = 0.35;
  const double sweep = 0.01 + 0.02 * az_clock_mod(50, 1, clock);
  assert(0.0 < sweep && sweep < 1.0);
  const GLfloat glow = (sweep < span ? 1.0 - sweep / span :
                        sweep > 1.0 - span ? 1.0 - (1.0 - sweep) / span : 0.0);
  glBegin(GL_LINE_STRIP); {
    for (int i = 0; ; ++i) {
      glColor3f(0.75f * glow, 0.75f + 0.25f * glow, 0.25f + 0.5f * glow);
      az_gl_vertex(vertices[i]);
      if (i == num_vertices - 1) break;
      if (sweep > 1.0 - span) {
        glColor3f(0, 0.75, 0.25);
        vertex_between(vertices[i], vertices[i + 1], sweep - 1.0 + span);
      }
      if (sweep > span) {
        glColor3f(0, 0.75, 0.25);
        vertex_between(vertices[i], vertices[i + 1], sweep - span);
      }
      glColor3f(0.75, 1, 0.75);
      vertex_between(vertices[i], vertices[i + 1], sweep);
      if (sweep < 1.0 - span) {
        glColor3f(0, 0.75, 0.25);
        vertex_between(vertices[i], vertices[i + 1], sweep + span);
      }
      if (sweep < span) {
        glColor3f(0, 0.75, 0.25);
        vertex_between(vertices[i], vertices[i + 1], sweep + 1.0 - span);
      }
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    const az_vector_t center = vertices[num_vertices - 1];
    az_gl_vertex(center);
    const double radius = 3.0;
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(center.x + radius * cos(AZ_DEG2RAD(i)),
                 center.y + radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

/*===========================================================================*/
// Upgrades drawer:

#define UPGRADE_DRAWER_SLIDE_DISTANCE 410

static const az_vector_t upgrade_drawer_vertices[] = {
  {12.5, 480.5}, {12.5, 20.5}, {22.5, 10.5}, {202.5, 10.5}, {206.5, 14.5},
  {421.5, 14.5}, {432.5, 3.5}, {627.5, 3.5}, {627.5, 480.5}
};

#define UPGRADE_DRAWER_HANDLE_TOP 3
#define UPGRADE_DRAWER_HANDLE_LEFT 0
static const az_vector_t upgrade_drawer_handle_vertices[] = {
  {627.5, 19.5}, {627.5, 0.5}, {432.5, 0.5}, {421.5, 11.5}, {429.5, 19.5}
};
static const az_polygon_t upgrade_drawer_handle_polygon =
  AZ_INIT_POLYGON(upgrade_drawer_handle_vertices);

#define GUN_BOX_WIDTH 64
#define UPG_BOX_WIDTH 150
#define UPG_BOX_HEIGHT 15

#define TANK_BOX_LEFT 24
#define SHIELD_BOX_TOP 26
#define ENERGY_BOX_TOP 46

#define ORDN_BOX_LEFT 466
#define ROCKETS_BOX_TOP 26
#define BOMBS_BOX_TOP 46

static const az_vector_t upgrade_toplefts[] = {
  [AZ_UPG_GUN_CHARGE] = {185, 26},
  [AZ_UPG_GUN_FREEZE] = {254, 26},
  [AZ_UPG_GUN_TRIPLE] = {322, 26},
  [AZ_UPG_GUN_HOMING] = {391, 26},
  [AZ_UPG_GUN_PHASE]  = {185, 46},
  [AZ_UPG_GUN_BURST]  = {254, 46},
  [AZ_UPG_GUN_PIERCE] = {322, 46},
  [AZ_UPG_GUN_BEAM]   = {391, 46},
  [AZ_UPG_HYPER_ROCKETS] = {466, 86},
  [AZ_UPG_MEGA_BOMBS] = {466, 106},
  [AZ_UPG_HIGH_EXPLOSIVES] = {466, 126},
  [AZ_UPG_ATTUNED_EXPLOSIVES] = {466, 146},
  [AZ_UPG_RETRO_THRUSTERS] = {466, 186},
  [AZ_UPG_CPLUS_DRIVE] = {466, 206},
  [AZ_UPG_ORION_BOOSTER] = {466, 226},
  [AZ_UPG_HARDENED_ARMOR] = {24, 86},
  [AZ_UPG_DYNAMIC_ARMOR] = {24, 106},
  [AZ_UPG_THERMAL_ARMOR] = {24, 126},
  [AZ_UPG_REACTIVE_ARMOR] = {24, 146},
  [AZ_UPG_FUSION_REACTOR] = {24, 186},
  [AZ_UPG_QUANTUM_REACTOR] = {24, 206},
  [AZ_UPG_TRACTOR_BEAM] = {24, 246},
  [AZ_UPG_TRACTOR_CLOAK] = {24, 266},
  [AZ_UPG_MAGNET_SWEEP] = {24, 286},
  [AZ_UPG_INFRASCANNER] = {466, 266},
  [AZ_UPG_MILLIWAVE_RADAR] = {466, 286}
};

static void draw_upg_box(const az_player_t *player, az_upgrade_t upgrade,
                         bool *has_upgrade_out) {
  const az_vector_t topleft = upgrade_toplefts[upgrade];
  const bool has_upgrade = az_has_upgrade(player, upgrade);
  if (has_upgrade) glColor3f(0, 1, 1);
  else glColor3f(0, 0.5, 0.5);
  draw_bezel_box(2, topleft.x, topleft.y, UPG_BOX_WIDTH, UPG_BOX_HEIGHT);
  if (!has_upgrade) return;
  az_draw_string(8, AZ_ALIGN_CENTER, topleft.x + UPG_BOX_WIDTH/2,
                 topleft.y + 4, az_upgrade_name(upgrade));
  *has_upgrade_out = true;
}

static int round_towards_middle(double amount, double maximum) {
  assert(amount >= 0.0);
  assert(amount <= maximum);
  const int offset = 0.5 * maximum;
  return (int)(amount - offset) + offset;
}

static void draw_upgrades(const az_paused_state_t *state) {
  const az_player_t *player = &state->ship->player;

  glColor4f(0, 0, 0, 0.92); // black tint
  glBegin(GL_TRIANGLE_FAN); {
    AZ_ARRAY_LOOP(vertex, upgrade_drawer_vertices) az_gl_vertex(*vertex);
  } glEnd();
  glColor3f(0.75, 0.75, 0.75); // light gray
  glBegin(GL_LINE_LOOP); {
    AZ_ARRAY_LOOP(vertex, upgrade_drawer_vertices) az_gl_vertex(*vertex);
  } glEnd();

  az_draw_borderless_button(&state->upgrade_drawer_handle);
  if (az_clock_mod(2, 40, state->clock)) glColor3f(0.5, 1, 0.5);
  else glColor3f(0, 1, 0);
  if (state->current_drawer == AZ_PAUSE_DRAWER_UPGRADES) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 525, 10, "\x12 MAP [%s] \x12",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_ORDN]));
  } else {
    az_draw_printf(8, AZ_ALIGN_CENTER, 525, 10, "\x11 UPGRADES [%s] \x11",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_FIRE]));
  }

  glColor3f(1, 0, 1);
  draw_bezel_box(2, TANK_BOX_LEFT, SHIELD_BOX_TOP,
                 UPG_BOX_WIDTH, UPG_BOX_HEIGHT);
  az_draw_printf(8, AZ_ALIGN_CENTER, TANK_BOX_LEFT + UPG_BOX_WIDTH/2,
                 SHIELD_BOX_TOP + 4, "SHIELD: %3d/%-3d",
                 round_towards_middle(player->shields, player->max_shields),
                 (int)player->max_shields);
  draw_bezel_box(2, TANK_BOX_LEFT, ENERGY_BOX_TOP,
                 UPG_BOX_WIDTH, UPG_BOX_HEIGHT);
  az_draw_printf(8, AZ_ALIGN_CENTER, TANK_BOX_LEFT + UPG_BOX_WIDTH/2,
                 ENERGY_BOX_TOP + 4, "ENERGY: %3d/%-3d",
                 round_towards_middle(player->energy, player->max_energy),
                 (int)player->max_energy);

  int num_guns = 0;
  for (int i = 0; i < 8; ++i) {
    if (az_has_upgrade(player, AZ_UPG_GUN_CHARGE + i)) ++num_guns;
  }
  for (int i = 0; i < 8; ++i) {
    const az_gun_t gun = AZ_GUN_CHARGE + i;
    const az_upgrade_t upgrade = AZ_UPG_GUN_CHARGE + i;
    const az_vector_t topleft = upgrade_toplefts[upgrade];
    const bool has_gun = az_has_upgrade(player, upgrade);
    draw_weapon_box(topleft.x, topleft.y, GUN_BOX_WIDTH, UPG_BOX_HEIGHT, i + 1,
                    has_gun, player->gun1 == gun || player->gun2 == gun,
                    num_guns > 2);
    if (has_gun) {
      az_draw_string(8, AZ_ALIGN_CENTER,
                     topleft.x + GUN_BOX_WIDTH/2 + (num_guns > 2 ? 1 : 0),
                     topleft.y + 4, az_gun_name(gun));
    }
  }

  draw_weapon_box(ORDN_BOX_LEFT, ROCKETS_BOX_TOP, UPG_BOX_WIDTH,
                  UPG_BOX_HEIGHT, 9, player->max_rockets > 0,
                  player->ordnance == AZ_ORDN_ROCKETS, player->max_bombs > 0);
  if (player->max_rockets > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, ORDN_BOX_LEFT + UPG_BOX_WIDTH/2,
                   ROCKETS_BOX_TOP + 4, "ROCKETS:%3d/%-3d",
                   player->rockets, player->max_rockets);
  }
  draw_weapon_box(ORDN_BOX_LEFT, BOMBS_BOX_TOP, UPG_BOX_WIDTH,
                  UPG_BOX_HEIGHT, 0, player->max_bombs > 0,
                  player->ordnance == AZ_ORDN_BOMBS, player->max_rockets > 0);
  if (player->max_bombs > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, ORDN_BOX_LEFT + UPG_BOX_WIDTH/2,
                   BOMBS_BOX_TOP + 4, "  BOMBS:%3d/%-3d",
                   player->bombs, player->max_bombs);
  }

  if (state->drawer_slide <= 0.0) return;

  glPushMatrix(); {
    glTranslatef(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);
    glRotatef(-100, 0, 0, 1);
    glScalef(6, 6, 1);
    draw_ship_schematic();
  } glPopMatrix();

  if (num_guns > 0) {
    draw_schematic_line(state->clock, gun_line_vertices,
                        AZ_ARRAY_SIZE(gun_line_vertices));
  }

  bool has_armor = false;
  draw_upg_box(player, AZ_UPG_HARDENED_ARMOR, &has_armor);
  draw_upg_box(player, AZ_UPG_DYNAMIC_ARMOR, &has_armor);
  draw_upg_box(player, AZ_UPG_THERMAL_ARMOR, &has_armor);
  draw_upg_box(player, AZ_UPG_REACTIVE_ARMOR, &has_armor);
  if (has_armor) {
    draw_schematic_line(state->clock, armor_line_vertices,
                        AZ_ARRAY_SIZE(armor_line_vertices));
  }

  bool has_reactor = false;
  draw_upg_box(player, AZ_UPG_FUSION_REACTOR, &has_reactor);
  draw_upg_box(player, AZ_UPG_QUANTUM_REACTOR, &has_reactor);
  if (has_reactor) {
    draw_schematic_line(state->clock, reactor_line_vertices,
                        AZ_ARRAY_SIZE(reactor_line_vertices));
  }

  bool has_util = false;
  draw_upg_box(player, AZ_UPG_TRACTOR_BEAM, &has_util);
  draw_upg_box(player, AZ_UPG_TRACTOR_CLOAK, &has_util);
  draw_upg_box(player, AZ_UPG_MAGNET_SWEEP, &has_util);
  if (has_util) {
    draw_schematic_line(state->clock, util_line_vertices,
                        AZ_ARRAY_SIZE(util_line_vertices));
  }

  bool has_ordnance = false;
  draw_upg_box(player, AZ_UPG_HYPER_ROCKETS, &has_ordnance);
  draw_upg_box(player, AZ_UPG_MEGA_BOMBS, &has_ordnance);
  draw_upg_box(player, AZ_UPG_HIGH_EXPLOSIVES, &has_ordnance);
  draw_upg_box(player, AZ_UPG_ATTUNED_EXPLOSIVES, &has_ordnance);
  if (has_ordnance) {
    draw_schematic_line(state->clock, ordnance_line_vertices,
                        AZ_ARRAY_SIZE(ordnance_line_vertices));
  }

  bool has_engines = false;
  draw_upg_box(player, AZ_UPG_RETRO_THRUSTERS, &has_engines);
  draw_upg_box(player, AZ_UPG_CPLUS_DRIVE, &has_engines);
  draw_upg_box(player, AZ_UPG_ORION_BOOSTER, &has_engines);
  if (has_engines) {
    draw_schematic_line(state->clock, engines_line_vertices,
                        AZ_ARRAY_SIZE(engines_line_vertices));
  }

  bool has_scan = false;
  draw_upg_box(player, AZ_UPG_INFRASCANNER, &has_scan);
  draw_upg_box(player, AZ_UPG_MILLIWAVE_RADAR, &has_scan);
  if (has_scan) {
    draw_schematic_line(state->clock, scan_line_vertices,
                        AZ_ARRAY_SIZE(scan_line_vertices));
  }

  glColor3f(1, 1, 0.5);
  draw_bezel_box(8, 40, 370, AZ_SCREEN_WIDTH - 80, 90);
  if (state->hovering == AZ_PAUSE_HOVER_UPGRADE) {
    glPushMatrix(); {
      glTranslatef(73.5, 410.5, 0);
      glScalef(1, -1, 1);
      az_draw_upgrade_icon(state->hovered_upgrade, state->clock);
      glBegin(GL_LINE_LOOP); {
        glColor4f(1, 1, 1, 0.75);
        glVertex2f(14, 14); glVertex2f(-14, 14);
        glVertex2f(-14, -14); glVertex2f(14, -14);
      } glEnd();
    } glPopMatrix();
    glColor3f(1, 0.5, 0.5);
    az_draw_printf(8, AZ_ALIGN_LEFT, 106, 387, "%s:",
                   (state->hovered_upgrade == AZ_UPG_SHIELD_BATTERY_00 ?
                    "SHIELDS" : state->hovered_upgrade == AZ_UPG_CAPACITOR_00 ?
                    "ENERGY" : az_upgrade_name(state->hovered_upgrade)));
    az_draw_paragraph(
        8, AZ_ALIGN_LEFT, 106, 409, 16, -1, state->prefs,
        (state->hovered_upgrade == AZ_UPG_SHIELD_BATTERY_00 ?
         "Absorbs damage to your ship, and can be refilled by save\n"
         "points, repair bays, or by collecting blue shield pickups." :
         state->hovered_upgrade == AZ_UPG_CAPACITOR_00 ?
         "Powers your primary weapons and other ship systems, and\n"
         "is automatically refilled over time by your reactor." :
         state->hovered_upgrade == AZ_UPG_ROCKET_AMMO_00 ?
         (az_upgrades_have_any_bombs(&player->upgrades) ?
          az_show_extra_weapon_key(state->prefs, 9) ?
          "Press [9] or [$9] to select rockets, then \n"
          "hold down [$o] and press [$f] to fire." :
          "Press [9] to select rockets, then hold down [$o] and\n"
          "press [$f] to fire." :
          "Hold down [$o] and press [$f] to fire.") :
         state->hovered_upgrade == AZ_UPG_BOMB_AMMO_00 ?
         (az_upgrades_have_any_rockets(&player->upgrades) ?
          az_show_extra_weapon_key(state->prefs, 0) ?
          "Press [0] or [$0] to select bombs,\n"
          "then hold down [$o] and press [$f] to drop." :
          "Press [0] to select bombs, then hold down [$o] and\n"
          "press [$f] to drop." :
          "Hold down [$o] and press [$f] to drop.") :
         az_upgrade_description(state->hovered_upgrade, &player->upgrades,
                                state->prefs)));
  } else if (state->hovering == AZ_PAUSE_HOVER_SHIP) {
    const az_ship_t ship = { .position = {75, 411}, .angle = AZ_DEG2RAD(135) };
    az_draw_ship_body(&ship, state->clock);
    glColor3f(1, 0.5, 0.5);
    az_draw_string(8, AZ_ALIGN_LEFT, 106, 387, "COBOLT MK. IX IPF GUNSHIP:");
    az_draw_paragraph(
        8, AZ_ALIGN_LEFT, 106, 409, 16, -1, state->prefs,
        "Fast, maneuverable, and highly adaptable, the Cobolt Mark IX\n"
        "is the mainstay of the UHP's Interstellar Patrol Force.");
  } else {
    az_draw_paragraph(
        8, AZ_ALIGN_LEFT, 106, 409, 16, -1, state->prefs,
        "Hover mouse cursor over any ship system for more info.");
  }
}

/*===========================================================================*/
// Options drawer:

#define OPTIONS_DRAWER_SLIDE_DISTANCE (AZ_PREFS_BOX_HEIGHT + 94)

static const az_vector_t options_drawer_vertices[] = {
  {12.5, -0.5}, {12.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 18.5},
  {194.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 18.5},
  {204.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 8.5},
  {435.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 8.5},
  {445.5, OPTIONS_DRAWER_SLIDE_DISTANCE - 1.5},
  {627.5, OPTIONS_DRAWER_SLIDE_DISTANCE - 1.5}, {627.5, -0.5}
};

#define OPTIONS_DRAWER_HANDLE_TOP (OPTIONS_DRAWER_SLIDE_DISTANCE - 2)
#define OPTIONS_DRAWER_HANDLE_LEFT 0
static const az_vector_t options_drawer_handle_vertices[] = {
  {12.5, 0.5}, {12.5, 20.5}, {194.5, 20.5}, {204.4, 10.5}, {194.5, 0.5}
};
static const az_polygon_t options_drawer_handle_polygon =
  AZ_INIT_POLYGON(options_drawer_handle_vertices);

#define PREFS_BOX_TOP 30
#define PREFS_BOX_LEFT ((AZ_SCREEN_WIDTH - AZ_PREFS_BOX_WIDTH) / 2)
#define PREFS_BOX_MARGIN 5

#define HINT_BUTTON_WIDTH 90
#define HINT_BUTTON_HEIGHT 20
#define HINT_BUTTON_LEFT 70
#define HINT_BUTTON_TOP \
  (PREFS_BOX_TOP + AZ_PREFS_BOX_HEIGHT + 2*PREFS_BOX_MARGIN + 12)

#define QUIT_BUTTON_WIDTH 120
#define QUIT_BUTTON_HEIGHT 20
#define QUIT_BUTTON_LEFT (AZ_SCREEN_WIDTH - 70 - QUIT_BUTTON_WIDTH)
#define QUIT_BUTTON_TOP \
  (PREFS_BOX_TOP + AZ_PREFS_BOX_HEIGHT + 2*PREFS_BOX_MARGIN + 12)

#define CONFIRM_CANCEL_SPACING 20
#define CONFIRM_BUTTON_LEFT \
  ((AZ_SCREEN_WIDTH - CONFIRM_CANCEL_SPACING) / 2 - QUIT_BUTTON_WIDTH)
#define CANCEL_BUTTON_LEFT ((AZ_SCREEN_WIDTH + CONFIRM_CANCEL_SPACING) / 2)
#define CONFIRM_CANCEL_TOP (PREFS_BOX_TOP + 120)

static const az_vector_t hint_button_vertices[] = {
  {HINT_BUTTON_WIDTH - 5.5, 0.5},
  {HINT_BUTTON_WIDTH - 0.5, 0.5 * HINT_BUTTON_HEIGHT},
  {HINT_BUTTON_WIDTH - 5.5, HINT_BUTTON_HEIGHT - 0.5},
  {5.5, HINT_BUTTON_HEIGHT - 0.5}, {0.5, 0.5 * HINT_BUTTON_HEIGHT}, {5.5, 0.5}
};
static const az_polygon_t hint_button_polygon =
  AZ_INIT_POLYGON(hint_button_vertices);

static const az_vector_t quit_button_vertices[] = {
  {QUIT_BUTTON_WIDTH - 5.5, 0.5},
  {QUIT_BUTTON_WIDTH - 0.5, 0.5 * QUIT_BUTTON_HEIGHT},
  {QUIT_BUTTON_WIDTH - 5.5, QUIT_BUTTON_HEIGHT - 0.5},
  {5.5, QUIT_BUTTON_HEIGHT - 0.5}, {0.5, 0.5 * QUIT_BUTTON_HEIGHT}, {5.5, 0.5}
};
static const az_polygon_t quit_button_polygon =
  AZ_INIT_POLYGON(quit_button_vertices);

static void draw_prefs(const az_paused_state_t *state) {
  glColor4f(0, 0, 0, 0.92); // black tint
  glBegin(GL_TRIANGLE_FAN); {
    AZ_ARRAY_LOOP(vertex, options_drawer_vertices) az_gl_vertex(*vertex);
  } glEnd();
  glColor3f(0.75, 0.75, 0.75); // light gray
  glBegin(GL_LINE_LOOP); {
    AZ_ARRAY_LOOP(vertex, options_drawer_vertices) az_gl_vertex(*vertex);
  } glEnd();

  az_draw_borderless_button(&state->options_drawer_handle);
  if (az_clock_mod(2, 40, state->clock)) glColor3f(0.5, 1, 0.5);
  else glColor3f(0, 1, 0);
  if (state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 105, OPTIONS_DRAWER_SLIDE_DISTANCE + 5,
                   "\x11 MAP [%s] \x11",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_ORDN]));
  } else {
    az_draw_printf(8, AZ_ALIGN_CENTER, 105, OPTIONS_DRAWER_SLIDE_DISTANCE + 5,
                   "\x12 OPTIONS [%s] \x12",
                   az_key_name(state->prefs->key_for_control[AZ_CONTROL_UTIL]));
  }

  glColor3f(0.25, 0.25, 0.25);
  draw_bezel_box(2, PREFS_BOX_LEFT - PREFS_BOX_MARGIN,
                 PREFS_BOX_TOP - PREFS_BOX_MARGIN,
                 AZ_PREFS_BOX_WIDTH + 2 * PREFS_BOX_MARGIN,
                 AZ_PREFS_BOX_HEIGHT + 2 * PREFS_BOX_MARGIN);
  if (state->confirming_quit) {
    glColor3f(1, 1, 1); // white
    az_draw_string(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, PREFS_BOX_TOP + 58,
                   "Quit to the main menu?");
    az_draw_string(8, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2, PREFS_BOX_TOP + 74,
                   "Progress will not be saved.");
    if (state->confirm_button.hovering) {
      glColor3f(1, state->confirm_button.hover_pulse, 0);
    } else glColor3f(0.3, 0, 0);
    az_draw_string(8, AZ_ALIGN_CENTER,
                   CONFIRM_BUTTON_LEFT + QUIT_BUTTON_WIDTH/2,
                   CONFIRM_CANCEL_TOP + QUIT_BUTTON_HEIGHT/2 - 4, "QUIT");
    if (state->cancel_button.hovering) {
      glColor3f(state->cancel_button.hover_pulse, 1, 0);
    } else glColor3f(0, 0.3, 0);
    az_draw_string(8, AZ_ALIGN_CENTER,
                   CANCEL_BUTTON_LEFT + QUIT_BUTTON_WIDTH/2,
                   CONFIRM_CANCEL_TOP + QUIT_BUTTON_HEIGHT/2 - 4, "CANCEL");
  } else az_draw_prefs_pane(&state->prefs_pane);

  if (state->prefs->enable_hints) {
    az_draw_standard_button(&state->hint_button);
    if (state->confirming_quit) glColor3f(0.25, 0.25, 0.25); // dark gray
    else glColor3f(0.5, 0.5, 1); // bluish
    az_draw_string(8, AZ_ALIGN_CENTER, HINT_BUTTON_LEFT + HINT_BUTTON_WIDTH/2,
                   HINT_BUTTON_TOP + HINT_BUTTON_HEIGHT/2 - 4, "GET HINT");
  }

  az_draw_dangerous_button(&state->quit_button);
  if (state->confirming_quit) glColor3f(0.25, 0.25, 0.25); // dark gray
  else glColor3f(0.75, 0, 0); // red
  az_draw_string(8, AZ_ALIGN_CENTER, QUIT_BUTTON_LEFT + QUIT_BUTTON_WIDTH/2,
                 QUIT_BUTTON_TOP + QUIT_BUTTON_HEIGHT/2 - 4, "QUIT TO MENU");
}

/*===========================================================================*/

// How long it takes to fade out the screen when quitting to main manu:
#define QUIT_FADE_TIME 1.0

void az_init_paused_state(
    az_paused_state_t *state, const az_planet_t *planet,
    const az_preferences_t *prefs, az_ship_t *ship) {
  assert(state != NULL);
  assert(planet != NULL);
  assert(prefs != NULL);
  assert(ship != NULL);
  AZ_ZERO_OBJECT(state);
  state->planet = planet;
  state->prefs = prefs;
  state->ship = ship;
  const az_player_t *player = &ship->player;
  double y_min = SCROLL_Y_MAX;
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    if (az_test_room_mapped(player, i, room)) {
      y_min = fmin(y_min, az_room_center(room).y + 140 * MINIMAP_ZOOM);
    }
  }
  const az_room_key_t current_room_key = player->current_room;
  assert(current_room_key < planet->num_rooms);
  const az_room_t *current_room = &planet->rooms[current_room_key];
  const double current_y = az_room_center(current_room).y;
  state->scroll_y = fmin(fmax(y_min, current_y), SCROLL_Y_MAX);
  state->scroll_y_min = y_min;
  az_init_prefs_pane(&state->prefs_pane, PREFS_BOX_LEFT, PREFS_BOX_TOP, prefs);
  az_init_button(&state->options_drawer_handle, options_drawer_handle_polygon,
                 OPTIONS_DRAWER_HANDLE_LEFT, OPTIONS_DRAWER_HANDLE_TOP);
  az_init_button(&state->upgrade_drawer_handle,
                 upgrade_drawer_handle_polygon,
                 UPGRADE_DRAWER_HANDLE_LEFT, UPGRADE_DRAWER_HANDLE_TOP);
  az_init_button(&state->hint_button, hint_button_polygon,
                 HINT_BUTTON_LEFT, HINT_BUTTON_TOP);
  az_init_button(&state->quit_button, quit_button_polygon,
                 QUIT_BUTTON_LEFT, QUIT_BUTTON_TOP);
  az_init_button(&state->confirm_button, quit_button_polygon,
                 CONFIRM_BUTTON_LEFT, CONFIRM_CANCEL_TOP);
  az_init_button(&state->cancel_button, quit_button_polygon,
                 CANCEL_BUTTON_LEFT, CONFIRM_CANCEL_TOP);
}

void az_paused_draw_screen(const az_paused_state_t *state) {
  draw_minimap(state);
  glPushMatrix(); {
    if (state->drawer_slide < 0.0) {
      glTranslated(0, UPGRADE_DRAWER_SLIDE_DISTANCE +
                   OPTIONS_DRAWER_SLIDE_DISTANCE * -state->drawer_slide, 0);
    } else {
      glTranslated(0, UPGRADE_DRAWER_SLIDE_DISTANCE *
                   (1 - state->drawer_slide), 0);
    }
    draw_upgrades(state);
    glTranslated(0, -(OPTIONS_DRAWER_SLIDE_DISTANCE +
                      UPGRADE_DRAWER_SLIDE_DISTANCE), 0);
    draw_prefs(state);
  } glPopMatrix();
  az_draw_cursor();
  if (state->do_quit) {
    glColor4f(0, 0, 0, state->quitting_fade_alpha);
    glBegin(GL_TRIANGLE_FAN); {
      glVertex2i(0, 0);
      glVertex2i(AZ_SCREEN_WIDTH, 0);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
    } glEnd();
  } else assert(state->quitting_fade_alpha == 0.0);
}

void az_tick_paused_state(az_paused_state_t *state, double time) {
  ++state->clock;

  if (state->ship->player.shields <= AZ_SHIELDS_LOW_THRESHOLD) {
    az_loop_sound(&state->soundboard,
                  (state->ship->player.shields >
                   AZ_SHIELDS_VERY_LOW_THRESHOLD ?
                   AZ_SND_KLAXON_SHIELDS_LOW :
                   AZ_SND_KLAXON_SHIELDS_VERY_LOW));
  }

  if (state->do_quit) {
    state->quitting_fade_alpha =
      fmin(1.0, state->quitting_fade_alpha + time / QUIT_FADE_TIME);
  }

  const double slide_by = time / 0.3;
  switch (state->current_drawer) {
    case AZ_PAUSE_DRAWER_MAP:
      if (state->drawer_slide < 0.0) {
        state->drawer_slide = fmin(0.0, state->drawer_slide + slide_by);
      } else {
        state->drawer_slide = fmax(0.0, state->drawer_slide - slide_by);
      }
      break;
    case AZ_PAUSE_DRAWER_OPTIONS:
      state->drawer_slide = fmax(-1.0, state->drawer_slide - slide_by);
      break;
    case AZ_PAUSE_DRAWER_UPGRADES:
      state->drawer_slide = fmin(1.0, state->drawer_slide + slide_by);
      break;
  }
  if (state->drawer_slide >= 0.0) {
    state->confirming_quit = false;
  }
  if (state->drawer_slide <= 0.0) {
    state->hovering = AZ_PAUSE_HOVER_NOTHING;
  }

  if (state->hint.active && state->hint.progress < 1.0) {
    state->hint.progress = fmin(1.0, state->hint.progress + time / 0.9);
    const double goal_y =
      az_room_center(&state->planet->rooms[state->hint.target_room]).y;
    const double scroll_delta = 1.5 * SCROLL_SPEED * time;
    if (state->scroll_y < goal_y) {
      state->scroll_y = fmin(state->scroll_y + scroll_delta, goal_y);
    } else {
      state->scroll_y = fmax(state->scroll_y - scroll_delta, goal_y);
    }
  } else if (state->current_drawer == AZ_PAUSE_DRAWER_MAP) {
    const az_key_id_t *key_for_control = state->prefs->key_for_control;
    const bool up = az_is_key_held(key_for_control[AZ_CONTROL_UP]);
    const bool down = az_is_key_held(key_for_control[AZ_CONTROL_DOWN]);
    if (up && !down) {
      state->scroll_y += time * SCROLL_SPEED;
    } else if (down && !up) {
      state->scroll_y -= time * SCROLL_SPEED;
    }
  }
  state->scroll_y =
    fmin(fmax(state->scroll_y_min, state->scroll_y), SCROLL_Y_MAX);

  az_tick_button(&state->options_drawer_handle, 0,
                 -OPTIONS_DRAWER_SLIDE_DISTANCE * (1.0 + state->drawer_slide),
                 true, time, state->clock, &state->soundboard);
  az_tick_button(&state->upgrade_drawer_handle, 0,
                 UPGRADE_DRAWER_SLIDE_DISTANCE * (1.0 - state->drawer_slide),
                 true, time, state->clock, &state->soundboard);
  const bool options_active =
    state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS &&
    state->drawer_slide == -1.0 && !state->do_quit;
  const bool prefs_active = options_active && !state->confirming_quit;
  az_tick_prefs_pane(&state->prefs_pane, prefs_active, time, state->clock,
                     &state->soundboard);
  az_tick_button(&state->hint_button, 0, 0,
                 (prefs_active && state->prefs->enable_hints), time,
                 state->clock, &state->soundboard);
  az_tick_button(&state->quit_button, 0, 0, prefs_active, time,
                 state->clock, &state->soundboard);
  const bool confirm_cancel_active = options_active && state->confirming_quit;
  az_tick_button(&state->confirm_button, 0, 0, confirm_cancel_active, time,
                 state->clock, &state->soundboard);
  az_tick_button(&state->cancel_button, 0, 0, confirm_cancel_active, time,
                 state->clock, &state->soundboard);
}

static bool maybe_hover_upgrade(az_paused_state_t *state, int x, int y,
                                int left, int top, int width, int height,
                                az_upgrade_t upgrade) {
  if (x >= left && x <= left + width && y >= top && y <= top + height) {
    state->hovering = AZ_PAUSE_HOVER_UPGRADE;
    state->hovered_upgrade = upgrade;
    return true;
  } else return false;
}

void az_paused_on_hover(az_paused_state_t *state, int x, int y) {
  if (state->current_drawer != AZ_PAUSE_DRAWER_UPGRADES ||
      state->drawer_slide < 1.0) return;
  const az_player_t *player = &state->ship->player;
  state->hovering = AZ_PAUSE_HOVER_NOTHING;
  if (235 <= x && x <= 405 && 120 <= y && y <= 325) {
    state->hovering = AZ_PAUSE_HOVER_SHIP;
  }
  for (int i = 0; i < AZ_ARRAY_SIZE(upgrade_toplefts); ++i) {
    const az_upgrade_t upgrade = (az_upgrade_t)i;
    if (!az_has_upgrade(player, upgrade)) continue;
    const az_vector_t topleft = upgrade_toplefts[i];
    if (maybe_hover_upgrade(state, x, y, topleft.x, topleft.y,
                            (upgrade <= AZ_UPG_GUN_BEAM ?
                             GUN_BOX_WIDTH : UPG_BOX_WIDTH),
                            UPG_BOX_HEIGHT, upgrade)) break;
  }
  maybe_hover_upgrade(state, x, y, TANK_BOX_LEFT, SHIELD_BOX_TOP,
                      UPG_BOX_WIDTH, UPG_BOX_HEIGHT, AZ_UPG_SHIELD_BATTERY_00);
  maybe_hover_upgrade(state, x, y, TANK_BOX_LEFT, ENERGY_BOX_TOP,
                      UPG_BOX_WIDTH, UPG_BOX_HEIGHT, AZ_UPG_CAPACITOR_00);
  if (state->ship->player.max_rockets > 0) {
    maybe_hover_upgrade(state, x, y, ORDN_BOX_LEFT, ROCKETS_BOX_TOP,
                        UPG_BOX_WIDTH, UPG_BOX_HEIGHT, AZ_UPG_ROCKET_AMMO_00);
  }
  if (state->ship->player.max_bombs > 0) {
    maybe_hover_upgrade(state, x, y, ORDN_BOX_LEFT, BOMBS_BOX_TOP,
                        UPG_BOX_WIDTH, UPG_BOX_HEIGHT, AZ_UPG_BOMB_AMMO_00);
  }
}

void az_paused_on_click(az_paused_state_t *state, int x, int y) {
  if (state->do_quit) return;
  if (az_button_on_click(&state->options_drawer_handle, x, y +
                         OPTIONS_DRAWER_SLIDE_DISTANCE *
                         (1.0 + state->drawer_slide),
                         &state->soundboard)) {
    state->current_drawer =
      (state->current_drawer != AZ_PAUSE_DRAWER_OPTIONS ?
       AZ_PAUSE_DRAWER_OPTIONS : AZ_PAUSE_DRAWER_MAP);
  } else if (az_button_on_click(&state->upgrade_drawer_handle, x, y -
                                UPGRADE_DRAWER_SLIDE_DISTANCE *
                                (1.0 - state->drawer_slide),
                                &state->soundboard)) {
    state->current_drawer =
      (state->current_drawer != AZ_PAUSE_DRAWER_UPGRADES ?
       AZ_PAUSE_DRAWER_UPGRADES : AZ_PAUSE_DRAWER_MAP);
  } else if (state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS &&
             state->drawer_slide == -1.0) {
    if (state->confirming_quit) {
      if (az_button_on_click(&state->confirm_button, x, y,
                             &state->soundboard)) {
        state->do_quit = true;
        az_stop_music(&state->soundboard, QUIT_FADE_TIME);
      }
      if (az_button_on_click(&state->cancel_button, x, y,
                             &state->soundboard)) {
        state->confirming_quit = false;
      }
    } else {
      az_prefs_pane_on_click(&state->prefs_pane, x, y,
                             &state->soundboard);
      if (state->prefs->enable_hints &&
          az_button_on_click(&state->hint_button, x, y, &state->soundboard)) {
        const az_hint_t *hint =
          az_get_hint(state->planet, &state->ship->player);
        state->hint.active = true;
        state->hint.progress = 0.0;
        state->hint.target_room = (hint == NULL ? state->planet->start_room :
                                   hint->target_room);
        state->current_drawer = AZ_PAUSE_DRAWER_MAP;
      }
      if (az_button_on_click(&state->quit_button, x, y, &state->soundboard)) {
        state->confirming_quit = true;
      }
    }
  } else if (state->drawer_slide == 0.0 || state->drawer_slide == 1.0) {
    const bool down = (state->drawer_slide == 0.0);
#ifndef NDEBUG
    const bool cheat = az_is_shift_key_held() && az_is_key_held(AZ_KEY_TAB);
    if (cheat) {
      for (int i = 0; i < 2; ++i) {
        int top = (i == 0 ? SHIELD_BOX_TOP : ENERGY_BOX_TOP);
        if (down) top += UPGRADE_DRAWER_SLIDE_DISTANCE;
        if (x >= TANK_BOX_LEFT && x <= TANK_BOX_LEFT + UPG_BOX_WIDTH &&
            y >= top && y <= top + UPG_BOX_HEIGHT) {
          const az_upgrade_t first =
            (i == 0 ? AZ_UPG_SHIELD_BATTERY_00 : AZ_UPG_CAPACITOR_00);
          const az_upgrade_t last =
            (i == 0 ? AZ_UPG_SHIELD_BATTERY_MAX : AZ_UPG_CAPACITOR_MAX);
          for (az_upgrade_t upgrade = first; upgrade <= last; ++upgrade) {
            if (!az_has_upgrade(&state->ship->player, upgrade)) {
              az_give_upgrade(&state->ship->player, upgrade);
              break;
            }
          }
        }
      }
      for (int i = 0; i < AZ_ARRAY_SIZE(upgrade_toplefts); ++i) {
        const az_upgrade_t upgrade = (az_upgrade_t)i;
        if (az_has_upgrade(&state->ship->player, upgrade)) continue;
        az_vector_t topleft = upgrade_toplefts[i];
        if (down) topleft.y += UPGRADE_DRAWER_SLIDE_DISTANCE;
        const double right = topleft.x +
          (upgrade <= AZ_UPG_GUN_BEAM ? GUN_BOX_WIDTH : UPG_BOX_WIDTH);
        const double bottom = topleft.y + UPG_BOX_HEIGHT;
        if (x >= topleft.x && x <= right && y >= topleft.y && y <= bottom) {
          az_give_upgrade(&state->ship->player, upgrade);
          break;
        }
      }
    }
#endif
    for (int i = 0; i < 8; ++i) {
      az_vector_t topleft = upgrade_toplefts[AZ_UPG_GUN_CHARGE + i];
      if (down) topleft.y += UPGRADE_DRAWER_SLIDE_DISTANCE;
      if (x >= topleft.x && x <= topleft.x + GUN_BOX_WIDTH &&
          y >= topleft.y && y <= topleft.y + UPG_BOX_HEIGHT) {
        az_select_gun(&state->ship->player, AZ_GUN_CHARGE + i);
        break;
      }
    }
    for (int i = 0; i < 2; ++i) {
      int top = (i == 0 ? ROCKETS_BOX_TOP : BOMBS_BOX_TOP);
      if (down) top += UPGRADE_DRAWER_SLIDE_DISTANCE;
      if (x >= ORDN_BOX_LEFT && x <= ORDN_BOX_LEFT + UPG_BOX_WIDTH &&
          y >= top && y <= top + UPG_BOX_HEIGHT) {
#ifndef NDEBUG
        if (cheat) {
          const az_upgrade_t first =
            (i == 0 ? AZ_UPG_ROCKET_AMMO_00 : AZ_UPG_BOMB_AMMO_00);
          const az_upgrade_t last =
            (i == 0 ? AZ_UPG_ROCKET_AMMO_MAX : AZ_UPG_BOMB_AMMO_MAX);
          for (az_upgrade_t upgrade = first; upgrade <= last; ++upgrade) {
            if (!az_has_upgrade(&state->ship->player, upgrade)) {
              az_give_upgrade(&state->ship->player, upgrade);
              break;
            }
          }
        }
#endif
        az_select_ordnance(&state->ship->player, AZ_ORDN_ROCKETS + i);
        break;
      }
    }
  }
}

/*===========================================================================*/
