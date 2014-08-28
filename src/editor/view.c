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
#include "azimuth/view/util.h"
#include "azimuth/view/wall.h"
#include "editor/list.h"
#include "editor/state.h"

/*===========================================================================*/

#define EDITOR_TEXT_BOX_TOP_MARGIN 20.5
#define EDITOR_TEXT_BOX_SIDE_MARGIN 5.5
#define EDITOR_TEXT_BOX_PADDING 3.5
#define EDITOR_TEXT_BOX_FONT_SIZE 16
#define EDITOR_TEXT_BOX_ROW_HEIGHT 22

/*===========================================================================*/

static void arc_vertices(double r, double start_theta, double end_theta) {
  const double diff = end_theta - start_theta;
  if (diff == 0.0) return;
  const double step = copysign(fmin(0.1, fabs(diff) * 0.05), diff);
  assert(step != 0.0);
  for (double theta = start_theta;
       (theta < end_theta) == (start_theta < end_theta); theta += step) {
    glVertex2d(r * cos(theta), r * sin(theta));
  }
  glVertex2d(r * cos(end_theta), r * sin(end_theta));
}

static void circle_vertices(double r) {
  arc_vertices(r, 0, AZ_TWO_PI);
}

static void draw_dashed_line(az_vector_t start, az_vector_t end) {
  const int num_steps =
    1 + 2 * (int)ceil(0.5 * (az_vdist(start, end) / 10.0 - 1.0));
  const az_vector_t step = az_vdiv(az_vsub(end, start), num_steps);
  glBegin(GL_LINES); {
    for (int i = 0; i <= num_steps; ++i) {
      az_gl_vertex(az_vadd(start, az_vmul(step, i)));
    }
  } glEnd();
}

