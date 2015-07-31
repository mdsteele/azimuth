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
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

typedef struct {
  double parallax;
  enum {
    POLAR,
    ABS_RECT,
    ROOM_RECT
  } repeat_style;
  double repeat_horz;
  double repeat_vert;
} az_background_data_t;

static const az_background_data_t background_datas[] = {
  [AZ_BG_BROWN_ROCK_WALL] = {
    .parallax = 0.25, .repeat_style = ABS_RECT,
    .repeat_horz = 200.0, .repeat_vert = 200.0
  },
  [AZ_BG_GREEN_HEX_TRELLIS] = {
    .parallax = 0.2, .repeat_style = ABS_RECT,
    .repeat_horz = 180.0, .repeat_vert = 104.0
  },
  [AZ_BG_YELLOW_PANELLING] = {
    .parallax = 0.2, .repeat_style = POLAR,
    .repeat_horz = 360.0, .repeat_vert = 400.0
  },
  [AZ_BG_PURPLE_ROCK_WALL] = {
    .parallax = 0.25, .repeat_style = ABS_RECT,
    .repeat_horz = 300.0, .repeat_vert = 300.0
  },
  [AZ_BG_RED_GIRDERS] = {
    .parallax = 0.75, .repeat_style = POLAR,
    .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GREEN_ROCK_WALL] = {
    .parallax = 0.35, .repeat_style = ABS_RECT,
    .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GREEN_ROCK_WALL_WITH_GIRDERS] = {
    .parallax = 0.15, .repeat_style = POLAR,
    .repeat_horz = 230.0, .repeat_vert = 215.0
  },
  [AZ_BG_GRAY_STONE_BRICKS] = {
    .parallax = 0.2, .repeat_style = POLAR,
    .repeat_horz = 130.0, .repeat_vert = 130.0
  },
  [AZ_BG_RED_GRAY_CINDERBLOCKS] = {
    .parallax = 0.2, .repeat_style = POLAR,
    .repeat_horz = 90.0, .repeat_vert = 118.8
  },
  [AZ_BG_GREEN_BUBBLES] = {
    .parallax = 0.35, .repeat_style = ABS_RECT,
    .repeat_horz = 150.0, .repeat_vert = 150.0
  },
  [AZ_BG_PURPLE_COLUMNS] = {
    .parallax = 0.35, .repeat_style = POLAR,
    .repeat_horz = 140.0, .repeat_vert = 170.0
  },
  [AZ_BG_GRAY_SHALE_ROCK] = {
    .parallax = 0.2, .repeat_style = POLAR,
    .repeat_horz = 200.0, .repeat_vert = 180.0
  },
  [AZ_BG_CRYSTAL_CAVE] = {
    .parallax = 0.25, .repeat_style = ABS_RECT,
    .repeat_horz = 120.0, .repeat_vert = 100.0
  },
  [AZ_BG_ICE_CAVE] = {
    .parallax = 0.25, .repeat_style = ABS_RECT,
    .repeat_horz = 96.0, .repeat_vert = 160.0
  },
  [AZ_BG_PURPLE_BUBBLES] = {
    .parallax = 0.35, .repeat_style = ABS_RECT,
    .repeat_horz = 300.0, .repeat_vert = 300.0
  },
  [AZ_BG_GREEN_DIAMONDS] = {
    .parallax = 0.2, .repeat_style = ROOM_RECT,
    .repeat_horz = 120.0, .repeat_vert = 120.0
  },
  [AZ_BG_VOLCANIC_ROCK] = {
    .parallax = 0.25, .repeat_style = ABS_RECT,
    .repeat_horz = 250.0, .repeat_vert = 250.0
  },
  [AZ_BG_BROWN_TREE_TRUNKS] = {
    .parallax = 0.25, .repeat_style = POLAR,
    .repeat_horz = 200.0, .repeat_vert = 400.0
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

static void draw_half_stone_brick(float width, float height, float rr) {
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

static void draw_half_cinderblock(float width, float height) {
  const az_color_t color1 = {64, 8, 8, 255};
  const az_color_t color2 = {24, 18, 18, 255};
  const float bezel = 13.5;
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color2);
    glVertex2f(0, 0); glVertex2f(width, 0);
    glVertex2f(0, -height); glVertex2f(width, -height);
  } glEnd();
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color1); glVertex2f(width, 0);
    az_gl_color(color2); glVertex2f(width, -bezel);
    az_gl_color(color1); glVertex2f(0, 0);
    az_gl_color(color2); glVertex2f(bezel, -bezel);
    az_gl_color(color1); glVertex2f(0, -height);
    az_gl_color(color2); glVertex2f(bezel, -height + bezel);
    az_gl_color(color1); glVertex2f(width, -height);
    az_gl_color(color2); glVertex2f(width, -height + bezel);
  } glEnd();
}

