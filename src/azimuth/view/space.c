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

#include "azimuth/view/space.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/player.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h" // for AZ_ASSERT_UNREACHABLE
#include "azimuth/util/vector.h"
#include "azimuth/view/background.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/cutscene.h"
#include "azimuth/view/door.h"
#include "azimuth/view/gravfield.h"
#include "azimuth/view/hud.h"
#include "azimuth/view/node.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/pickup.h"
#include "azimuth/view/projectile.h"
#include "azimuth/view/ship.h"
#include "azimuth/view/speck.h"
#include "azimuth/view/util.h"
#include "azimuth/view/wall.h"

/*===========================================================================*/

static az_room_flags_t room_properties(const az_space_state_t *state) {
  const az_room_key_t current_room = state->ship.player.current_room;
  assert(0 <= current_room && current_room < state->planet->num_rooms);
  return state->planet->rooms[current_room].properties;
}

static double mode_fade_alpha(az_space_state_t *state) {
  switch (state->mode) {
    case AZ_MODE_DOORWAY:
      switch (state->doorway_mode.step) {
        case AZ_DWS_FADE_OUT: return state->doorway_mode.progress;
        case AZ_DWS_SHIFT:    return 1.0;
        case AZ_DWS_FADE_IN:  return 1.0 - state->doorway_mode.progress;
      }
      AZ_ASSERT_UNREACHABLE();
    case AZ_MODE_GAME_OVER:
      if (state->game_over_mode.step == AZ_GOS_FADE_OUT) {
        return state->game_over_mode.progress;
      }
      return 0.0;
    case AZ_MODE_PAUSING:
      return state->pausing_mode.fade_alpha;
    case AZ_MODE_NORMAL:
    case AZ_MODE_BOSS_DEATH:
    case AZ_MODE_CONSOLE:
    case AZ_MODE_UPGRADE:
      return 0.0;
  }
  AZ_ASSERT_UNREACHABLE();
}

static void tint_screen(GLfloat gray, GLfloat alpha) {
  glColor4f(gray, gray, gray, alpha);
  glBegin(GL_QUADS); {
    glVertex2i(0, 0);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
  } glEnd();
}

static void transform_to_camera_matrix(const az_space_state_t *state) {
  // Make positive Y be up instead of down.
  glScaled(1, -1, 1);
  // Center the screen on position (0, 0).
  glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
  // If we're in a superheated room, wobble the screen slightly (to simulate
  // heat refraction).
  if (room_properties(state) & AZ_ROOMF_HEATED) {
    const GLfloat xy = 0.002 * cos(state->camera.wobble_theta);
    const GLfloat yy = 1 + 0.005 * sin(state->camera.wobble_theta);
    GLfloat matrix[16] = {
      1, xy, 0, 0,
      0, yy, 0, 0,
      0,  0, 1, 0,
      0,  0, 0, 1};
    glMultMatrixf(matrix);
  }
  // If camera wobble is active (e.g. because of an NPS portal), wobble the
  // screen accordingly.
  if (state->camera.wobble_intensity > 0.0) {
    const GLfloat xy = 0.15 * state->camera.wobble_intensity *
      cos(3.0 * state->camera.wobble_theta);
    const GLfloat yx = 0.2 * state->camera.wobble_intensity *
      sin(2.0 * state->camera.wobble_theta);
    GLfloat matrix[16] = {
      1, xy, 0, 0,
      yx, 1, 0, 0,
      0,  0, 1, 0,
      0,  0, 0, 1};
    glMultMatrixf(matrix);
  }
  // Move the screen to the camera pose.
  glTranslated(0, -az_vnorm(state->camera.center), 0);
  glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera.center)), 0, 0, 1);
  // Apply camera shake to the screen position.
  const az_vector_t shake_offset =
    az_camera_shake_offset(&state->camera, state->clock);
  glTranslated(shake_offset.x, shake_offset.y, 0);
}

