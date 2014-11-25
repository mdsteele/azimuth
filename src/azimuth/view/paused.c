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
#include "azimuth/view/hud.h"
#include "azimuth/view/node.h"
#include "azimuth/view/prefs.h"
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

/*===========================================================================*/
// Minimap geometry:

#define MINIMAP_ZOOM 75.0
#define SCROLL_Y_MAX (AZ_PLANETOID_RADIUS - 180 * MINIMAP_ZOOM)
#define SCROLL_SPEED (AZ_PLANETOID_RADIUS)

static bool test_room_mapped(const az_player_t *player,
                             const az_room_t *room) {
  return (!(room->properties & AZ_ROOMF_UNMAPPED) &&
          az_test_zone_mapped(player, room->zone_key));
}

static az_vector_t get_room_center(const az_room_t *room) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  return (bounds->theta_span >= 6.28 && bounds->min_r < AZ_SCREEN_HEIGHT ?
          AZ_VZERO : az_bounds_center(bounds));
}

/*===========================================================================*/
// Drawing minimap:

static void draw_map_marker(const az_room_t *room, az_clock_t clock) {
  const az_vector_t center = get_room_center(room);
  glPushMatrix(); {
    glTranslated(center.x, center.y, 0);
    glRotatef(3 * az_clock_mod(120, 1, clock), 0, 0, 1);
    glScalef(MINIMAP_ZOOM, MINIMAP_ZOOM, 0);
    for (int i = 0; i < 4; ++i) {
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0, 0); glVertex2f(4, 0); glColor3f(1, 1, 1);
        glVertex2f(5, -4); glVertex2f(1, 0); glVertex2f(5, 4);
      } glEnd();
      glRotated(90, 0, 0, 1);
    }
  } glPopMatrix();
}

static void begin_map_label(az_color_t color, const az_room_t *room) {
  const az_vector_t center = get_room_center(room);
  glPushMatrix();
  glTranslated(center.x, center.y, 0);
  glScalef(MINIMAP_ZOOM, MINIMAP_ZOOM, 0);
  az_gl_color(color);
  glBegin(GL_QUADS); {
    glVertex2f(4, 4); glVertex2f(-4, 4); glVertex2f(-4, -4); glVertex2f(4, -4);
  } glEnd();
  glColor3f(0, 0, 0);
  glBegin(GL_LINE_STRIP);
}

static void end_map_label(void) {
  glEnd();
  glPopMatrix();
}

static void draw_minimap_rooms(const az_paused_state_t *state) {
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
    const bool visited = az_test_room_visited(player, i);
    if (!visited && !test_room_mapped(player, room)) continue;
    az_draw_minimap_room(planet, room, visited, false);
  }

  // Draw map labels:
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    const bool visited = az_test_room_visited(player, i);
    if (!visited && !test_room_mapped(player, room)) continue;

    // Draw a console label (if any).  We mark all save rooms on the map, but
    // only mark refill/comm rooms if we've actually visited them.
    if (az_clock_mod(2, 30, state->clock) == 0) {
      if (room->properties & AZ_ROOMF_WITH_SAVE) {
        begin_map_label((az_color_t){128, 255, 128, 255}, room); {
          glVertex2f(2, 2); glVertex2f(-2, 2); glVertex2f(-2, 0);
          glVertex2f(2, 0); glVertex2f(2, -2); glVertex2f(-2, -2);
        } end_map_label();
      } else if (visited) {
        if (room->properties & AZ_ROOMF_WITH_REFILL) {
          begin_map_label((az_color_t){255, 255, 128, 255}, room); {
            glVertex2f(-2, -2); glVertex2f(-2, 2); glVertex2f(2, 2);
            glVertex2f(2, 0); glVertex2f(-2, 0); glVertex2f(2, -2);
          } end_map_label();
        } else if (room->properties & AZ_ROOMF_WITH_COMM) {
          begin_map_label((az_color_t){128, 128, 255, 255}, room); {
            glVertex2f(2, 2); glVertex2f(-2, 2);
            glVertex2f(-2, -2); glVertex2f(2, -2);
          } end_map_label();
        }
      }
    }
  }

  // Draw map markers:
  for (int i = 0; i < planet->num_rooms; ++i) {
    const az_room_t *room = &planet->rooms[i];
    if ((room->properties & AZ_ROOMF_MARK_IF_SET) &&
        az_test_flag(player, room->marker_flag)) {
      draw_map_marker(room, state->clock);
    } else if ((room->properties & AZ_ROOMF_MARK_IF_CLR) &&
               !az_test_flag(player, room->marker_flag)) {
      if (az_test_room_visited(player, i) || test_room_mapped(player, room)) {
        draw_map_marker(room, state->clock);
      }
    }
  }

  // Draw a marker for the current ship position:
  if (az_clock_mod(2, 12, state->clock) == 0) {
    glPushMatrix(); {
      glTranslated(ship->position.x, ship->position.y, 0);
      glRotated(AZ_RAD2DEG(ship->angle), 0, 0, 1);
      glBegin(GL_LINE_LOOP); {
        glColor3f(0, 1, 1);
        glVertex2f(700, 0); glVertex2f(100, 100);
        glVertex2f(0, 700); glVertex2f(-100, 100);
        glVertex2f(-700, 0); glVertex2f(-100, -100);
        glVertex2f(0, -700); glVertex2f(100, -100);
      } glEnd();
    } glPopMatrix();
  }
}

