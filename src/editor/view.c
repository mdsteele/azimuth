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
#include "azimuth/view/gravfield.h"
#include "azimuth/view/node.h"
#include "azimuth/view/string.h"
#include "azimuth/view/wall.h"
#include "editor/list.h"
#include "editor/state.h"

/*===========================================================================*/

static void arc_vertices(double r, double start_theta, double end_theta) {
  const double diff = end_theta - start_theta;
  const double step = copysign(fmin(0.1, fabs(diff) * 0.05), diff);
  for (double theta = start_theta;
       (theta < end_theta) == (start_theta < end_theta); theta += step) {
    glVertex2d(r * cos(theta), r * sin(theta));
  }
}

static void circle_vertices(double r) {
  arc_vertices(r, 0, AZ_TWO_PI);
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
  if (room->selected) glColor3f(0, 1, 0); // green
  else glColor3f(0.75, 0.75, 0.75); // white
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  if (bounds->theta_span >= 6.28) {
    glBegin(GL_LINE_LOOP); {
      circle_vertices(hypot(AZ_SCREEN_WIDTH/2,
          bounds->min_r + bounds->r_span + AZ_SCREEN_HEIGHT/2));
    } glEnd();
    if (bounds->min_r > AZ_SCREEN_HEIGHT/2) {
      glBegin(GL_LINE_LOOP); {
        circle_vertices(bounds->min_r - AZ_SCREEN_HEIGHT/2);
      } glEnd();
    }
    return;
  }
  double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  double r_span = bounds->r_span + AZ_SCREEN_HEIGHT;
  if (min_r < 0.0) {
    r_span += min_r;
    min_r = 0.0;
  }
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

static void tint_screen(GLfloat alpha) {
  glPushMatrix(); {
    glLoadIdentity();
    glColor4f(0, 0, 0, alpha);
    glBegin(GL_QUADS); {
      glVertex2i(0, 0);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(AZ_SCREEN_WIDTH, 0);
    } glEnd();
  } glPopMatrix();
}

static void draw_node(const az_editor_state_t *state,
                      az_editor_node_t *editor_node) {
  const az_node_t real_node = {
    .kind = editor_node->spec.kind,
    .subkind = editor_node->spec.subkind,
    .position = editor_node->spec.position,
    .angle = editor_node->spec.angle
  };
  az_draw_node(&real_node, state->clock);
  if (editor_node->spec.on_use != NULL) {
    glPushMatrix(); {
      camera_to_screen_orient(state, real_node.position);
      glColor3f(0.75, 0, 1); // purple
      glBegin(GL_QUADS); {
        glVertex2f(5, -9); glVertex2f(-5, -9);
        glVertex2f(-5, 0); glVertex2f(5, 0);
      } glEnd();
      glColor3f(0, 0, 0); // black
      az_draw_string(8, AZ_ALIGN_CENTER, 0, -8, "$");
    } glPopMatrix();
  }
}

static void draw_room(az_editor_state_t *state, az_editor_room_t *room) {
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_BG) {
      draw_node(state, editor_node);
    }
  }
  if (room == AZ_LIST_GET(state->planet.rooms, state->current_room)) {
    tint_screen(0.5);
  }
  AZ_LIST_LOOP(editor_gravfield, room->gravfields) {
    const az_gravfield_t real_gravfield = {
      .kind = editor_gravfield->spec.kind,
      .position = editor_gravfield->spec.position,
      .angle = editor_gravfield->spec.angle,
      .strength = editor_gravfield->spec.strength,
      .size = editor_gravfield->spec.size
    };
    az_draw_gravfield(&real_gravfield, state->total_time);
  }
  AZ_LIST_LOOP(editor_wall, room->walls) {
    const az_wall_t real_wall = {
      .kind = editor_wall->spec.kind,
      .data = editor_wall->spec.data,
      .position = editor_wall->spec.position,
      .angle = editor_wall->spec.angle,
      .flare = (editor_wall->spec.kind == AZ_WALL_INDESTRUCTIBLE ? 0.0 :
                0.5 + 0.01 * az_clock_zigzag(50, 1, state->clock))
    };
    az_draw_wall(&real_wall);
  }
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_FG ||
        editor_node->spec.kind == AZ_NODE_DOODAD_BG) continue;
    draw_node(state, editor_node);
  }
  AZ_LIST_LOOP(editor_baddie, room->baddies) {
    az_baddie_t real_baddie = {0};
    az_init_baddie(&real_baddie, editor_baddie->spec.kind,
                   editor_baddie->spec.position, editor_baddie->spec.angle);
    if (real_baddie.kind == AZ_BAD_NIGHTBUG) real_baddie.param = 1.0;
    az_draw_baddie(&real_baddie, state->clock);
    if (editor_baddie->spec.on_kill != NULL ||
        editor_baddie->spec.uuid_slot != 0) {
      glPushMatrix(); {
        camera_to_screen_orient(state, real_baddie.position);
        if (editor_baddie->spec.on_kill != NULL) {
          glColor3f(0.75, 0, 1); // purple
          glBegin(GL_QUADS); {
            glVertex2f(5, -9); glVertex2f(-5, -9);
            glVertex2f(-5, 0); glVertex2f(5, 0);
          } glEnd();
          glColor3f(0, 0, 0); // black
          az_draw_string(8, AZ_ALIGN_CENTER, 0, -8, "$");
        }
        if (editor_baddie->spec.uuid_slot != 0) {
          glColor3f(1, 0, 0); // red
          glBegin(GL_QUADS); {
            glVertex2f(9, 0); glVertex2f(-9, 0);
            glVertex2f(-9, 9); glVertex2f(9, 9);
          } glEnd();
          glColor3f(0, 0, 0); // black
          az_draw_printf(8, AZ_ALIGN_CENTER, 0, 1, "%02d",
                         editor_baddie->spec.uuid_slot);
        }
      } glPopMatrix();
    }
  }
  AZ_LIST_LOOP(editor_door, room->doors) {
    const az_door_t real_door = {
      .kind = editor_door->spec.kind,
      .position = editor_door->spec.position,
      .angle = editor_door->spec.angle
    };
    az_draw_door(&real_door);
    glPushMatrix(); {
      camera_to_screen_orient(state, real_door.position);
      glColor3f(0, 0, 1); // blue
      az_draw_printf(16, AZ_ALIGN_CENTER, 0, -7, "%03d",
                     editor_door->spec.destination);
      if (editor_door->spec.on_open != NULL) {
        glColor3f(0.75, 0, 1); // purple
        az_draw_string(8, AZ_ALIGN_CENTER, 0, -15, "$");
      }
      if (editor_door->spec.uuid_slot != 0) {
        glColor3f(1, 0, 0); // red
        az_draw_printf(8, AZ_ALIGN_CENTER, 0, 8, "%02d",
                     editor_door->spec.uuid_slot);
      }
    } glPopMatrix();
  }
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_FG) {
      draw_node(state, editor_node);
    }
  }
}

