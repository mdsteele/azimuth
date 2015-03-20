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

#include "azimuth/view/background.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/background.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

typedef struct {
  double parallax;
  double repeat_horz;
  double repeat_vert;
} az_background_data_t;

static const az_background_data_t background_datas[] = {
  [AZ_BG_BROWN_ROCK_WALL] = {
    .parallax = 0.25, .repeat_horz = 200.0, .repeat_vert = 200.0
  },
  [AZ_BG_GREEN_HEX_TRELLIS] = {
    .parallax = 0.2, .repeat_horz = 180.0, .repeat_vert = 104.0
  },
  [AZ_BG_YELLOW_PANELLING] = {
    .parallax = 0.2, .repeat_horz = 360.0, .repeat_vert = 400.0
  },
  [AZ_BG_PURPLE_ROCK_WALL] = {
    .parallax = 0.25, .repeat_horz = 300.0, .repeat_vert = 300.0
  },
  [AZ_BG_RED_GIRDERS] = {
    .parallax = 0.75, .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GREEN_ROCK_WALL] = {
    .parallax = 0.35, .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GREEN_ROCK_WALL_WITH_GIRDERS] = {
    .parallax = 0.35, .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GRAY_STONE_BRICKS] = {
    .parallax = 0.2, .repeat_horz = 130.0, .repeat_vert = 130.0
  }
};

AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(background_datas) == AZ_NUM_BG_PATTERNS);

static void draw_rivet(az_color_t color1, az_color_t color2,
                       float x, float y) {
  const float r = 3;
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(x, y); az_gl_color(color2);
    glVertex2f(x + r, y); glVertex2f(x, y + r); glVertex2f(x - r, y);
    glVertex2f(x, y - r); glVertex2f(x + r, y);
  } glEnd();
}

static void draw_panel(az_color_t color1, az_color_t color2,
                       float x_min, float y_min, float x_max, float y_max) {
  const float gap = 1;
  const float thick = 5;
  const float space = 20;
  const float hspace = space / 2;
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color1); glVertex2f(x_min + gap, y_min + gap);
    az_gl_color(color2); glVertex2f(x_min + thick, y_min + thick);
    az_gl_color(color1); glVertex2f(x_max - gap, y_min + gap);
    az_gl_color(color2); glVertex2f(x_max - thick, y_min + thick);
    az_gl_color(color1); glVertex2f(x_max - gap, y_max - gap);
    az_gl_color(color2); glVertex2f(x_max - thick, y_max - thick);
    az_gl_color(color1); glVertex2f(x_min + gap, y_max - gap);
    az_gl_color(color2); glVertex2f(x_min + thick, y_max - thick);
    az_gl_color(color1); glVertex2f(x_min + gap, y_min + gap);
    az_gl_color(color2); glVertex2f(x_min + thick, y_min + thick);
  } glEnd();
  for (float x = x_min + hspace; x < x_max; x += space) {
    draw_rivet(color1, color2, x, y_min + hspace);
    draw_rivet(color1, color2, x, y_max - hspace);
  }
  for (float y = y_min + hspace + space; y < y_max - space; y += space) {
    draw_rivet(color1, color2, x_min + hspace, y);
    draw_rivet(color1, color2, x_max - hspace, y);
  }
}