static void draw_minimap(const az_paused_state_t *state) {
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
      draw_minimap_rooms(state);
    } glPopMatrix();
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
  const az_color_t color = current_zone->color;
  glColor3ub(color.r, color.g, color.b);
  az_draw_string(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 38, 17,
                 current_zone->name);
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

#define GUN_BOX_WIDTH 60
#define UPG_BOX_WIDTH 150
#define UPG_BOX_HEIGHT 15

#define TANK_BOX_LEFT 24
#define SHIELD_BOX_TOP 26
#define ENERGY_BOX_TOP 46

#define ORDN_BOX_LEFT 466
#define ROCKETS_BOX_TOP 26
#define BOMBS_BOX_TOP 46

static const az_vector_t upgrade_toplefts[] = {
  [AZ_UPG_GUN_CHARGE] = {191, 26},
  [AZ_UPG_GUN_FREEZE] = {257, 26},
  [AZ_UPG_GUN_TRIPLE] = {323, 26},
  [AZ_UPG_GUN_HOMING] = {389, 26},
  [AZ_UPG_GUN_PHASE] = {191, 46},
  [AZ_UPG_GUN_BURST] = {257, 46},
  [AZ_UPG_GUN_PIERCE] = {323, 46},
  [AZ_UPG_GUN_BEAM] = {389, 46},
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

static void set_weapon_box_color(bool has, bool selected) {
  assert(has || !selected);
  if (selected) glColor3f(1, 1, 1);
  else if (has) glColor3f(1, 0, 1);
  else glColor3f(0.5, 0, 0.5);
}

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

static const az_vector_t upgrade_drawer_vertices[] = {
  {12.5, 480.5}, {12.5, 20.5}, {22.5, 10.5}, {202.5, 10.5}, {206.5, 14.5},
  {421.5, 14.5}, {432.5, 3.5}, {627.5, 3.5}, {627.5, 480.5}
};

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

  if (az_clock_mod(2, 40, state->clock)) glColor3f(0.5, 1, 0.5);
  else glColor3f(0, 1, 0);
  if (state->current_drawer == AZ_PAUSE_DRAWER_UPGRADES) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 525, 10, "\x12 MAP [%s] \x12",
                   az_key_name(state->prefs->keys[AZ_PREFS_ORDN_KEY_INDEX]));
  } else {
    az_draw_printf(8, AZ_ALIGN_CENTER, 525, 10, "\x11 UPGRADES [%s] \x11",
                   az_key_name(state->prefs->keys[AZ_PREFS_FIRE_KEY_INDEX]));
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

  bool has_guns = false;
  for (int i = 0; i < 8; ++i) {
    const az_gun_t gun = AZ_GUN_CHARGE + i;
    const az_upgrade_t upgrade = AZ_UPG_GUN_CHARGE + i;
    const az_vector_t topleft = upgrade_toplefts[upgrade];
    const bool has_gun = az_has_upgrade(player, upgrade);
    set_weapon_box_color(has_gun, player->gun1 == gun || player->gun2 == gun);
    draw_bezel_box(2, topleft.x, topleft.y, GUN_BOX_WIDTH, UPG_BOX_HEIGHT);
    if (has_gun) {
      az_draw_string(8, AZ_ALIGN_CENTER, topleft.x + GUN_BOX_WIDTH/2,
                     topleft.y + 4, az_gun_name(gun));
      has_guns = true;
    }
  }

  set_weapon_box_color(player->max_rockets > 0,
                       player->ordnance == AZ_ORDN_ROCKETS);
  draw_bezel_box(2, ORDN_BOX_LEFT, ROCKETS_BOX_TOP,
                 UPG_BOX_WIDTH, UPG_BOX_HEIGHT);
  if (player->max_rockets > 0) {
    az_draw_printf(8, AZ_ALIGN_CENTER, ORDN_BOX_LEFT + UPG_BOX_WIDTH/2,
                   ROCKETS_BOX_TOP + 4, "ROCKETS:%3d/%-3d",
                   player->rockets, player->max_rockets);
  }
  set_weapon_box_color(player->max_bombs > 0,
                       player->ordnance == AZ_ORDN_BOMBS);
  draw_bezel_box(2, ORDN_BOX_LEFT, BOMBS_BOX_TOP,
                 UPG_BOX_WIDTH, UPG_BOX_HEIGHT);
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

  if (has_guns) {
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
  draw_bezel_box(8, 40, 370, AZ_SCREEN_WIDTH - 80, 82);
  if (state->hovering_over_upgrade) {
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
                   az_upgrade_name(state->hovered_upgrade));
    az_draw_paragraph(
        8, AZ_ALIGN_LEFT, 106, 409, 16, -1, state->prefs,
        (state->hovered_upgrade == AZ_UPG_ROCKET_AMMO_00 ?
         "Press [9] to select, hold [$o] and press [$f] to fire." :
         state->hovered_upgrade == AZ_UPG_BOMB_AMMO_00 ?
         "Press [0] to select, hold [$o] and press [$f] to drop." :
         az_upgrade_description(state->hovered_upgrade)));
  }
}