static void draw_bubble(double center_x, double center_y, double radius,
                        int slowdown, az_clock_t clock, az_color_t inner,
                        az_color_t mid, az_color_t outer) {
  const double mid_radius =
    radius * (0.4 + 0.01 * az_clock_zigzag(20, slowdown, clock));
  glPushMatrix(); {
    glTranslated(center_x, center_y, 0);
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner);
      glVertex2f(-0.1 * radius, 0.1 * radius);
      az_gl_color(mid);
      for (int i = 0; i <= 360; i += 30) {
        glVertex2d(mid_radius * cos(AZ_DEG2RAD(i)),
                   mid_radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 30) {
        az_gl_color(mid);
        glVertex2d(mid_radius * cos(AZ_DEG2RAD(i)),
                   mid_radius * sin(AZ_DEG2RAD(i)));
        az_gl_color(outer);
        glVertex2d(1.5 * radius * cos(AZ_DEG2RAD(i)),
                   radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_green_bubble(double center_x, double center_y, double radius,
                              int slowdown, az_clock_t clock) {
  draw_bubble(center_x, center_y, radius, slowdown, clock,
              (az_color_t){51, 51, 0, 255}, (az_color_t){0, 51, 0, 255},
              (az_color_t){0, 25, 51, 0});
}

static void draw_purple_bubble(double center_x, double center_y, double radius,
                               int slowdown, az_clock_t clock) {
  draw_bubble(center_x, center_y, radius, slowdown, clock,
              (az_color_t){40, 25, 51, 255}, (az_color_t){25, 0, 51, 255},
              (az_color_t){0, 0, 51, 0});
}

static void draw_purple_column(float center_x, double top, float semi_width,
                               float height, bool cap) {
  const az_color_t outer = {48, 32, 64, 255};
  const az_color_t inner = {24, 16, 32, 255};
  if (cap) {
    const float cap_height = 5;
    glBegin(GL_TRIANGLE_FAN); {
      az_gl_color(inner);
      glVertex2f(center_x, top - cap_height);
      az_gl_color(outer);
      glVertex2f(center_x - semi_width, top - cap_height);
      glVertex2f(center_x, top);
      glVertex2f(center_x + semi_width, top - cap_height);
    } glEnd();
    top -= cap_height;
    height -= cap_height;
  }
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(outer);
    glVertex2f(center_x - semi_width, top);
    glVertex2f(center_x - semi_width, top - height);
    az_gl_color(inner);
    glVertex2f(center_x, top);
    glVertex2f(center_x, top - height);
    az_gl_color(outer);
    glVertex2f(center_x + semi_width, top);
    glVertex2f(center_x + semi_width, top - height);
  } glEnd();
}

static void draw_half_shale_rock(float x_0, float y_0, float x_1, float y_1,
                                 float x_2, float y_2, float x_3, float y_3,
                                 float x_4, float y_4, float x_5, float y_5) {
  const az_color_t color1 = {50, 50, 51, 255};
  const az_color_t color2 = {23, 24, 25, 255};
  const az_color_t color3 = {42, 42, 43, 255};
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color3); glVertex2f(0.5f * (x_0 + x_1), 0.5f * (y_0 + y_1));
    az_gl_color(color1); glVertex2f(x_0, y_0);
    az_gl_color(color2); glVertex2f(x_2, y_2); glVertex2f(x_1, y_1);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color3); glVertex2f(0.25f * (x_0 + x_2 + x_3 + x_4),
                                    0.25f * (y_0 + y_2 + y_3 + y_4));
    az_gl_color(color1); glVertex2f(x_0, y_0);
    az_gl_color(color2);
    glVertex2f(x_2, y_2); glVertex2f(x_3, y_3); glVertex2f(x_4, y_4);
    az_gl_color(color1); glVertex2f(x_0, y_0);
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color3); glVertex2f(0.5f * (x_0 + x_5), 0.5f * (y_0 + y_5));
    az_gl_color(color1); glVertex2f(x_0, y_0);
    az_gl_color(color2); glVertex2f(x_4, y_4); glVertex2f(x_5, y_5);
  } glEnd();
}