static void draw_rock_wall(az_color_t color1, az_color_t color2,
                           float width, float height) {
  glPushMatrix(); {
    glScalef(width / 200.0f, height / 200.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(-40, 0); az_gl_color(color2);
      glVertex2f(-100, 0); glVertex2f(-30, -30); glVertex2f(20, 0);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(70, 0); az_gl_color(color2);
      glVertex2f(20, 0); glVertex2f(-30, -30); glVertex2f(0, -56);
      glVertex2f(75, -40); glVertex2f(100, 0);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(100, -44); az_gl_color(color2);
      glVertex2f(100, 0); glVertex2f(75, -40); glVertex2f(100, -109);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(-100, -44); az_gl_color(color2);
      glVertex2f(-100, 0); glVertex2f(-30, -30); glVertex2f(0, -56);
      glVertex2f(-40, -85); glVertex2f(-100, -109);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(40, -80); az_gl_color(color2);
      glVertex2f(100, -109); glVertex2f(75, -40); glVertex2f(0, -56);
      glVertex2f(-40, -85); glVertex2f(10, -121); glVertex2f(100, -109);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(-50, -135); az_gl_color(color2);
      glVertex2f(-100, -109); glVertex2f(-57, -170); glVertex2f(-15, -162);
      glVertex2f(10, -121); glVertex2f(-40, -85); glVertex2f(-100, -109);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(-100, -161); az_gl_color(color2);
      glVertex2f(-100, -109); glVertex2f(-57, -170); glVertex2f(-100, -200);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(-40, -200); az_gl_color(color2);
      glVertex2f(-100, -200); glVertex2f(-57, -170); glVertex2f(-15, -162);
      glVertex2f(20, -200);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(40, -160); az_gl_color(color2);
      glVertex2f(100, -109); glVertex2f(10, -121); glVertex2f(-15, -162);
      glVertex2f(20, -200); glVertex2f(65, -179); glVertex2f(100, -109);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(100, -161); az_gl_color(color2);
      glVertex2f(100, -109); glVertex2f(65, -179); glVertex2f(100, -200);
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(color1); glVertex2f(70, -200); az_gl_color(color2);
      glVertex2f(100, -200); glVertex2f(65, -179); glVertex2f(20, -200);
    } glEnd();
  } glPopMatrix();
}

static void draw_girders(az_color_t color1, az_color_t color2,
                         az_color_t color3, float bottom) {
  for (int sign = -1; sign <= 1; sign += 2) {
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(color1);
      glVertex2f(sign * 100, 0); glVertex2f(sign * -100, -185);
      az_gl_color(color3);
      glVertex2f(sign * 100, -15); glVertex2f(sign * -100, -200);
    } glEnd();
  }
  for (int sign = -1; sign <= 1; sign += 2) {
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(color3);
      glVertex2f(sign * 115, 0); glVertex2f(sign * 115, bottom);
      glVertex2f(sign * 105, 0); glVertex2f(sign * 105, bottom);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      az_gl_color(color1);
      glVertex2f(sign * 105, 0); glVertex2f(sign * 105, bottom);
      az_gl_color(color2);
      glVertex2f(sign * 100, 0); glVertex2f(sign * 100, bottom);
    } glEnd();
  }
}