/*===========================================================================*/
// Options drawer:

#define OPTIONS_DRAWER_SLIDE_DISTANCE 280

static const az_vector_t options_drawer_vertices[] = {
  {12.5, -0.5}, {12.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 18.5},
  {194.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 18.5},
  {204.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 8.5},
  {435.5, OPTIONS_DRAWER_SLIDE_DISTANCE + 8.5},
  {445.5, OPTIONS_DRAWER_SLIDE_DISTANCE - 1.5},
  {627.5, OPTIONS_DRAWER_SLIDE_DISTANCE - 1.5}, {627.5, -0.5}
};

#define PREFS_BOX_TOP 30
#define PREFS_BOX_LEFT ((AZ_SCREEN_WIDTH - AZ_PREFS_BOX_WIDTH) / 2)
#define PREFS_BOX_MARGIN 5

#define QUIT_BUTTON_WIDTH 120
#define QUIT_BUTTON_HEIGHT 20
#define QUIT_BUTTON_LEFT (570 - QUIT_BUTTON_WIDTH)
#define QUIT_BUTTON_TOP \
  (PREFS_BOX_TOP + AZ_PREFS_BOX_HEIGHT + 2*PREFS_BOX_MARGIN + 12)

#define CONFIRM_CANCEL_SPACING 20
#define CONFIRM_BUTTON_LEFT \
  ((AZ_SCREEN_WIDTH - CONFIRM_CANCEL_SPACING) / 2 - QUIT_BUTTON_WIDTH)