static void draw_darkness(az_space_state_t *state) {
  assert(state->darkness > 0.0);
  assert(state->darkness <= 1.0);
  const GLfloat blue = 0.11;
  const GLfloat alpha = state->darkness;
  if (az_has_upgrade(&state->ship.player, AZ_UPG_INFRASCANNER)) {
    const double radius = 100.0;
    const double spread = 250.0;
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0, 0, blue, 0);
      glVertex2f(0, 0);
      glColor4f(0, 0, blue, alpha);
      for (int i = 90; i <= 270; i += 10) {
        glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0, 0, blue, alpha);
      glVertex2f(0, radius); glVertex2f(1000, radius + spread);
      glColor4f(0, 0, blue, 0);
      glVertex2f(0, 0); glVertex2f(1000, 0);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0, 0, blue, alpha);
      glVertex2f(0, -radius); glVertex2f(1000, -radius - spread);
      glColor4f(0, 0, blue, 0);
      glVertex2f(0, 0); glVertex2f(1000, 0);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0, 0, blue, alpha);
      glVertex2f(1000, radius + spread);
      glVertex2f(1000, 1000);
      for (int i = 90; i <= 270; i += 10) {
        const double c = cos(AZ_DEG2RAD(i)), s = sin(AZ_DEG2RAD(i));
        glVertex2d(radius * c, radius * s);
        glVertex2d(1000 * c, 1000 * s);
      }
      glVertex2f(1000, -radius - spread);
      glVertex2f(1000, -1000);
    } glEnd();
  } else {
    const double radius = 80.0;
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0, 0, blue, 0);
      glVertex2f(0, 0);
      glColor4f(0, 0, blue, alpha);
      for (int i = 0; i <= 360; i += 10) {
        glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 10) {
        const double c = cos(AZ_DEG2RAD(i)), s = sin(AZ_DEG2RAD(i));
        glVertex2d(radius * c, radius * s);
        glVertex2d(1000 * c, 1000 * s);
      }
    } glEnd();
  }
}

static void draw_camera_view(az_space_state_t *state) {
  const az_room_t *room =
    &state->planet->rooms[state->ship.player.current_room];
  az_draw_background_pattern(
      room->background_pattern, &room->camera_bounds, state->camera.center,
      state->clock);
  az_draw_background_nodes(state);
  glPushMatrix(); {
    glLoadIdentity();
    tint_screen(0, 0.6);
  } glPopMatrix();
  az_draw_gravfields(state);
  az_draw_console_and_upgrade_nodes(state);
  az_draw_background_baddies(state);
  az_draw_walls(state);
  az_draw_tractor_nodes(state);
  az_draw_pickups(state);
  az_draw_projectiles(state);
  if (state->mode == AZ_MODE_BOSS_DEATH &&
      state->boss_death_mode.boss.kind != AZ_BAD_NOTHING) {
    az_draw_baddie(&state->boss_death_mode.boss, state->clock);
  }
  az_draw_foreground_baddies(state);
  az_draw_ship(state);
  az_draw_particles(state);
  az_draw_doors(state);
  az_draw_specks(state);
  az_draw_liquid(state);
  az_draw_foreground_nodes(state);
}

static void draw_doorway_transition(az_space_state_t *state) {
  assert(state->mode == AZ_MODE_DOORWAY);
  const az_doorway_mode_data_t *mode_data = &state->doorway_mode;
  if (mode_data->step == AZ_DWS_SHIFT) {
    az_draw_door_shift(mode_data->entrance.position, mode_data->entrance.angle,
                       mode_data->exit.position, mode_data->exit.angle,
                       mode_data->progress);
  } else {
    const az_uid_t uid = (mode_data->step == AZ_DWS_FADE_OUT ?
                          mode_data->entrance.uid : mode_data->exit.uid);
    az_door_t *door;
    if (az_lookup_door(state, uid, &door) && door->kind != AZ_DOOR_PASSAGE) {
      const double progress =
        (mode_data->step == AZ_DWS_FADE_OUT ? mode_data->progress :
         1.0 - mode_data->progress);
      az_draw_door_pipe_fade(door->position, door->angle, pow(progress, 0.75));
    }
  }
}