static void draw_half_brick(float width, float height, float rr) {
  const az_color_t color1 = {30, 30, 38, 255};
  const az_color_t color2 = {15, 15, 15, 255};
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color2); glVertex2f(rr, 0); glVertex2f(width, 0);
    az_gl_color(color1); glVertex2f(rr, -rr); glVertex2f(width, -rr);
    glVertex2f(rr, -height + rr); glVertex2f(width, -height + rr);
    az_gl_color(color2); glVertex2f(rr, -height); glVertex2f(width, -height);
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color2); glVertex2f(0, -rr); glVertex2f(0, -height + rr);
    az_gl_color(color1); glVertex2f(rr, -rr); glVertex2f(rr, -height + rr);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(rr, -rr); az_gl_color(color2);
    for (int i = 90; i <= 180; i += 30) {
      glVertex2d(rr + rr * cos(AZ_DEG2RAD(i)), -rr + rr * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(rr, -height + rr); az_gl_color(color2);
    for (int i = 180; i <= 270; i += 30) {
      glVertex2d(rr + rr * cos(AZ_DEG2RAD(i)),
                 -height + rr + rr * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

// Draw one patch of the background pattern.  It should cover the rect from
// <-repeat_horz/2, 0.0> to <repeat_horz/2, -repeat_vert>.
static void draw_bg_patch(az_background_pattern_t pattern, az_clock_t clock) {
  const float bottom = -background_datas[pattern].repeat_vert;
  switch (pattern) {
    case AZ_BG_SOLID_BLACK: break;
    case AZ_BG_BROWN_ROCK_WALL: {
      const az_color_t color1 = {40, 38, 35, 255};
      const az_color_t color2 = {20, 15, 10, 255};
      draw_rock_wall(color1, color2, 200, 200);
    } break;
    case AZ_BG_GREEN_HEX_TRELLIS: {
      const az_color_t color1 = {32, 48, 32, 255};
      const az_color_t color2 = {24, 24, 24, 255};
      const float thick1 = 3.0f;
      const float thick2 = thick1 * (0.25 + 0.5 * sqrt(3));
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(-45 - thick2, 0);
        az_gl_color(color2); glVertex2f(-45 + thick2, 0);
        az_gl_color(color1); glVertex2f(-60 - thick2, -26);
        az_gl_color(color2); glVertex2f(-60 + thick2, -26);
        az_gl_color(color1); glVertex2f(-30 - thick2, -78);
        az_gl_color(color2); glVertex2f(-30 + thick2, -78);
        az_gl_color(color1); glVertex2f(-45 - thick2, -104);
        az_gl_color(color2); glVertex2f(-45 + thick2, -104);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(45 - thick2, 0);
        az_gl_color(color2); glVertex2f(45 + thick2, 0);
        az_gl_color(color1); glVertex2f(60 - thick2, -26);
        az_gl_color(color2); glVertex2f(60 + thick2, -26);
        az_gl_color(color1); glVertex2f(30 - thick2, -78);
        az_gl_color(color2); glVertex2f(30 + thick2, -78);
        az_gl_color(color1); glVertex2f(45 - thick2, -104);
        az_gl_color(color2); glVertex2f(45 + thick2, -104);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1);
        glVertex2f(-90, -26 + thick1); glVertex2f(-61, -26 + thick1);
        az_gl_color(color2);
        glVertex2f(-90, -26 - thick1); glVertex2f(-61, -26 - thick1);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1);
        glVertex2f(90, -26 + thick1); glVertex2f(61, -26 + thick1);
        az_gl_color(color2);
        glVertex2f(90, -26 - thick1); glVertex2f(61, -26 - thick1);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1);
        glVertex2f(-29, -78 + thick1); glVertex2f(29, -78 + thick1);
        az_gl_color(color2);
        glVertex2f(-29, -78 - thick1); glVertex2f(29, -78 - thick1);
      } glEnd();
    } break;
    case AZ_BG_YELLOW_PANELLING: {
      const az_color_t color1 = {60, 45, 30, 255};
      const az_color_t color2 = {30, 20, 10, 255};
      const az_color_t color3 = {40, 35, 20, 255};
      const float horz = 30;
      const float vert = 50;
      draw_panel(color2, color1, -5 * horz, -4 * vert, -1 * horz, 0);
      draw_panel(color2, color1, -1 * horz, -4 * vert,  1 * horz, 0);
      draw_panel(color2, color1, -5 * horz, -6 * vert,  1 * horz, -4 * vert);
      draw_panel(color2, color1,  1 * horz, -2 * vert,  5 * horz, 0);
      draw_panel(color2, color1,  1 * horz, -6 * vert,  5 * horz, -2 * vert);
      draw_panel(color2, color1, -5 * horz, -8 * vert, -3 * horz, -6 * vert);
      draw_panel(color2, color1, -3 * horz, -8 * vert,  5 * horz, -6 * vert);
      for (int sign = -1; sign <= 1; sign += 2) {
        for (int y = bottom + 20; y <= 0; y += 20) {
          glBegin(GL_TRIANGLE_STRIP); {
            az_gl_color(color2);
            glVertex2f(sign * (5 * horz + 2), y);
            glVertex2f(sign * (5 * horz + 2), y - 19);
            az_gl_color(color3);
            glVertex2f(sign * 170, y);
            glVertex2f(sign * 170, y - 19);
            glVertex2f(sign * 180, y);
            glVertex2f(sign * 180, y - 19);
          } glEnd();
        }
      }
    } break;
    case AZ_BG_PURPLE_ROCK_WALL: {
      const az_color_t color1 = {40, 35, 50, 255};
      const az_color_t color2 = {20, 15, 25, 255};
      draw_rock_wall(color1, color2, 300, 300);
    } break;
    case AZ_BG_RED_GIRDERS: {
      const az_color_t color1 = {40, 20, 0, 255};
      const az_color_t color2 = {30, 15, 0, 255};
      const az_color_t color3 = {25, 12, 0, 255};
      draw_girders(color1, color2, color3, bottom);
    } break;
    case AZ_BG_GREEN_ROCK_WALL: {
      const az_color_t color1 = {35, 50, 40, 255};
      const az_color_t color2 = {15, 25, 20, 255};
      draw_rock_wall(color1, color2, 230, 215);
    } break;
    case AZ_BG_GREEN_ROCK_WALL_WITH_GIRDERS: {
      const az_color_t color1 = {35, 50, 40, 255};
      const az_color_t color2 = {15, 25, 20, 255};
      draw_rock_wall(color1, color2, 230, 215);
      const az_color_t color3 = {25, 30, 0, 255};
      const az_color_t color4 = {15, 20, 0, 255};
      const az_color_t color5 = {10, 15, 0, 255};
      draw_girders(color3, color4, color5, bottom);
    } break;
    case AZ_BG_GRAY_STONE_BRICKS: {
      const float half_width = 65;
      const float height = 65;
      const float radius = 8;
      glPushMatrix(); {
        draw_half_brick(half_width, height, radius);
        glTranslatef(-half_width, -height, 0);
        draw_half_brick(half_width, height, radius);
        glTranslatef(half_width, height, 0);
        glScalef(-1, 1, 1);
        draw_half_brick(half_width, height, radius);
        glTranslatef(-half_width, -height, 0);
        draw_half_brick(half_width, height, radius);
      } glPopMatrix();
    } break;
  }
}