static void draw_crystal_cell(float x_0, float y_0, float x_1, float y_1,
                              float x_2, float y_2) {
  const az_color_t color1 = {64, 48, 40, 255};
  const az_color_t color2 = {24, 6, 16, 128};
  const az_color_t color3 = {42, 34, 48, 255};
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color3); glVertex2f(0.33333333f * (x_0 + x_1 + x_2),
                                    0.33333333f * (y_0 + y_1 + y_2));
    az_gl_color(color1); glVertex2f(x_0, y_0);
    az_gl_color(color2); glVertex2f(x_1, y_1); glVertex2f(x_2, y_2);
    az_gl_color(color1); glVertex2f(x_0, y_0);
  } glEnd();
}

static void draw_ice_cell(float x_0, float y_0, float x_1, float y_1,
                          float x_2, float y_2) {
  const az_color_t color1 = {32, 72, 72, 255};
  const az_color_t color2 = {16, 48, 32, 255};
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(x_0, y_0);
    az_gl_color(color2); glVertex2f(x_1, y_1); glVertex2f(x_2, y_2);
  } glEnd();
}

static void draw_green_diamond_quarter(float left, float top, float width,
                                       float height, float hbez, float vbez) {
  const az_color_t color1 = {39, 42, 39, 255};
  const az_color_t color2 = {13, 17, 13, 255};
  glBegin(GL_TRIANGLE_STRIP); {
    az_gl_color(color1);
    glVertex2f(left, top);
    glVertex2f(left, top - height + vbez);
    glVertex2f(left + width - hbez, top);
    az_gl_color(color2);
    glVertex2f(left, top - height);
    glVertex2f(left + width, top);
    az_gl_color(color1);
    glVertex2f(left + hbez, top - height);
    glVertex2f(left + width, top - vbez);
    glVertex2f(left + width, top - height);
  } glEnd();
}

static void draw_tree_branch(
    GLfloat x_0, GLfloat y_0, GLfloat x_1, GLfloat y_1,
    GLfloat x_2, GLfloat y_2, GLfloat x_3, GLfloat y_3) {
  const az_color_t color1 = {48, 45, 42, 255};
  const az_color_t color2 = {24, 18, 12, 255};
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(color1); glVertex2f(x_0, y_0); az_gl_color(color2);
    glVertex2f(x_1, y_1); glVertex2f(x_2, y_2); glVertex2f(x_3, y_3);
  } glEnd();
}

