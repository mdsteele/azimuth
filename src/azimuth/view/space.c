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

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h" // for AZ_ASSERT_UNREACHABLE
#include "azimuth/util/vector.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/door.h"
#include "azimuth/view/gravfield.h"
#include "azimuth/view/hud.h"
#include "azimuth/view/node.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/pickup.h"
#include "azimuth/view/projectile.h"
#include "azimuth/view/ship.h"
#include "azimuth/view/speck.h"
#include "azimuth/view/wall.h"

/*===========================================================================*/

static double mode_fade_alpha(az_space_state_t *state) {
  switch (state->mode) {
    case AZ_MODE_DOORWAY:
      switch (state->mode_data.doorway.step) {
        case AZ_DWS_FADE_OUT:
          return state->mode_data.doorway.progress;
        case AZ_DWS_SHIFT:
          return 1.0;
        case AZ_DWS_FADE_IN:
          return 1.0 - state->mode_data.doorway.progress;
      }
      AZ_ASSERT_UNREACHABLE();
    case AZ_MODE_GAME_OVER:
      if (state->mode_data.game_over.step == AZ_GOS_FADE_OUT) {
        return state->mode_data.game_over.progress;
      }
      return 0.0;
    case AZ_MODE_PAUSING:
      return state->mode_data.pause.progress;
    case AZ_MODE_RESUMING:
      return 1.0 - state->mode_data.pause.progress;
    case AZ_MODE_NORMAL:
    case AZ_MODE_DIALOG:
    case AZ_MODE_SAVING:
    case AZ_MODE_UPGRADE:
      return 0.0;
  }
  AZ_ASSERT_UNREACHABLE();
}

static void tint_screen(GLfloat alpha) {
  glColor4f(0, 0, 0, alpha);
  glBegin(GL_QUADS); {
    glVertex2i(0, 0);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
  } glEnd();
}

static void draw_camera_view(az_space_state_t *state) {
  az_draw_background_nodes(state);
  glPushMatrix(); {
    glLoadIdentity();
    tint_screen(0.5);
  } glPopMatrix();
  az_draw_gravfields(state);
  az_draw_upgrade_nodes(state);
  az_draw_walls(state);
  az_draw_middle_nodes(state);
  az_draw_pickups(state);
  az_draw_projectiles(state);
  az_draw_baddies(state);
  az_draw_ship(state);
  az_draw_particles(state);
  az_draw_doors(state);
  az_draw_specks(state);
  az_draw_foreground_nodes(state);
}

void az_space_draw_screen(az_space_state_t *state) {
  // Check if we're in a mode where we should be tinting the camera view black.
  const double fade_alpha = mode_fade_alpha(state);
  assert(fade_alpha >= 0.0);
  assert(fade_alpha <= 1.0);

  // Draw the camera view.
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    // Move the screen to the camera pose.
    glTranslated(0, -az_vnorm(state->camera.center), 0);
    glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera.center)), 0, 0, 1);
    // Apply camera shake to the screen position.
    const az_vector_t shake_offset =
      az_camera_shake_offset(&state->camera, state->clock);
    glTranslated(shake_offset.x, shake_offset.y, 0);
    // Draw what the camera sees.
    if (fade_alpha < 1.0) {
      draw_camera_view(state);
    }
    if (state->mode == AZ_MODE_DOORWAY) {
      // TODO: draw door transition
    }
  } glPopMatrix();

  // Tint the camera view black (based on fade_alpha).
  if (fade_alpha > 0.0) tint_screen(fade_alpha);

  az_draw_hud(state);
}

/*===========================================================================*/