static void draw_global_fade(const az_space_state_t *state) {
  const az_global_fade_state_t *fade = &state->global_fade;
  if (fade->step != AZ_GFS_INACTIVE) {
    assert(fade->fade_alpha >= 0.0);
    assert(fade->fade_alpha <= 1.0);
    tint_screen(fade->fade_gray, fade->fade_alpha);
  } else {
    assert(fade->fade_alpha == 0.0);
  }
}

void az_space_draw_screen(az_space_state_t *state) {
  // If we're watching a cutscene, draw that instead of our normal camera view.
  if (state->cutscene.scene != AZ_SCENE_NOTHING) {
    az_draw_cutscene(state);
    az_draw_dialogue(state);
    az_draw_monologue(state);
    draw_global_fade(state);
    return;
  }

  // Check if we're in a mode where we should be tinting the camera view black.
  const double fade_alpha = mode_fade_alpha(state);
  assert(fade_alpha >= 0.0);
  assert(fade_alpha <= 1.0);

  if (fade_alpha < 1.0) {
    // Draw what the camera sees.
    glPushMatrix(); {
      transform_to_camera_matrix(state);
      draw_camera_view(state);
    } glPopMatrix();

    // If we're in a superheated room, make everything glow red.
    const az_room_flags_t properties = room_properties(state);
    if (properties & AZ_ROOMF_HEATED) {
      glBegin(GL_QUADS); {
        const GLfloat alpha1 = 0.1 + 0.03 * cos(state->camera.wobble_theta);
        const GLfloat alpha2 = 0.1 + 0.03 * sin(state->camera.wobble_theta);
        glColor4f(1, 0, 0, alpha1);
        glVertex2i(0, 0);
        glColor4f(1, 0, 0, alpha2);
        glVertex2i(0, AZ_SCREEN_HEIGHT);
        glColor4f(1, 0, 0, alpha1);
        glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
        glColor4f(1, 0, 0, alpha2);
        glVertex2i(AZ_SCREEN_WIDTH, 0);
      } glEnd();
    }

    // If the room is darkened, draw darkness around the ship.
    if (state->darkness > 0.0) {
      glPushMatrix(); {
        transform_to_camera_matrix(state);
        az_gl_translated(state->ship.position);
        az_gl_rotated(state->ship.angle);
        draw_darkness(state);
      } glPopMatrix();
    }
  }

  // Tint the camera view black (based on fade_alpha).
  if (fade_alpha > 0.0) tint_screen(0, fade_alpha);

  // If we're in boss-death mode, draw the explosion.
  if (state->mode == AZ_MODE_BOSS_DEATH) {
    assert(state->boss_death_mode.progress >= 0.0);
    assert(state->boss_death_mode.progress <= 1.0);
    if (state->boss_death_mode.step == AZ_BDS_BOOM) {
      glPushMatrix(); {
        transform_to_camera_matrix(state);
        const az_vector_t position = state->boss_death_mode.boss.position;
        az_gl_translated(position);
        az_gl_rotated(az_vtheta(position));
        glBegin(GL_QUADS); {
          const GLfloat outer = 1.5f * AZ_SCREEN_WIDTH;
          const GLfloat inner = outer * state->boss_death_mode.progress *
            state->boss_death_mode.progress;
          glColor4f(1, 1, 1, state->boss_death_mode.progress);
          glVertex2f(outer, inner); glVertex2f(-outer, inner);
          glVertex2f(-outer, -inner); glVertex2f(outer, -inner);
          glVertex2f(inner, outer); glVertex2f(-inner, outer);
          glVertex2f(-inner, inner); glVertex2f(inner, inner);
          glVertex2f(inner, -outer); glVertex2f(-inner, -outer);
          glVertex2f(-inner, -inner); glVertex2f(inner, -inner);
        } glEnd();
      } glPopMatrix();
    } else if (state->boss_death_mode.step == AZ_BDS_FADE) {
      tint_screen(1, 1.0 - state->boss_death_mode.progress);
    }
  }

  // If we're going through a doorway, draw the doorway transition animation.
  if (state->mode == AZ_MODE_DOORWAY) {
    glPushMatrix(); {
      transform_to_camera_matrix(state);
      draw_doorway_transition(state);
    } glPopMatrix();
  }

  az_draw_hud(state);
  draw_global_fade(state);
}

/*===========================================================================*/
