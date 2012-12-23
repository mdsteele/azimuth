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

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/gui/event.h" // for az_get_mouse_position
#include "azimuth/state/baddie.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/baddie.h"
#include "azimuth/view/cursor.h"
#include "azimuth/view/door.h"
#include "azimuth/view/node.h"
#include "azimuth/view/string.h"
#include "azimuth/view/wall.h"
#include "editor/list.h"
#include "editor/state.h"

/*===========================================================================*/

static void arc_vertices(double r, double start_theta, double end_theta) {
  const double step = fmin(0.1, (end_theta - start_theta) * 0.05);
  for (double theta = start_theta;
       (theta < end_theta) == (start_theta < end_theta); theta += step) {
    glVertex2d(r * cos(theta), r * sin(theta));
  }
}

// Draw the bounds of where the camera center is allowed to be within the given
// room.
static void draw_camera_center_bounds(const az_editor_room_t *room) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  const double min_r = bounds->min_r;
  const double max_r = min_r + bounds->r_span;
  const double min_theta = bounds->min_theta;
  const double max_theta = min_theta + bounds->theta_span;
  glColor4f(1, 0.5, 0, 0.5); // orange tint
  glBegin(GL_LINE_LOOP); {
    glVertex2d(min_r * cos(min_theta), min_r * sin(min_theta));
    glVertex2d(max_r * cos(min_theta), max_r * sin(min_theta));
    arc_vertices(max_r, min_theta, max_theta);
    glVertex2d(max_r * cos(max_theta), max_r * sin(max_theta));
    glVertex2d(min_r * cos(max_theta), min_r * sin(max_theta));
    arc_vertices(min_r, max_theta, min_theta);
  } glEnd();
}

// Draw the (approximate) bounds of what the camera can actually see within the
// given room.
static void draw_camera_edge_bounds(const az_editor_room_t *room) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  double r_span = bounds->r_span + AZ_SCREEN_HEIGHT;
  if (min_r < 0.0) {
    r_span += min_r;
    min_r = 0.0;
  }
  glColor4f(1, 1, 1, 0.5); // white tint
  glBegin(GL_LINE_LOOP); {
    const double max_r = min_r + r_span;
    const double min_theta = bounds->min_theta;
    const double theta_span = bounds->theta_span;
    const double max_theta = min_theta + theta_span;
    arc_vertices(min_r, max_theta, min_theta);
    const az_vector_t vright =
      az_vneg(az_vrot90ccw(az_vpolar(AZ_SCREEN_WIDTH/2, min_theta)));
    const double minc = cos(min_theta), mins = sin(min_theta);
    glVertex2d(min_r * minc, min_r * mins);
    glVertex2d(min_r * minc + vright.x, min_r * mins + vright.y);
    const az_vector_t topright = {max_r * minc + vright.x,
                                  max_r * mins + vright.y};
    glVertex2d(topright.x, topright.y);
    const double topright_theta = az_vtheta(topright);
    const double topright_theta_plus = topright_theta + theta_span;
    arc_vertices(az_vnorm(topright), topright_theta, topright_theta_plus);
    const az_vector_t vleft =
      az_vrot90ccw(az_vpolar(AZ_SCREEN_WIDTH/2, max_theta));
    const double maxc = cos(max_theta), maxs = sin(max_theta);
    const az_vector_t topleft = {max_r * maxc + vleft.x,
                                 max_r * maxs + vleft.y};
    double topleft_theta = az_vtheta(topleft);
    while (topleft_theta < topright_theta_plus) topleft_theta += AZ_TWO_PI;
    arc_vertices(az_vnorm(topleft),
                 fmax(topleft_theta - theta_span, topright_theta_plus),
                 topleft_theta);
    glVertex2d(topleft.x, topleft.y);
    glVertex2d(min_r * maxc + vleft.x, min_r * maxs + vleft.y);
    glVertex2d(min_r * maxc, min_r * maxs);
  } glEnd();
}

static void camera_to_screen_orient(const az_editor_state_t *state,
                                    az_vector_t position) {
  glTranslated(position.x, position.y, 0);
  glScaled(1, -1, 1);
  if (state->spin_camera) {
    glRotated(90.0 - AZ_RAD2DEG(az_vtheta(state->camera)), 0, 0, 1);
  }
}