// Draw one patch of the background pattern.  It should cover the rect from
// <-repeat_horz/2, 0.0> to <repeat_horz/2, -repeat_vert>.
static void draw_bg_patch(az_background_pattern_t pattern, az_clock_t clock) {
  const float bottom = -background_datas[pattern].repeat_vert;
  switch (pattern) {
    case AZ_BG_SOLID_BLACK: break;
    case AZ_BG_BROWN_ROCK_WALL: {
      const az_color_t color1 = {48, 45, 42, 255};
      const az_color_t color2 = {24, 18, 12, 255};
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
        glTranslatef(-0.5f * half_width, 0, 0);
        draw_half_stone_brick(1.5f * half_width, height, radius);
        glTranslatef(half_width, -height, 0);
        draw_half_stone_brick(0.5f * half_width, height, radius);
        glTranslatef(-half_width, height, 0);
        glScalef(-1, 1, 1);
        draw_half_stone_brick(0.5f * half_width, height, radius);
        glTranslatef(-half_width, -height, 0);
        draw_half_stone_brick(1.5f * half_width, height, radius);
      } glPopMatrix();
    } break;
    case AZ_BG_RED_GRAY_CINDERBLOCKS: {
      const float half_width = 45;
      const float height = 59.4;
      glPushMatrix(); {
        glTranslatef(-0.5f * half_width, 0, 0);
        draw_half_cinderblock(1.5f * half_width, height);
        glTranslatef(half_width, -height, 0);
        draw_half_cinderblock(0.5f * half_width, height);
        glTranslatef(-half_width, height, 0);
        glScalef(-1, 1, 1);
        draw_half_cinderblock(0.5f * half_width, height);
        glTranslatef(-half_width, -height, 0);
        draw_half_cinderblock(1.5f * half_width, height);
      } glPopMatrix();
    } break;
    case AZ_BG_GREEN_BUBBLES: {
      draw_green_bubble(0, -90, 50, 12, clock);
      draw_green_bubble(-40, -25, 40, 10, clock);
      draw_green_bubble(30, -40, 35, 8, clock);
      draw_green_bubble(-55, -120, 32, 6, clock);
      draw_green_bubble(40, -130, 30, 4, clock);
      draw_green_bubble(63, -75, 27, 6, clock + 5);
    } break;
    case AZ_BG_PURPLE_COLUMNS: {
      draw_purple_column(-50, 0, 20, 15, false);
      draw_purple_column(50, 0, 20, 53, false);
      draw_purple_column(0, 0, 20, 35, false);
      draw_purple_column(35, 0, 20, 53, false);
      draw_purple_column(-28, 0, 20, 53, false);
      draw_purple_column(-14, 0, 20, 120, false);
      draw_purple_column(-50, -10, 20, 160, true);
      draw_purple_column(20, -30, 20, 65, true);
      draw_purple_column(50, -48, 20, 122, true);
      draw_purple_column(0, -60, 20, 110, true);
      draw_purple_column(35, -90, 20, 80, true);
      draw_purple_column(-28, -115, 20, 55, true);
      draw_purple_column(-14, -160, 20, 10, true);
    } break;
    case AZ_BG_GRAY_SHALE_ROCK: {
      // Top rock:
      draw_half_shale_rock(100, -30, 100, 0, 20, 0,
                           0, -33, -8, -64, 100, -58);
      draw_half_shale_rock(-100, -30, -100, 0, 20, 0,
                           0, -33, -8, -64, -100, -58);
      // Middle rock:
      draw_half_shale_rock(100, -90, 100, -115, -45, -120,
                           0, -93, 12, -62, 100, -58);
      draw_half_shale_rock(-100, -90,  -100, -115, -45, -120,
                           0, -93, 12, -62, -100, -58);
      // Bottom rock:
      draw_half_shale_rock(-10, -150, 0, -118, -100, -115,
                           -100, -143, -100, -180, 0, -180);
      draw_half_shale_rock(-10, -150, 0, -118, 115, -114,
                           100, -143, 100, -180, 0, -180);
    } break;
    case AZ_BG_CRYSTAL_CAVE: {
      glBegin(GL_TRIANGLE_STRIP); {
        const float gray = 0.003f * az_clock_zigzag(100, 1, clock);
        glColor3f(gray, gray, gray);
        glVertex2f(-60,    0); glVertex2f(60,    0);
        glVertex2f(-60, -100); glVertex2f(60, -100);
      } glEnd();
      draw_crystal_cell(-10, -58, -20, 0, -60, -58);
      draw_crystal_cell(-60, 0, -20, 0, -60, -58);
      draw_crystal_cell(-10, -58, -20, 0, 60, -58);
      draw_crystal_cell(60, 0, -20, 0, 60, -58);
      draw_crystal_cell(-10, -58, -20, -100, -60, -58);
      draw_crystal_cell(-60, -100, -20, -100, -60, -58);
      draw_crystal_cell(-10, -58, -20, -100, 60, -58);
      draw_crystal_cell(60, -100, -20, -100, 60, -58);
    } break;
    case AZ_BG_ICE_CAVE: {
      draw_ice_cell(-8, -93, -16, 0, -48, -93);
      draw_ice_cell(-48, 0, -16, 0, -48, -93);
      draw_ice_cell(-8, -93, -16, 0, 48, -93);
      draw_ice_cell(48, 0, -16, 0, 48, -93);
      draw_ice_cell(-8, -93, -16, -160, -48, -93);
      draw_ice_cell(-48, -160, -16, -160, -48, -93);
      draw_ice_cell(-8, -93, -16, -160, 48, -93);
      draw_ice_cell(48, -160, -16, -160, 48, -93);
    } break;
    case AZ_BG_PURPLE_BUBBLES: {
      glPushMatrix(); {
        glScalef(2, 2, 1);
        draw_purple_bubble(0, -90, 50, 6, clock);
        draw_purple_bubble(-40, -25, 40, 5, clock);
        draw_purple_bubble(30, -40, 35, 4, clock);
        draw_purple_bubble(-55, -120, 32, 3, clock);
        draw_purple_bubble(40, -130, 30, 2, clock);
        draw_purple_bubble(63, -75, 27, 3, clock + 5);
      } glPopMatrix();
    } break;
    case AZ_BG_GREEN_DIAMONDS: {
      draw_green_diamond_quarter(0, 0, 60, 60, 10, 10);
      draw_green_diamond_quarter(0, 0, -60, 60, -10, 10);
      draw_green_diamond_quarter(-60, -60, 60, 60, 10, 10);
      draw_green_diamond_quarter(60, -60, -60, 60, -10, 10);
    } break;
    case AZ_BG_VOLCANIC_ROCK: {
      const az_color_t color1 = {52, 47, 50, 255};
      const az_color_t color2 = {25, 15, 25, 255};
      const az_color_t color3 = {25, 15, 25, 0};
      const az_color_t color4 = {30, 30, 30, 255};
      draw_rock_wall(color1, color2, 250, 250);
      const double xstep = 30.0;
      const double ystep = (sqrt(3) / 3.0) * xstep;
      az_random_seed_t seed = {3, 7};
      bool indent = false;
      for (double y = -10; y > -250; y -= ystep) {
        for (double x = -120; x < 120; x += xstep) {
          const double rx =
            0.25 * xstep * 0.25 * (4.0 + az_rand_sdouble(&seed));
          const double ry =
            0.25 * xstep * 0.25 * (4.0 + az_rand_sdouble(&seed));
          const double cx = x + 0.3 * xstep * az_rand_sdouble(&seed) +
            (indent ? 0.5 * xstep : 0);
          const double cy = y + 0.3 * ystep * az_rand_sdouble(&seed);
          const double theta = AZ_TWO_PI * az_rand_udouble(&seed);
          glBegin(GL_TRIANGLE_FAN); {
            az_gl_color(color4); glVertex2f(cx, cy); az_gl_color(color3);
            for (int i = 0; i <= 360; i += 45) {
              glVertex2d(cx + rx * cos(AZ_DEG2RAD(i) + theta),
                         cy + ry * sin(AZ_DEG2RAD(i) + theta));
            }
          } glEnd();
        }
        indent = !indent;
      }
    } break;
    case AZ_BG_BROWN_TREE_TRUNKS: {
      const az_color_t color1 = {48, 45, 42, 255};
      const az_color_t color2 = {24, 18, 12, 255};
      // Trunks:
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(-100,    0); glVertex2f(-100, -115);
        az_gl_color(color2); glVertex2f( -50,    0); glVertex2f( -50,  -45);
        az_gl_color(color1); glVertex2f(   0,    0); glVertex2f(   0, -115);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(   0,    0); glVertex2f(   0, -180);
        az_gl_color(color2); glVertex2f(  50,    0); glVertex2f(  50, -110);
        az_gl_color(color1); glVertex2f( 100,    0); glVertex2f( 100, -180);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color2); glVertex2f(-100, -115); glVertex2f(-100, -290);
        az_gl_color(color1); glVertex2f( -50,  -45); glVertex2f( -50, -315);
        az_gl_color(color2); glVertex2f(   0, -115); glVertex2f(   0, -245);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color2); glVertex2f(   0, -180); glVertex2f(   0, -310);
        az_gl_color(color1); glVertex2f(  50, -110); glVertex2f(  50, -380);
        az_gl_color(color2); glVertex2f( 100, -180); glVertex2f( 100, -310);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(-100, -290); glVertex2f(-100, -400);
        az_gl_color(color2); glVertex2f( -50, -315); glVertex2f( -50, -400);
        az_gl_color(color1); glVertex2f(   0, -245); glVertex2f(   0, -400);
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        az_gl_color(color1); glVertex2f(   0, -310); glVertex2f(   0, -400);
        az_gl_color(color2); glVertex2f(  50, -380); glVertex2f(  50, -400);
        az_gl_color(color1); glVertex2f( 100, -310); glVertex2f( 100, -400);
      } glEnd();
      // Top-left trunk branches:
      draw_tree_branch( -50, -105,  -50,  -45, -10,   -5,  -15,  -95);
      draw_tree_branch( -50, -105,  -50,  -45, -90,    0,  -75,  -95);
      draw_tree_branch( -60, -130,  -53,  -95, -90,  -35, -100, -125);
      draw_tree_branch( -35, -120,    0, -115,  40,  -45,  -25,  -80);
      // Top-right trunk branches:
      draw_tree_branch(  50, -170,   50, -110,  10,  -40,   25, -150);
      draw_tree_branch(  50, -170,   50, -110,  90,  -55,   75, -150);
      draw_tree_branch(  60, -200,   53, -160,  90, -100,  100, -190);
      draw_tree_branch(  40, -190,    0, -180, -30, -120,   30, -150);
      // Split-bottom trunk branches:
      draw_tree_branch( 100, -370,  100, -290,  60, -240,   75, -350);
      // Mid-bottom trunk branches:
      draw_tree_branch(   0, -350,  -20, -320, -10, -220,   25, -330);
      draw_tree_branch(   0, -350,   20, -320,  85, -285,   45, -370);
      // Split-bottom trunk branches:
      draw_tree_branch(-100, -370, -100, -290, -75, -195,  -65, -310);
      draw_tree_branch( -85, -365,  -70, -310,   5, -240,  -50, -350);
      draw_tree_branch(  85, -380,   50, -385,   0, -305,   80, -345);
      // Mid-bottom trunk branches:
      draw_tree_branch(   0, -350,  -20, -320, -90, -320,  -45, -370);
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
  const az_vector_t topleft_corner =
    az_vadd(az_vsub(camera_center, halfscreen_horz), halfscreen_vert);
  const az_vector_t bottomright_corner =
    az_vsub(az_vadd(camera_center, halfscreen_horz), halfscreen_vert);
  const az_vector_t bottomleft_corner =
    az_vsub(az_vsub(camera_center, halfscreen_horz), halfscreen_vert);
  // Draw copies of the background to cover the whole camera rect.
  switch (data->repeat_style) {
    case POLAR: {
      assert(data->repeat_horz > 0.0);
      assert(data->repeat_vert > 0.0);
      const double theta_step = atan(data->repeat_horz / base_origin_r);
      const double r_step = data->repeat_vert;
      const double min_r = az_vnorm(camera_center) - AZ_SCREEN_HEIGHT/2;
      const double min_theta =
        az_vtheta(bottomright_corner) - 0.5 * theta_step;
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
    } break;
    case ABS_RECT: {
      const double x_step = data->repeat_horz;
      const double y_step = data->repeat_vert;
      assert(x_step > 0.0 && y_step > 0.0);
      const double min_x =
        fmin(fmin(topleft_corner.x, topright_corner.x),
             fmin(bottomleft_corner.x, bottomright_corner.x)) - 0.5 * x_step;
      const double min_y =
        fmin(fmin(topleft_corner.y, topright_corner.y),
             fmin(bottomleft_corner.y, bottomright_corner.y));
      const double max_x =
        fmax(fmax(topleft_corner.x, topright_corner.x),
             fmax(bottomleft_corner.x, bottomright_corner.x)) + 0.5 * x_step;
      const double max_y =
        fmax(fmax(topleft_corner.y, topright_corner.y),
             fmax(bottomleft_corner.y, bottomright_corner.y)) + y_step;
      const double x_start = shifted_origin.x +
        x_step * ceil((min_x - shifted_origin.x) / x_step);
      const double y_start = shifted_origin.y +
        y_step * ceil((min_y - shifted_origin.y) / y_step);
      const int num_x_steps = ceil((max_x - min_x) / x_step);
      const int num_y_steps = ceil((max_y - min_y) / y_step);
      for (int i = 0; i < num_x_steps; ++i) {
        for (int j = 0; j < num_y_steps; ++j) {
          glPushMatrix(); {
            glTranslated(x_start + i * x_step, y_start + j * y_step, 0);
            draw_bg_patch(pattern, clock);
          } glPopMatrix();
        }
      }
    } break;
    case ROOM_RECT: {
      const double i_step = data->repeat_horz;
      const double j_step = data->repeat_vert;
      assert(i_step > 0.0 && j_step > 0.0);
      const az_vector_t unit_i = az_vpolar(1, base_origin_theta - AZ_HALF_PI);
      const az_vector_t unit_j = az_vrot90ccw(unit_i);
      const az_vector_t rel_topright =
        az_vsub(topright_corner, shifted_origin);
      const az_vector_t rel_topleft = az_vsub(topleft_corner, shifted_origin);
      const az_vector_t rel_bottomright =
        az_vsub(bottomright_corner, shifted_origin);
      const az_vector_t rel_bottomleft =
        az_vsub(bottomleft_corner, shifted_origin);
      const double min_i =
        fmin(fmin(az_vdot(rel_topleft, unit_i),
                  az_vdot(rel_topright, unit_i)),
             fmin(az_vdot(rel_bottomleft, unit_i),
                  az_vdot(rel_bottomright, unit_i))) - 0.5 * i_step;
      const double min_j =
        fmin(fmin(az_vdot(rel_topleft, unit_j),
                  az_vdot(rel_topright, unit_j)),
             fmin(az_vdot(rel_bottomleft, unit_j),
                  az_vdot(rel_bottomright, unit_j)));
      const double max_i =
        fmax(fmax(az_vdot(rel_topleft, unit_i),
                  az_vdot(rel_topright, unit_i)),
             fmax(az_vdot(rel_bottomleft, unit_i),
                  az_vdot(rel_bottomright, unit_i))) + 0.5 * i_step;
      const double max_j =
        fmax(fmax(az_vdot(rel_topleft, unit_j),
                  az_vdot(rel_topright, unit_j)),
             fmax(az_vdot(rel_bottomleft, unit_j),
                  az_vdot(rel_bottomright, unit_j))) + j_step;
      const double i_start = i_step * ceil(min_i / i_step);
      const double j_start = j_step * ceil(min_j / j_step);
      const int num_i_steps = ceil((max_i - min_i) / i_step);
      const int num_j_steps = ceil((max_j - min_j) / j_step);
      glPushMatrix(); {
        az_gl_translated(shifted_origin);
        for (int i = 0; i < num_i_steps; ++i) {
          for (int j = 0; j < num_j_steps; ++j) {
            glPushMatrix(); {
              az_gl_translated(az_vadd(az_vmul(unit_i, i_start + i * i_step),
                                       az_vmul(unit_j, j_start + j * j_step)));
              az_gl_rotated(base_origin_theta - AZ_HALF_PI);
              draw_bg_patch(pattern, clock);
            } glPopMatrix();
          }
        }
      } glPopMatrix();
    } break;
  }
}

/*===========================================================================*/