static void draw_room_minimap(az_editor_state_t *state,
                              az_editor_room_t *room, az_room_key_t key) {
  draw_camera_edge_bounds(room);
  // Draw doors (schematically):
  if (state->zoom_level < 64.0) {
    AZ_LIST_LOOP(editor_door, room->doors) {
      switch (editor_door->spec.kind) {
        case AZ_DOOR_NOTHING: AZ_ASSERT_UNREACHABLE();
        case AZ_DOOR_NORMAL: glColor3f(1, 1, 1); break;
        case AZ_DOOR_LOCKED: glColor3f(0.5, 0.5, 0.5); break;
        case AZ_DOOR_ROCKET: glColor3f(1, 0, 0); break;
        case AZ_DOOR_HYPER_ROCKET: glColor3f(1, 0, 1); break;
        case AZ_DOOR_BOMB: glColor3f(0, 0, 1); break;
        case AZ_DOOR_MEGA_BOMB: glColor3f(0, 1, 1); break;
        case AZ_DOOR_PASSAGE: glColor3f(0, 1, 0); break;
      }
      glPushMatrix(); {
        glTranslated(editor_door->spec.position.x,
                     editor_door->spec.position.y, 0);
        glBegin(GL_POLYGON); {
          circle_vertices(AZ_DOOR_BOUNDING_RADIUS);
        } glEnd();
      } glPopMatrix();
    }
  }
  // Draw room number:
  if (state->zoom_level < 32.0) {
    glPushMatrix(); {
      az_camera_bounds_t *bounds = &room->camera_bounds;
      camera_to_screen_orient(state, az_vpolar(
          bounds->min_r + 0.5 * bounds->r_span,
          bounds->min_theta + 0.5 * bounds->theta_span));
      glScaled(state->zoom_level, state->zoom_level, 1);
      const az_color_t color =
        AZ_LIST_GET(state->planet.zones, room->zone_index)->color;
      glColor3ub(color.r, color.g, color.b);
      az_draw_printf(8, AZ_ALIGN_CENTER, 0, -3, "%03d", key);
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
      glVertex2f(0, 0);
      for (int i = 0; i <= 36; ++i) {
        glVertex2d(cos(i * AZ_PI / 18.0), sin(i * AZ_PI / 18.0));
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_camera_view(az_editor_state_t *state) {
  glColor3f(1, 1, 0); // yellow
  glBegin(GL_LINE_LOOP); {
    circle_vertices(AZ_PLANETOID_RADIUS);
  } glEnd();

  // Draw other rooms:
  for (int i = 0; i < AZ_LIST_SIZE(state->planet.rooms); ++i) {
    if (i == state->current_room) continue;
    az_editor_room_t *room = AZ_LIST_GET(state->planet.rooms, i);
    if (!az_editor_is_in_minimap_mode(state)) {
      draw_room(state, room);
      if (room->selected) draw_camera_edge_bounds(room);
    } else draw_room_minimap(state, room, i);
  }

  // Fade out other rooms:
  if (!az_editor_is_in_minimap_mode(state)) {
    tint_screen(0.5);
  }

  // Draw current room:
  az_editor_room_t *room = AZ_LIST_GET(state->planet.rooms,
                                       state->current_room);
  if (!az_editor_is_in_minimap_mode(state)) draw_room(state, room);
  else draw_room_minimap(state, room, state->current_room);

  // Draw the camera bounds, and a dot for the camera center.
  draw_camera_center_bounds(room);
  draw_camera_edge_bounds(room);
  glColor4f(1, 0, 0, 0.75); // red tint
  glBegin(GL_POINTS); {
    glVertex2d(state->camera.x, state->camera.y);
  } glEnd();

  // Draw selection circles:
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    draw_selection_circle(wall->spec.position, wall->spec.angle,
                          wall->spec.data->bounding_radius);
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    if (gravfield->spec.kind == AZ_GRAV_TRAPEZOID) {
      draw_selection_circle(gravfield->spec.position, gravfield->spec.angle,
                            gravfield->spec.size.trapezoid.semilength);
    } else {
      draw_selection_circle(gravfield->spec.position, gravfield->spec.angle,
                            gravfield->spec.size.sector.inner_radius +
                            gravfield->spec.size.sector.thickness);
    }
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
  const az_editor_room_t *room =
    AZ_LIST_GET(state->planet.rooms, state->current_room);
  // Draw the room name:
  glColor3f(1, 1, 1); // white
  az_draw_printf(8, AZ_ALIGN_LEFT, 20, 5, "[Room %03d]", state->current_room);
  if (room->on_start != NULL) {
    glColor3f(0.75, 0, 1); // purple
    az_draw_string(8, AZ_ALIGN_LEFT, 100, 5, "$");
  }

  // Draw the zone name:
  const az_zone_t *zone = AZ_LIST_GET(state->planet.zones, room->zone_index);
  glColor3ub(zone->color.r, zone->color.g, zone->color.b);
  az_draw_string(8, AZ_ALIGN_LEFT, 150, 5, zone->name);

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
    case AZ_TOOL_GRAVFIELD:
      tool_name = "GRAVFIELD";
      glColor3f(0, 0.5, 0.5);
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
      glVertex2f(5, 5); glVertex2f(5, 15); glVertex2f(15, 5);
    } glEnd();
  }

  // Draw the text box:
  if (state->text.action != AZ_ETA_NOTHING) {
    const GLfloat left = 5.5;
    const GLfloat right = AZ_SCREEN_WIDTH - 5.5;
    const GLfloat top = 20.5;
    const GLfloat bottom = top + 22;
    glColor3f(0, 0, 0); // black
    glBegin(GL_QUADS); {
      glVertex2f(left, top); glVertex2f(left, bottom);
      glVertex2f(right, bottom); glVertex2f(right, top);
    } glEnd();
    glColor3f(0, 1, 1); // cyan
    glBegin(GL_LINE_LOOP); {
      glVertex2f(left, top); glVertex2f(left, bottom);
      glVertex2f(right, bottom); glVertex2f(right, top);
    } glEnd();
    glColor3f(1, 1, 1); // white
    az_draw_chars(16, AZ_ALIGN_LEFT, 9, 24, state->text.buffer,
                  state->text.length);
    if (az_clock_mod(2, 16, state->clock) != 0) {
      glColor3f(1, 0, 0); // red
      glBegin(GL_LINES); {
        const GLfloat x = 9.5 + 16 * state->text.cursor;
        glVertex2f(x, top + 3);
        glVertex2f(x, bottom - 3);
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