static void draw_room(az_editor_state_t *state, az_editor_room_t *room) {
  AZ_LIST_LOOP(wall, room->walls) {
    az_draw_wall(&wall->spec);
  }
  AZ_LIST_LOOP(editor_node, room->nodes) {
    az_node_t real_node;
    real_node.kind = editor_node->spec.kind;
    real_node.position = editor_node->spec.position;
    real_node.angle = editor_node->spec.angle;
    real_node.upgrade = editor_node->spec.upgrade;
    az_draw_node(&real_node, state->clock);
  }
  AZ_LIST_LOOP(editor_baddie, room->baddies) {
    az_baddie_t real_baddie;
    az_init_baddie(&real_baddie, editor_baddie->spec.kind,
                   editor_baddie->spec.position, editor_baddie->spec.angle);
    if (real_baddie.kind == AZ_BAD_NIGHTBUG) real_baddie.param = 1.0;
    az_draw_baddie(&real_baddie, state->clock);
  }
  AZ_LIST_LOOP(editor_door, room->doors) {
    az_door_t real_door;
    real_door.kind = editor_door->spec.kind;
    real_door.position = editor_door->spec.position;
    real_door.angle = editor_door->spec.angle;
    az_draw_door(&real_door);
    glPushMatrix(); {
      camera_to_screen_orient(state, real_door.position);
      glColor3f(0, 0, 1); // blue
      az_draw_printf(16, AZ_ALIGN_CENTER, 0, -7, "%03d",
                     editor_door->spec.destination);
    } glPopMatrix();
  }
}

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

static void draw_camera_view(az_editor_state_t *state) {
  // Draw other rooms:
  for (int i = 0; i < AZ_LIST_SIZE(state->planet.rooms); ++i) {
    if (i == state->current_room) continue;
    draw_room(state, AZ_LIST_GET(state->planet.rooms, i));
  }

  // Fade out other rooms:
  glPushMatrix(); {
    camera_to_screen_orient(state, state->camera);
    glScaled(state->zoom_level, state->zoom_level, 1);
    glColor4f(0, 0, 0, 0.75); // black tint
    glBegin(GL_QUADS); {
      glVertex2i( AZ_SCREEN_WIDTH/2,  AZ_SCREEN_HEIGHT/2);
      glVertex2i(-AZ_SCREEN_WIDTH/2,  AZ_SCREEN_HEIGHT/2);
      glVertex2i(-AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2);
      glVertex2i( AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2);
    } glEnd();
  } glPopMatrix();

  // Draw current room:
  az_editor_room_t *room = AZ_LIST_GET(state->planet.rooms,
                                       state->current_room);
  draw_room(state, room);

  // Draw the camera bounds, and a dot for the camera center.
  draw_camera_center_bounds(room);
  draw_camera_edge_bounds(room);
  glColor4f(1, 0, 0, 0.5); // red tint
  glBegin(GL_POINTS); {
    glVertex2d(state->camera.x, state->camera.y);
  } glEnd();

  // Draw selection circles:
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    draw_selection_circle(wall->spec.position, wall->spec.angle,
                          wall->spec.data->bounding_radius);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    draw_selection_circle(node->spec.position, node->spec.angle,
                          AZ_NODE_BOUNDING_RADIUS);
  }
  AZ_LIST_LOOP(baddie, room->baddies) {
    if (!baddie->selected) continue;
    draw_selection_circle(baddie->spec.position, baddie->spec.angle,
                          az_get_baddie_data(baddie->spec.kind)->
                          overall_bounding_radius);
  }
  AZ_LIST_LOOP(door, room->doors) {
    if (!door->selected) continue;
    draw_selection_circle(door->spec.position, door->spec.angle,
                          AZ_DOOR_BOUNDING_RADIUS);
  }
}