static void draw_dashed_circle(az_vector_t center, double r) {
  glPushMatrix(); {
    az_gl_translated(center);
    glBegin(GL_LINES); {
      circle_vertices(r);
    } glEnd();
  } glPopMatrix();
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

static void draw_zone_swatch(az_editor_state_t *state,
                             const az_editor_room_t *room) {
  const az_camera_bounds_t *bounds = &room->camera_bounds;
  const double min_r = bounds->min_r - AZ_SCREEN_HEIGHT/2;
  const double max_r = min_r + bounds->r_span + AZ_SCREEN_HEIGHT;
  const double min_theta = bounds->min_theta;
  const double max_theta = min_theta + bounds->theta_span;

  const az_vector_t offset1 =
    az_vpolar(AZ_SCREEN_WIDTH/2, min_theta - AZ_HALF_PI);
  const az_vector_t bot_right = az_vadd(offset1, az_vpolar(min_r, min_theta));
  const az_vector_t top_right = az_vadd(offset1, az_vpolar(max_r, min_theta));
  const az_vector_t offset2 =
    az_vpolar(AZ_SCREEN_WIDTH/2, max_theta + AZ_HALF_PI);
  const az_vector_t bot_left = az_vadd(offset2, az_vpolar(min_r, max_theta));
  const az_vector_t top_left = az_vadd(offset2, az_vpolar(max_r, max_theta));

  const az_color_t color =
    AZ_LIST_GET(state->planet.zones, room->zone_key)->color;
  if (room->properties & AZ_ROOMF_UNMAPPED) {
    glColor3ub(color.r / 4, color.g / 4, color.b / 4);
  } else glColor3ub(color.r / 2, color.g / 2, color.b / 2);
  glBegin(GL_QUAD_STRIP); {
    az_gl_vertex(top_right);
    az_gl_vertex(bot_right);
    const double step = fmax(AZ_DEG2RAD(0.1), bounds->theta_span * 0.05);
    for (double theta = min_theta; theta <= max_theta; theta += step) {
      glVertex2d(max_r * cos(theta), max_r * sin(theta));
      glVertex2d(min_r * cos(theta), min_r * sin(theta));
    }
    az_gl_vertex(top_left);
    az_gl_vertex(bot_left);
  } glEnd();
  // Draw a red + if the room is heated.
  if (room->properties & AZ_ROOMF_HEATED) {
    glColor3f(1, 0, 0);
    glBegin(GL_LINES); {
      az_gl_vertex(az_vmul(az_vadd(bot_right, top_right), 0.5));
      az_gl_vertex(az_vmul(az_vadd(bot_left, top_left), 0.5));
      az_gl_vertex(az_vpolar(min_r, min_theta + 0.5 * bounds->theta_span));
      az_gl_vertex(az_vpolar(max_r, min_theta + 0.5 * bounds->theta_span));
    } glEnd();
  }
  // Draw a black X if the room is dark.
  if (room->on_start != NULL && room->on_start->num_instructions > 0 &&
      room->on_start->instructions[0].opcode == AZ_OP_DARK) {
    const double darkness = room->on_start->instructions[0].immediate;
    glColor4f(0, 0, 0, darkness);
    glBegin(GL_LINES); {
      az_gl_vertex(top_left); az_gl_vertex(bot_right);
      az_gl_vertex(bot_left); az_gl_vertex(top_right);
    } glEnd();
  }
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

static void draw_script_and_uuid_slot(
    const az_editor_state_t *state, az_vector_t position,
    const az_script_t *script, int uuid_slot) {
  if (script == NULL && uuid_slot == 0) return;
  glPushMatrix(); {
    camera_to_screen_orient(state, position);
    if (script != NULL) {
      glColor3f(0.75, 0, 1); // purple
      glBegin(GL_QUADS); {
        glVertex2f(5, -9); glVertex2f(-5, -9);
        glVertex2f(-5, 0); glVertex2f(5, 0);
      } glEnd();
      glColor3f(0, 0, 0); // black
      az_draw_string(8, AZ_ALIGN_CENTER, 0, -8, "$");
    }
    if (uuid_slot != 0) {
      glColor3f(1, 0, 0); // red
      glBegin(GL_QUADS); {
        glVertex2f(9, 0); glVertex2f(-9, 0);
        glVertex2f(-9, 9); glVertex2f(9, 9);
      } glEnd();
      glColor3f(0, 0, 0); // black
      az_draw_printf(8, AZ_ALIGN_CENTER, 0, 1, "%02d", uuid_slot);
    }
  } glPopMatrix();
}

static void draw_gravfield_border(const az_gravfield_spec_t *gravfield) {
  glPushMatrix(); {
    glTranslated(gravfield->position.x, gravfield->position.y, 0);
    glRotated(AZ_RAD2DEG(gravfield->angle), 0, 0, 1);
    if (az_is_trapezoidal(gravfield->kind)) {
      const double foff = gravfield->size.trapezoid.front_offset;
      const double fsw = gravfield->size.trapezoid.front_semiwidth;
      const double rsw = gravfield->size.trapezoid.rear_semiwidth;
      const double sl = gravfield->size.trapezoid.semilength;
      glBegin(GL_LINE_LOOP); {
        glVertex2d(sl, foff - fsw); glVertex2d(sl, foff + fsw);
        glVertex2d(-sl, rsw); glVertex2d(-sl, -rsw);
      } glEnd();
      glBegin(GL_LINES); {
        glVertex2d(0, 0); glVertex2d(sl, 0);
      } glEnd();
    } else {
      glBegin(GL_LINE_LOOP); {
        const double sweep = az_sector_interior_angle(&gravfield->size);
        const double irad = gravfield->size.sector.inner_radius;
        const double thick = gravfield->size.sector.thickness;
        arc_vertices(irad + thick, 0, sweep);
        if (irad > 0.0) arc_vertices(irad, sweep, 0);
        else glVertex2f(0, 0);
      } glEnd();
    }
  } glPopMatrix();
}

static void draw_truck_route(
    const az_editor_state_t *state, const az_baddie_t *baddie,
    double dist, double turn, int count) {
  az_vector_t start = baddie->position;
  double angle = baddie->angle;
  for (int i = 0; i < count; ++i) {
    az_vector_t end;
    if (!az_circle_hits_editor_walls(
            state, baddie->data->overall_bounding_radius, start,
            az_vpolar(10000, angle), &end, NULL)) break;
    if (az_vdist(start, end) > dist) {
      az_vpluseq(&end, az_vwithlen(az_vsub(start, end), dist));
      glColor4f(1, 0, 0, 0.5);
      draw_dashed_line(start, end);
      start = end;
    }
    angle += turn;
  }
}

static void draw_beam_path(const az_editor_state_t *state,
                           const az_baddie_t *baddie) {
  const az_vector_t delta = az_vpolar(10000, baddie->angle);
  az_vector_t end = az_vadd(baddie->position, delta);
  az_circle_hits_editor_walls(state, 0.0, baddie->position, delta, &end, NULL);
  glColor4f(1, 0, 0, 0.5);
  draw_dashed_line(baddie->position, end);
}

static void draw_baddie(const az_editor_state_t *state,
                        az_editor_baddie_t *editor_baddie, bool draw_bg) {
  az_baddie_t real_baddie;
  az_init_baddie(&real_baddie, editor_baddie->spec.kind,
                 editor_baddie->spec.position, editor_baddie->spec.angle);
  const bool is_bg = az_baddie_has_flag(&real_baddie, AZ_BADF_DRAW_BG);
  if (is_bg != draw_bg) return;
  if (real_baddie.kind == AZ_BAD_NIGHTBUG ||
      real_baddie.kind == AZ_BAD_NIGHTSHADE ||
      real_baddie.kind == AZ_BAD_NOCTURNE) real_baddie.param = 1.0;
  if (real_baddie.kind == AZ_BAD_DEATH_RAY ||
      real_baddie.kind == AZ_BAD_HEAT_RAY ||
      real_baddie.kind == AZ_BAD_SENSOR_LASER) {
    draw_beam_path(state, &real_baddie);
  } else if (real_baddie.kind == AZ_BAD_NUCLEAR_MINE) {
    glColor4f(1, 0, 0, 0.5);
    draw_dashed_circle(real_baddie.position, 150);
  } else if (real_baddie.kind == AZ_BAD_PROXY_MINE) {
    glColor4f(1, 0, 0, 0.5);
    draw_dashed_circle(real_baddie.position, 80);
  } else if (real_baddie.kind == AZ_BAD_SMALL_TRUCK) {
    draw_truck_route(state, &real_baddie, 100.0, AZ_DEG2RAD(90), 5);
  } else if (real_baddie.kind == AZ_BAD_SMALL_AUV) {
    draw_truck_route(state, &real_baddie, 50.0, AZ_DEG2RAD(-60), 7);
  }
  az_draw_baddie(&real_baddie, state->clock);
  draw_script_and_uuid_slot(
      state, real_baddie.position, editor_baddie->spec.on_kill,
      editor_baddie->spec.uuid_slot);
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
  draw_script_and_uuid_slot(
      state, real_node.position, editor_node->spec.on_use,
      editor_node->spec.uuid_slot);
}

static void draw_room(az_editor_state_t *state, az_editor_room_t *room) {
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_BG ||
        editor_node->spec.kind == AZ_NODE_FAKE_WALL_BG) {
      draw_node(state, editor_node);
    }
  }
  if (room == AZ_LIST_GET(state->planet.rooms, state->current_room)) {
    tint_screen(0.5);
  }
  AZ_LIST_LOOP(editor_gravfield, room->gravfields) {
    if (az_is_liquid(editor_gravfield->spec.kind)) continue;
    const az_gravfield_t real_gravfield = {
      .kind = editor_gravfield->spec.kind,
      .position = editor_gravfield->spec.position,
      .angle = editor_gravfield->spec.angle,
      .strength = editor_gravfield->spec.strength,
      .size = editor_gravfield->spec.size,
      .age = state->total_time * editor_gravfield->spec.strength
    };
    az_draw_gravfield(&real_gravfield);
    if (real_gravfield.strength == 0.0) {
      glColor3f(1, 0, 1);
      draw_gravfield_border(&editor_gravfield->spec);
    }
    draw_script_and_uuid_slot(
        state, real_gravfield.position, editor_gravfield->spec.on_enter,
        editor_gravfield->spec.uuid_slot);
  }
  AZ_LIST_LOOP(editor_baddie, room->baddies) {
    draw_baddie(state, editor_baddie, true);
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
    az_draw_wall(&real_wall, state->clock);
    draw_script_and_uuid_slot(
        state, real_wall.position, NULL, editor_wall->spec.uuid_slot);
  }
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_FG ||
        editor_node->spec.kind == AZ_NODE_DOODAD_BG ||
        editor_node->spec.kind == AZ_NODE_FAKE_WALL_FG ||
        editor_node->spec.kind == AZ_NODE_FAKE_WALL_BG) continue;
    draw_node(state, editor_node);
  }
  AZ_LIST_LOOP(editor_baddie, room->baddies) {
    draw_baddie(state, editor_baddie, false);
  }
  AZ_LIST_LOOP(editor_door, room->doors) {
    const az_door_t real_door = {
      .kind = editor_door->spec.kind,
      .position = editor_door->spec.position,
      .angle = editor_door->spec.angle,
      .is_open = (editor_door->spec.kind == AZ_DOOR_ALWAYS_OPEN),
      .openness = (editor_door->spec.kind == AZ_DOOR_ALWAYS_OPEN ? 1.0 : 0.0)
    };
    az_draw_door(&real_door, state->clock);
    glPushMatrix(); {
      camera_to_screen_orient(state, real_door.position);
      if (editor_door->spec.kind != AZ_DOOR_FORCEFIELD) {
        glColor3f(0, 0, 1); // blue
        az_draw_printf(16, AZ_ALIGN_CENTER, 0, -7, "%03d",
                       editor_door->spec.destination);
      }
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
  AZ_LIST_LOOP(editor_gravfield, room->gravfields) {
    if (!az_is_liquid(editor_gravfield->spec.kind)) continue;
    const az_gravfield_t real_gravfield = {
      .kind = editor_gravfield->spec.kind,
      .position = editor_gravfield->spec.position,
      .angle = editor_gravfield->spec.angle,
      .size = editor_gravfield->spec.size,
      .strength = 1.0, .age = state->total_time
    };
    az_draw_gravfield(&real_gravfield);
    draw_script_and_uuid_slot(
        state, real_gravfield.position, editor_gravfield->spec.on_enter,
        editor_gravfield->spec.uuid_slot);
  }
  AZ_LIST_LOOP(editor_node, room->nodes) {
    if (editor_node->spec.kind == AZ_NODE_DOODAD_FG ||
        editor_node->spec.kind == AZ_NODE_FAKE_WALL_FG) {
      draw_node(state, editor_node);
    }
  }
  draw_camera_edge_bounds(room);
}

static void draw_room_minimap(az_editor_state_t *state,
                              az_editor_room_t *room, az_room_key_t key) {
  if (state->zoom_level >= 32.0) draw_zone_swatch(state, room);

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
        case AZ_DOOR_FORCEFIELD: continue; // don't draw
        case AZ_DOOR_UNLOCKED: glColor3f(1, 1, 0); break;
        case AZ_DOOR_ALWAYS_OPEN: glColor3f(0.8, 0.8, 0.8); break;
        case AZ_DOOR_BOSS: glColor3f(0.5, 0.5, 0.5); break;
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

  draw_camera_edge_bounds(room);

  // Draw room number:
  if (state->zoom_level < 32.0) {
    glPushMatrix(); {
      az_camera_bounds_t *bounds = &room->camera_bounds;
      camera_to_screen_orient(state, az_vpolar(
          bounds->min_r + 0.5 * bounds->r_span,
          bounds->min_theta + 0.5 * bounds->theta_span));
      glScaled(state->zoom_level, state->zoom_level, 1);
      const az_color_t color =
        AZ_LIST_GET(state->planet.zones, room->zone_key)->color;
      glColor3ub(color.r, color.g, color.b);
      az_draw_printf(8, AZ_ALIGN_CENTER, 0, -3, "%03d", key);
    } glPopMatrix();
  } else if (state->zoom_level < 128.0 && room->label != AZ_ERL_NORMAL_ROOM) {
    glPushMatrix(); {
      az_camera_bounds_t *bounds = &room->camera_bounds;
      camera_to_screen_orient(state, az_vpolar(
          bounds->min_r + 0.5 * bounds->r_span,
          bounds->min_theta + 0.5 * bounds->theta_span));
      glScaled(state->zoom_level, state->zoom_level, 1);
      glColor3f(1, 1, 1);
      const char *label = "?";
      switch (room->label) {
        case AZ_ERL_NORMAL_ROOM: AZ_ASSERT_UNREACHABLE();
        case AZ_ERL_BOSS_ROOM:    label = "B"; break;
        case AZ_ERL_COMM_ROOM:    label = "C"; break;
        case AZ_ERL_REFILL_ROOM:  label = "R"; break;
        case AZ_ERL_SAVE_ROOM:    label = "S"; break;
        case AZ_ERL_UPGRADE_ROOM: label = "U"; break;
      }
      az_draw_string(8, AZ_ALIGN_CENTER, 0, -3, label);
    } glPopMatrix();
  }
}

static void draw_selection_circle(az_vector_t position, double angle,
                                  double radius) {
  glPushMatrix(); {
    az_gl_translated(position);
    az_gl_rotated(angle);
    glScaled(radius, radius, 1);
    glColor3f(1, 1, 1); // white
    glBegin(GL_LINE_STRIP); {
      glVertex2f(0, 0);
      for (int i = 0; i <= 360; i += 10) {
        glVertex2d(cos(AZ_DEG2RAD(i)), sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_camera_view(az_editor_state_t *state) {
  glColor3f(1, 1, 0); // yellow
  glBegin(GL_LINE_LOOP); {
    circle_vertices(AZ_PLANETOID_RADIUS);
  } glEnd();

  // Determine bounds of current camera view:
  const double camera_radius = state->zoom_level * AZ_SCREEN_RADIUS;
  const double camera_mid_r = az_vnorm(state->camera);
  const double camera_min_r = fmax(0.0, camera_mid_r - camera_radius);
  const double camera_max_r = camera_mid_r + camera_radius;
  const double camera_theta = az_vtheta(state->camera);
  const double camera_theta_span = (camera_mid_r <= camera_radius ? AZ_TWO_PI :
                                    2.0 * asin(camera_radius / camera_mid_r));

  // Draw other rooms:
  for (int i = 0; i < AZ_LIST_SIZE(state->planet.rooms); ++i) {
    if (i == state->current_room) continue;
    az_editor_room_t *room = AZ_LIST_GET(state->planet.rooms, i);
    // Determine whether the room is (possibly) in view.  If not, don't bother
    // drawing it.
    const az_camera_bounds_t *bounds = &room->camera_bounds;
    if (bounds->min_r + bounds->r_span + AZ_SCREEN_RADIUS < camera_min_r ||
        bounds->min_r - AZ_SCREEN_RADIUS > camera_max_r) continue;
    const double extra_theta_span = camera_theta_span +
      (bounds->min_r <= AZ_SCREEN_RADIUS ? AZ_TWO_PI :
       2.0 * asin(AZ_SCREEN_RADIUS / bounds->min_r));
    if (az_mod2pi_nonneg(camera_theta -
                         (bounds->min_theta - 0.5 * extra_theta_span)) >
        bounds->theta_span + extra_theta_span) continue;
    // Draw the room.
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
  glColor4f(1, 0, 0, 0.75); // red tint
  glBegin(GL_POINTS); {
    az_gl_vertex(state->camera);
  } glEnd();

  // Draw selection circles:
  AZ_LIST_LOOP(wall, room->walls) {
    if (!wall->selected) continue;
    draw_selection_circle(wall->spec.position, wall->spec.angle,
                          wall->spec.data->bounding_radius);
  }
  AZ_LIST_LOOP(gravfield, room->gravfields) {
    if (!gravfield->selected) continue;
    glColor3f(1, 1, 1);
    draw_gravfield_border(&gravfield->spec);
  }
  AZ_LIST_LOOP(node, room->nodes) {
    if (!node->selected) continue;
    draw_selection_circle(node->spec.position, node->spec.angle,
                          AZ_NODE_BOUNDING_RADIUS);
    if (node->spec.kind == AZ_NODE_UPGRADE) {
      const az_upgrade_t upgrade = node->spec.subkind.upgrade;
      glPushMatrix(); {
        az_gl_translated(node->spec.position);
        az_gl_rotated(node->spec.angle);
        glScalef(1, -1, 0);
        glColor3f(1, 1, 1);
        az_draw_printf(8, AZ_ALIGN_CENTER, 0, -24, "%02d", (int)upgrade);
        if (AZ_UPG_CAPACITOR_00 <= upgrade &&
            upgrade <= AZ_UPG_CAPACITOR_MAX) {
          az_draw_printf(8, AZ_ALIGN_CENTER, 0, 17, "%02d",
                         (int)upgrade - (int)AZ_UPG_CAPACITOR_00);
        } else if (AZ_UPG_SHIELD_BATTERY_00 <= upgrade &&
                   upgrade <= AZ_UPG_SHIELD_BATTERY_MAX) {
          az_draw_printf(8, AZ_ALIGN_CENTER, 0, 17, "%02d",
                         (int)upgrade - (int)AZ_UPG_SHIELD_BATTERY_00);
        }
      } glPopMatrix();
    }
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

  // Draw selection sector:
  if (state->selection_sector.active) {
    const double r1 = az_vnorm(state->selection_sector.corner1);
    const double r2 = az_vnorm(state->selection_sector.corner2);
    const double min_r = fmin(r1, r2);
    const double max_r = fmax(r1, r2);
    const double theta0 = az_vtheta(state->selection_sector.corner1);
    const double sweep =
      az_mod2pi(az_vtheta(state->selection_sector.corner2) - theta0);
    glColor3f(0.5, 1, 1);
    glBegin(GL_LINE_LOOP); {
      arc_vertices(max_r, theta0, theta0 + sweep);
      if (min_r > 0.0) arc_vertices(min_r, theta0 + sweep, theta0);
      else glVertex2f(0, 0);
    } glEnd();
  }
}

static void draw_hud(az_editor_state_t *state) {
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
  const az_zone_t *zone = AZ_LIST_GET(state->planet.zones, room->zone_key);
  glColor3ub(zone->color.r, zone->color.g, zone->color.b);
  az_draw_string(8, AZ_ALIGN_LEFT, 150, 5, zone->name);

  // Draw room properties:
  glColor3f(1, 1, 1);
  if (room->properties & AZ_ROOMF_HEATED) {
    az_draw_string(8, AZ_ALIGN_LEFT, 590, 5, "H");
  }
  if (room->properties & AZ_ROOMF_MARK_IF_CLR) {
    az_draw_string(8, AZ_ALIGN_LEFT, 600, 5, "Kc");
  } else if (room->properties & AZ_ROOMF_MARK_IF_SET) {
    az_draw_string(8, AZ_ALIGN_LEFT, 600, 5, "Ks");
  }
  if (room->properties & AZ_ROOMF_UNMAPPED) {
    az_draw_string(8, AZ_ALIGN_LEFT, 620, 5, "U");
  }

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
      glColor3f(0.25, 0.25, 1);
      az_draw_printf(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5,
                     AZ_SCREEN_HEIGHT - 28, "%d/%d",
                     AZ_LIST_SIZE(room->baddies), AZ_MAX_NUM_BADDIES);
      break;
    case AZ_TOOL_DOOR:
      tool_name = "DOOR";
      glColor3f(0, 1, 0);
      az_draw_printf(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5,
                     AZ_SCREEN_HEIGHT - 28, "%d/%d",
                     AZ_LIST_SIZE(room->doors), AZ_MAX_NUM_DOORS);
      break;
    case AZ_TOOL_GRAVFIELD:
      tool_name = "GRAVFIELD";
      glColor3f(0, 0.5, 0.5);
      az_draw_printf(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5,
                     AZ_SCREEN_HEIGHT - 28, "%d/%d",
                     AZ_LIST_SIZE(room->gravfields), AZ_MAX_NUM_GRAVFIELDS);
      break;
    case AZ_TOOL_NODE:
      tool_name = "NODE";
      glColor3f(0, 1, 1);
      az_draw_printf(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5,
                     AZ_SCREEN_HEIGHT - 28, "%d/%d",
                     AZ_LIST_SIZE(room->nodes), AZ_MAX_NUM_NODES);
      break;
    case AZ_TOOL_WALL:
      tool_name = "WALL";
      glColor3f(1, 1, 0);
      az_draw_printf(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5,
                     AZ_SCREEN_HEIGHT - 28, "%d/%d",
                     AZ_LIST_SIZE(room->walls), AZ_MAX_NUM_WALLS);
      break;
  }
  az_draw_string(8, AZ_ALIGN_RIGHT, AZ_SCREEN_WIDTH - 5, AZ_SCREEN_HEIGHT - 13,
                 tool_name);

  // Draw space coords of mouse position:
  int mouse_x, mouse_y;
  if (az_get_mouse_position(&mouse_x, &mouse_y)) {
    const az_vector_t pt = az_pixel_to_position(state, mouse_x, mouse_y);
    glColor3f(1, 1, 1); // white
    az_draw_printf(8, AZ_ALIGN_LEFT, 5, AZ_SCREEN_HEIGHT - 13,
                   "(%.01f, %.01f)", pt.x, pt.y);
  }

  // Draw the unsaved indicator:
  if (state->unsaved) {
    glColor3f(1, 0.5, 0); // orange
    glBegin(GL_TRIANGLES); {
      glVertex2f(5, 5); glVertex2f(5, 15); glVertex2f(15, 5);
      if (room->unsaved) {
        glVertex2f(10, 10); glVertex2f(10, 15); glVertex2f(15, 10);
      }
    } glEnd();
  } else assert(!room->unsaved);

  // Draw the text box:
  if (state->text.action != AZ_ETA_NOTHING) {
    const int num_rows =
      az_imax(1, (state->text.length + EDITOR_TEXT_BOX_CHARS_PER_ROW - 1) /
              EDITOR_TEXT_BOX_CHARS_PER_ROW);
    const GLfloat left = EDITOR_TEXT_BOX_SIDE_MARGIN;
    const GLfloat right = AZ_SCREEN_WIDTH - EDITOR_TEXT_BOX_SIDE_MARGIN;
    const GLfloat top = EDITOR_TEXT_BOX_TOP_MARGIN;
    const GLfloat bottom = top + EDITOR_TEXT_BOX_ROW_HEIGHT * num_rows;
    // Draw box:
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
    // Draw text:
    glColor3f(1, 1, 1); // white
    for (int row = 0; row < num_rows; ++row) {
      const int start = row * EDITOR_TEXT_BOX_CHARS_PER_ROW;
      az_draw_chars(EDITOR_TEXT_BOX_FONT_SIZE, AZ_ALIGN_LEFT,
                    left + EDITOR_TEXT_BOX_PADDING,
                    top + EDITOR_TEXT_BOX_PADDING +
                    EDITOR_TEXT_BOX_ROW_HEIGHT * row,
                    state->text.buffer + start,
                    az_imin(EDITOR_TEXT_BOX_CHARS_PER_ROW,
                            state->text.length - start));
    }
    // Draw cursor:
    if (az_clock_mod(2, 16, state->clock) != 0) {
      int row = state->text.cursor / EDITOR_TEXT_BOX_CHARS_PER_ROW;
      int col = state->text.cursor % EDITOR_TEXT_BOX_CHARS_PER_ROW;
      if (row == num_rows) {
        assert(col == 0);
        row = num_rows - 1;
        col = EDITOR_TEXT_BOX_CHARS_PER_ROW;
      } else assert(row < num_rows);
      glColor3f(1, 0, 0); // red
      glBegin(GL_LINES); {
        const GLfloat x = left + (EDITOR_TEXT_BOX_PADDING - 0.5) +
          EDITOR_TEXT_BOX_FONT_SIZE * col;
        const GLfloat y = top + (EDITOR_TEXT_BOX_PADDING - 0.5) +
          EDITOR_TEXT_BOX_ROW_HEIGHT * row;
        glVertex2f(x, y);
        glVertex2f(x, y + 15);
      } glEnd();
    }
  }
}

void az_editor_draw_screen(az_editor_state_t *state) {
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
    return az_vrotate(pt, az_vtheta(state->camera) - AZ_HALF_PI);
  } else {
    return az_vadd(pt, state->camera);
  }
}

int az_pixel_to_text_box_index(int x, int y) {
  const int row = floor(((double)y - EDITOR_TEXT_BOX_TOP_MARGIN -
                         EDITOR_TEXT_BOX_PADDING) /
                        EDITOR_TEXT_BOX_ROW_HEIGHT);
  const int col = floor(((double)x + 0.5 * EDITOR_TEXT_BOX_FONT_SIZE -
                         EDITOR_TEXT_BOX_SIDE_MARGIN -
                         EDITOR_TEXT_BOX_PADDING) /
                        EDITOR_TEXT_BOX_FONT_SIZE);
  return (EDITOR_TEXT_BOX_CHARS_PER_ROW * row +
          az_imin(az_imax(0, col), EDITOR_TEXT_BOX_CHARS_PER_ROW));
}

/*===========================================================================*/