#define CANCEL_BUTTON_LEFT ((AZ_SCREEN_WIDTH + CONFIRM_CANCEL_SPACING) / 2)
#define CONFIRM_CANCEL_TOP (PREFS_BOX_TOP + 120)

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

  if (az_clock_mod(2, 40, state->clock)) glColor3f(0.5, 1, 0.5);
  else glColor3f(0, 1, 0);
  if (state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS) {
    az_draw_printf(8, AZ_ALIGN_CENTER, 105, OPTIONS_DRAWER_SLIDE_DISTANCE + 4,
                   "\x11 MAP [%s] \x11",
                   az_key_name(state->prefs->keys[AZ_PREFS_ORDN_KEY_INDEX]));
  } else {
    az_draw_printf(8, AZ_ALIGN_CENTER, 105, OPTIONS_DRAWER_SLIDE_DISTANCE + 4,
                   "\x12 OPTIONS [%s] \x12",
                   az_key_name(state->prefs->keys[AZ_PREFS_UTIL_KEY_INDEX]));
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
    if (test_room_mapped(player, room) || az_test_room_visited(player, i)) {
      y_min = fmin(y_min, get_room_center(room).y + 120 * MINIMAP_ZOOM);
    }
  }
  const az_room_key_t current_room_key = player->current_room;
  assert(current_room_key < planet->num_rooms);
  const az_room_t *current_room = &planet->rooms[current_room_key];
  const double current_y = get_room_center(current_room).y;
  state->scroll_y = fmin(fmax(y_min, current_y), SCROLL_Y_MAX);
  state->scroll_y_min = y_min;
  az_init_prefs_pane(&state->prefs_pane, PREFS_BOX_LEFT, PREFS_BOX_TOP, prefs);
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
    glBegin(GL_QUADS); {
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

  if (state->current_drawer == AZ_PAUSE_DRAWER_MAP) {
    const az_preferences_t *prefs = state->prefs;
    const bool up = az_is_key_held(prefs->keys[AZ_PREFS_UP_KEY_INDEX]);
    const bool down = az_is_key_held(prefs->keys[AZ_PREFS_DOWN_KEY_INDEX]);
    if (up && !down) {
      state->scroll_y += time * SCROLL_SPEED;
    } else if (down && !up) {
      state->scroll_y -= time * SCROLL_SPEED;
    }
    state->scroll_y =
      fmin(fmax(state->scroll_y_min, state->scroll_y), SCROLL_Y_MAX);
  }

  const bool options_active =
    state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS &&
    state->drawer_slide == -1.0 && !state->do_quit;
  const bool prefs_active = options_active && !state->confirming_quit;
  az_tick_prefs_pane(&state->prefs_pane, prefs_active, time, state->clock,
                     &state->soundboard);
  az_tick_button(&state->quit_button, 0, 0, prefs_active, time,
                 state->clock, &state->soundboard);
  const bool confirm_cancel_active = options_active && state->confirming_quit;
  az_tick_button(&state->confirm_button, 0, 0, confirm_cancel_active, time,
                 state->clock, &state->soundboard);
  az_tick_button(&state->cancel_button, 0, 0, confirm_cancel_active, time,
                 state->clock, &state->soundboard);
}

void az_paused_on_hover(az_paused_state_t *state, int x, int y) {
  if (state->current_drawer != AZ_PAUSE_DRAWER_UPGRADES ||
      state->drawer_slide < 1.0) return;
  const az_player_t *player = &state->ship->player;
  state->hovering_over_upgrade = false;
  for (int i = 0; i < AZ_ARRAY_SIZE(upgrade_toplefts); ++i) {
    const az_upgrade_t upgrade = (az_upgrade_t)i;
    if (!az_has_upgrade(player, upgrade)) continue;
    const az_vector_t topleft = upgrade_toplefts[i];
    const double right =
      topleft.x + (upgrade <= AZ_UPG_GUN_BEAM ? GUN_BOX_WIDTH : UPG_BOX_WIDTH);
    const double bottom = topleft.y + UPG_BOX_HEIGHT;
    if (x >= topleft.x && x <= right && y >= topleft.y && y <= bottom) {
      state->hovering_over_upgrade = true;
      state->hovered_upgrade = upgrade;
      break;
    }
  }
  for (int i = 0; i < 2; ++i) {
    if ((i == 0 ? player->max_rockets : player->max_bombs) == 0) continue;
    int top = (i == 0 ? ROCKETS_BOX_TOP : BOMBS_BOX_TOP);
    if (x >= ORDN_BOX_LEFT && x <= ORDN_BOX_LEFT + UPG_BOX_WIDTH &&
        y >= top && y <= top + UPG_BOX_HEIGHT) {
      state->hovering_over_upgrade = true;
      state->hovered_upgrade =
        (i == 0 ? AZ_UPG_ROCKET_AMMO_00 : AZ_UPG_BOMB_AMMO_00);
      break;
    }
  }
}

void az_paused_on_click(az_paused_state_t *state, int x, int y) {
  if (state->do_quit) return;
  if (state->current_drawer == AZ_PAUSE_DRAWER_OPTIONS &&
      state->drawer_slide == -1.0) {
    if (state->confirming_quit) {
      if (az_button_on_click(&state->confirm_button, x, y)) {
        state->do_quit = true;
        az_stop_music(&state->soundboard, QUIT_FADE_TIME);
      }
      if (az_button_on_click(&state->cancel_button, x, y)) {
        state->confirming_quit = false;
      }
    } else {
      az_prefs_pane_on_click(&state->prefs_pane, x, y);
      if (az_button_on_click(&state->quit_button, x, y)) {
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