static void draw_hud(az_editor_state_t* state) {
  // Draw the room name:
  glColor3f(1, 1, 1); // white
  az_draw_printf(8, AZ_ALIGN_LEFT, 20, 5, "[Room %03d]", state->current_room);

  // Draw the tool name:
  const char *tool_name = "???";
  switch (state->tool) {
    case AZ_TOOL_MOVE:
      tool_name = "MOVE";
      glColor3f(1, 0, 1);
      break;
    case AZ_TOOL_MASS_MOVE:
      tool_name = "MASS-MOVE";
      glColor3f(1, 0.5, 1);
      break;
    case AZ_TOOL_ROTATE:
      tool_name = "ROTATE";
      glColor3f(1, 0, 0);
      break;
    case AZ_TOOL_MASS_ROTATE:
      tool_name = "MASS-ROTATE";
      glColor3f(1, 0.5, 0.5);
      break;
    case AZ_TOOL_CAMERA:
      tool_name = "CAMERA";
      glColor3f(1, 0.5, 0);
      break;
    case AZ_TOOL_BADDIE:
      tool_name = "BADDIE";
      glColor3f(0, 0, 1);
      break;
    case AZ_TOOL_DOOR:
      tool_name = "DOOR";
      glColor3f(0, 1, 0);
      break;
    case AZ_TOOL_NODE:
      tool_name = "NODE";
      glColor3f(0, 1, 1);
      break;
    case AZ_TOOL_WALL:
      tool_name = "WALL";
      glColor3f(1, 1, 0);
      break;
  }
  az_draw_string(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5, AZ_SCREEN_HEIGHT - 13,
                 tool_name);

  // Draw space coords of mouse position:
  int x, y;
  if (az_get_mouse_position(&x, &y)) {
    const az_vector_t pt = az_pixel_to_position(state, x, y);
    glColor3f(1, 1, 1); // white
    az_draw_printf(8, AZ_ALIGN_LEFT, 5, AZ_SCREEN_HEIGHT - 13,
                   "(%.01f, %.01f)", pt.x, pt.y);
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

  // Draw the text box:
  if (state->text.action != AZ_ETA_NOTHING) {
    const double left = 5.5;
    const double right = AZ_SCREEN_WIDTH - 5.5;
    const double top = 20.5;
    const double bottom = top + 22;
    glColor3f(0, 0, 0); // black
    glBegin(GL_QUADS); {
      glVertex2d(left, top);
      glVertex2d(left, bottom);
      glVertex2d(right, bottom);
      glVertex2d(right, top);
    } glEnd();
    glColor3f(0, 1, 1); // cyan
    glBegin(GL_LINE_LOOP); {
      glVertex2d(left, top);
      glVertex2d(left, bottom);
      glVertex2d(right, bottom);
      glVertex2d(right, top);
    } glEnd();
    glColor3f(1, 1, 1); // white
    az_draw_chars(16, AZ_ALIGN_LEFT, 9, 24, state->text.buffer,
                  state->text.length);
    if (az_clock_mod(2, 16, state->clock) != 0) {
      glColor3f(1, 0, 0); // red
      glBegin(GL_LINES); {
        const double x = 9.5 + 16 * state->text.length;
        glVertex2d(x, top + 3);
        glVertex2d(x, bottom - 3);
      } glEnd();
    }
  }
}

void az_editor_draw_screen(az_editor_state_t* state) {
  glPushMatrix(); {
    // Make positive Y be up instead of down.
    glScaled(1, -1, 1);
    // Center the screen on position (0, 0).
    glTranslated(AZ_SCREEN_WIDTH/2, -AZ_SCREEN_HEIGHT/2, 0);
    // Zoom out.
    glScaled(1.0 / state->zoom_level, 1.0 / state->zoom_level, 1);
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
  az_draw_cursor();
}

az_vector_t az_pixel_to_position(const az_editor_state_t *state,
                                 int x, int y) {
  az_vector_t pt = {x - AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2 - y};
  pt = az_vmul(pt, state->zoom_level);
  if (state->spin_camera) {
    pt.y += az_vnorm(state->camera);
    pt = az_vrotate(pt, az_vtheta(state->camera) - AZ_HALF_PI);
  } else {
    pt = az_vadd(pt, state->camera);
  }
  return pt;
}

/*===========================================================================*/
