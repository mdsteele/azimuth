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
#include "azimuth/state/baddie.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/door.h"
#include "azimuth/view/string.h"
#include "azimuth/view/wall.h"
#include "editor/list.h"
#include "editor/state.h"

/*===========================================================================*/

static void draw_selection_circle(az_vector_t position, double angle,
                                  double radius) {
  glPushMatrix(); {
    glTranslated(position.x, position.y, 0);
    glRotated(AZ_RAD2DEG(angle), 0, 0, 1);
    glScaled(radius, radius, 1);
    glColor3f(1, 1, 1); // white
    glBegin(GL_LINE_STRIP); {
      glVertex2d(0, 0);
      for (int i = 0; i <= 36; ++i) {
        glVertex2d(cos(i * AZ_PI / 18.0), sin(i * AZ_PI / 18.0));
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_camera_view(az_editor_state_t* state) {
  AZ_LIST_LOOP(wall, state->walls) {
    az_draw_wall(&wall->spec);
  }
  AZ_LIST_LOOP(editor_baddie, state->baddies) {
    az_baddie_t real_baddie;
    az_init_baddie(&real_baddie, editor_baddie->spec.kind,
                   editor_baddie->spec.position, editor_baddie->spec.angle);
    az_draw_baddie(&real_baddie);
  }
  AZ_LIST_LOOP(editor_door, state->doors) {
    az_door_t real_door;
    real_door.kind = editor_door->spec.kind;
    real_door.position = editor_door->spec.position;
    real_door.angle = editor_door->spec.angle;
    az_draw_door(&real_door);
  }
  AZ_LIST_LOOP(wall, state->walls) {
    if (!wall->selected) continue;
    draw_selection_circle(wall->spec.position, wall->spec.angle,
                          wall->spec.data->bounding_radius);
  }
  AZ_LIST_LOOP(baddie, state->baddies) {
    if (!baddie->selected) continue;
    draw_selection_circle(baddie->spec.position, baddie->spec.angle,
                          az_get_baddie_data(baddie->spec.kind)->
                          bounding_radius);
  }
  AZ_LIST_LOOP(door, state->doors) {
    if (!door->selected) continue;
    draw_selection_circle(door->spec.position, door->spec.angle,
                          AZ_DOOR_BOUNDING_RADIUS);
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
    case AZ_TOOL_BADDIE:
      tool_name = "BADDIE";
      glColor3f(0, 0, 1);
      break;
    case AZ_TOOL_DOOR:
      tool_name = "DOOR";
      glColor3f(0, 1, 0);
      break;
    case AZ_TOOL_WALL:
      tool_name = "WALL";
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