void az_draw_background_pattern(
    az_background_pattern_t pattern, const az_camera_bounds_t *camera_bounds,
    az_vector_t camera_center, az_clock_t clock) {
  if (pattern == AZ_BG_SOLID_BLACK) return;
  const int data_index = (int)pattern;
  assert(data_index >= 0 && data_index <= AZ_NUM_BG_PATTERNS);
  const az_background_data_t *data = &background_datas[data_index];
  // Determine the origin point for the background pattern.
  const double base_origin_r =
    camera_bounds->min_r + camera_bounds->r_span + AZ_SCREEN_HEIGHT/2;
  const double base_origin_theta =
    camera_bounds->min_theta + 0.5 * camera_bounds->theta_span;
  // Compute the repeat strides for this background pattern.
  assert(data->repeat_horz > 0.0);
  assert(data->repeat_vert > 0.0);
  const double theta_step = atan(data->repeat_horz / base_origin_r);
  const double r_step = data->repeat_vert;
  // Apply parallax to get a "shifted" origin.
  assert(data->parallax >= 0.0);
  assert(data->parallax <= 1.0);
  const az_vector_t base_origin = az_vpolar(base_origin_r, base_origin_theta);
  const az_vector_t shifted_origin =
    az_vadd(az_vmul(az_vsub(camera_center, base_origin), data->parallax),
            base_origin);
  const double shifted_origin_r = az_vnorm(shifted_origin);
  const double shifted_origin_theta = az_vtheta(shifted_origin);
  // Determine the bounds of the camera rect.
  const az_vector_t halfscreen_horz =
    az_vwithlen(az_vrot90ccw(camera_center), -AZ_SCREEN_WIDTH/2);
  const az_vector_t halfscreen_vert =
    az_vwithlen(camera_center, AZ_SCREEN_HEIGHT/2);
  const az_vector_t topright_corner =
    az_vadd(az_vadd(camera_center, halfscreen_horz), halfscreen_vert);
  const az_vector_t bottomright_corner =
    az_vsub(az_vadd(camera_center, halfscreen_horz), halfscreen_vert);
  const az_vector_t bottomleft_corner =
    az_vsub(az_vsub(camera_center, halfscreen_horz), halfscreen_vert);
  const double min_r = az_vnorm(camera_center) - AZ_SCREEN_HEIGHT/2;
  const double min_theta = az_vtheta(bottomright_corner) - 0.5 * theta_step;
  // Draw copies of the background to cover the whole camera rect.
  const double r_start = shifted_origin_r +
    r_step * ceil((min_r - shifted_origin_r) / r_step);
  const double r_span = az_vnorm(topright_corner) + r_step - r_start;
  const int num_r_steps = ceil(r_span / r_step);
  const double theta_start = shifted_origin_theta +
    theta_step * ceil((min_theta - shifted_origin_theta) / theta_step);
  const double theta_span =
    az_mod2pi_nonneg(az_vtheta(bottomleft_corner) + 0.5 * theta_step -
                     theta_start);
  const int num_theta_steps = ceil(theta_span / theta_step);
  for (int i = 0; i < num_r_steps; ++i) {
    for (int j = 0; j < num_theta_steps; ++j) {
      const az_vector_t position =
        az_vpolar(r_start + i * r_step, theta_start + j * theta_step);
      glPushMatrix(); {
        az_gl_translated(position);
        az_gl_rotated(az_vtheta(position) - AZ_HALF_PI);
        draw_bg_patch(pattern, clock);
      } glPopMatrix();
    }
  }
}

/*===========================================================================*/
