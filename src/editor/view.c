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

#include "editor/view.h"

#include <math.h>
#include <stdio.h> // for snprintf
#include <string.h> // for strlen

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/gui/event.h" // for az_get_mouse_position
#include "azimuth/util/vector.h"
#include "azimuth/view/string.h"
#include "azimuth/view/wall.h"
#include "editor/state.h"

/*===========================================================================*/

static void draw_camera_view(az_editor_state_t* state) {
  az_room_t *room = state->room;
  for (int i = 0; i < room->num_walls; ++i) {
    const az_wall_t *wall = &room->walls[i];
    az_draw_wall(wall);
  }
  if (state->selected_wall != NULL) {
    const az_wall_t *wall = state->selected_wall;
    glPushMatrix(); {
      glTranslated(wall->position.x, wall->position.y, 0);
      glRotated(AZ_RAD2DEG(wall->angle), 0, 0, 1);
      glColor3f(1, 1, 1); // white
      glBegin(GL_LINE_STRIP); {
        glVertex2d(0, 0);
        const double r = wall->data->bounding_radius;
        for (int i = 0; i <= 36; ++i) {
          glVertex2d(r * cos(i * AZ_PI / 18.0), r * sin(i * AZ_PI / 18.0));
        }
      } glEnd();
    } glPopMatrix();
  }
}

static void draw_hud(az_editor_state_t* state) {
  // Draw the tool name:
  const char *tool_name = "???";
  switch (state->tool) {
    case AZ_TOOL_MOVE:
      tool_name = "MOVE";
      glColor3f(1, 0, 1);
      break;
    case AZ_TOOL_ROTATE:
      tool_name = "ROTATE";
      glColor3f(1, 0, 0);
      break;
    case AZ_TOOL_ADD:
      tool_name = "ADD";
      glColor3f(1, 1, 0);
      break;
  }
  az_draw_string((az_vector_t){AZ_SCREEN_WIDTH - 10 - 8 * strlen(tool_name),
                               AZ_SCREEN_HEIGHT - 18}, 8, tool_name);

  // Draw space coords of mouse position:
  int x, y;
  if (az_get_mouse_position(&x, &y)) {
    const az_vector_t pt = az_pixel_to_position(state, x, y);
    char buffer[40];
    snprintf(buffer, sizeof(buffer), "(%.01f, %.01f)", pt.x, pt.y);
    glColor3f(1, 1, 1); // white
    az_draw_string((az_vector_t){10, AZ_SCREEN_HEIGHT - 18}, 8, buffer);
  }

  // Draw the unsaved indicator:
  if (state->unsaved) {
    glColor3f(1, 0.5, 0); // orange
    glBegin(GL_TRIANGLES); {
      glVertex2d(5, 5);
      glVertex2d(5, 15);
      glVertex2d(15, 5);
    } glEnd();
  }
}

void az_editor_draw_screen(az_editor_state_t* state) {
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    // Move the screen to the camera pose.
    if (state->spin_camera) {
      glTranslated(0, -az_vnorm(state->camera), 0);
      glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera)), 0, 0, 1);
    } else {
      glTranslated(-state->camera.x, -state->camera.y, 0);
    }
    // Draw what the camera sees.
    draw_camera_view(state);
  } glPopMatrix();

  draw_hud(state);
}

az_vector_t az_pixel_to_position(az_editor_state_t *state, int x, int y) {
  az_vector_t pt = {x - AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2 - y};
  if (state->spin_camera) {
    pt.y += az_vnorm(state->camera);
    pt = az_vrotate(pt, az_vtheta(state->camera) - AZ_HALF_PI);
  } else {
    pt = az_vadd(pt, state->camera);
  }
  return pt;
}

/*===========================================================================*/
