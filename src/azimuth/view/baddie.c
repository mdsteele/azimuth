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

#include "azimuth/view/baddie.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

static void az_gl_color(az_color_t color) {
  glColor4ub(color.r, color.g, color.b, color.a);
}

static void draw_turret_body_outer_edge(
    const az_baddie_t *baddie, az_color_t far_edge, az_color_t mid_edge) {
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= 360; i += 60) {
      az_gl_color(mid_edge);
      glVertex2d(18 * cos(AZ_DEG2RAD(i)), 18 * sin(AZ_DEG2RAD(i)));
      az_gl_color(far_edge);
      glVertex2d(20 * cos(AZ_DEG2RAD(i)), 20 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

static void draw_turret_body_center(
    const az_baddie_t *baddie, az_color_t mid_edge, az_color_t near_edge,
    az_color_t center) {
  az_gl_color(center);
  glBegin(GL_POLYGON); {
    for (int i = 0; i < 360; i += 60) {
      glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= 360; i += 60) {
      az_gl_color(near_edge);
      glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
      az_gl_color(mid_edge);
      glVertex2d(18 * cos(AZ_DEG2RAD(i)), 18 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

static void draw_turret(const az_baddie_t *baddie,
                        az_color_t far_edge, az_color_t mid_edge,
                        az_color_t near_edge, az_color_t center,
                        az_color_t gun_edge, az_color_t gun_middle) {
  draw_turret_body_outer_edge(baddie, far_edge, mid_edge);
  glPushMatrix(); {
    glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
    glBegin(GL_QUAD_STRIP); {
      az_gl_color(gun_edge);
      glVertex2f( 0,  5); glVertex2f(30,  5);
      az_gl_color(gun_middle);
      glVertex2f( 0,  0); glVertex2f(30,  0);
      az_gl_color(gun_edge);
      glVertex2f( 0, -5); glVertex2f(30, -5);
    } glEnd();
  } glPopMatrix();
  draw_turret_body_center(baddie, mid_edge, near_edge, center);
}

static void draw_atom_electron(double radius, az_vector_t position,
                               double angle) {
  const double cmult = 1.0 + 0.2 * sin(angle);
  const double rmult = 1.0 + 0.08 * sin(angle);
  glPushMatrix(); {
    glTranslated(position.x, position.y, 0);
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(cmult * 0.5, cmult * 0.7, cmult * 0.5); // greenish-gray
      glVertex2d(-1, 1);
      glColor3f(cmult * 0.20, cmult * 0.15, cmult * 0.25); // dark purple-gray
      for (int i = 0; i <= 360; i += 15) {
        glVertex2d(radius * rmult * cos(AZ_DEG2RAD(i)),
                   radius * rmult * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

static void draw_spiner_spine(float flare, float frozen) {
  glBegin(GL_TRIANGLE_STRIP); {
    glColor4f(0.5f * flare, 0.3, 0, 0);
    glVertex2f(-3, 3);
    glColor3f(0.6f + 0.4f * flare, 0.7, 0.6);
    glVertex2f(5, 0);
    glColor3f(0.6f + 0.4f * flare, 0.7, frozen);
    glVertex2f(-5, 0);
    glColor4f(0, 0.3, 0, 0);
    glVertex2f(-3, -3);
  } glEnd();
}

static void draw_box(bool armored, float flare) {
  glBegin(GL_QUADS); {
    if (armored) glColor3f(0.45, 0.45 - 0.3 * flare, 0.65 - 0.3 * flare);
    else glColor3f(0.65, 0.65 - 0.3 * flare, 0.65 - 0.3 * flare); // light gray
    glVertex2f(10, 7); glVertex2f(-10, 7);
    glVertex2f(-10, -7); glVertex2f(10, -7);

    glColor3f(0.2, 0.2, 0.2); // dark gray
    glVertex2f(11, 13); glVertex2f(-11, 13);
    if (armored) glColor3f(0.4, 0.4 - 0.3 * flare, 0.6 - 0.3 * flare);
    else glColor3f(0.6, 0.6 - 0.3 * flare, 0.6 - 0.3 * flare); // gray
    glVertex2f(-10, 7); glVertex2f(10, 7);

    glVertex2f(-10, -7); glVertex2f(-10, 7);
    glColor3f(0.2, 0.2, 0.2); // dark gray
    glVertex2f(-16, 8); glVertex2f(-16, -8);

    glVertex2f(16, -8); glVertex2f(16, 8);
    if (armored) glColor3f(0.4, 0.4 - 0.3 * flare, 0.6 - 0.3 * flare);
    else glColor3f(0.6, 0.6 - 0.3 * flare, 0.6 - 0.3 * flare); // gray
    glVertex2f(10, 7); glVertex2f(10, -7);

    glVertex2f(10, -7); glVertex2f(-10, -7);
    glColor3f(0.2, 0.2, 0.2); // dark gray
    glVertex2f(-11, -13); glVertex2f(11, -13);
  } glEnd();
  glBegin(GL_TRIANGLES); {
    glColor3f(0.3, 0.3 - 0.2 * flare, 0.3 - 0.2 * flare); // dark gray
    glVertex2f( 10,  7); glVertex2f( 11,  13); glVertex2f( 16,   8);
    glVertex2f(-10,  7); glVertex2f(-11,  13); glVertex2f(-16,   8);
    glVertex2f(-10, -7); glVertex2f(-16,  -8); glVertex2f(-11, -13);
    glVertex2f( 10, -7); glVertex2f( 11, -13); glVertex2f( 16,  -8);
  } glEnd();
}

static void draw_zipper(
    az_color_t inner1, az_color_t inner2, az_color_t outer,
    float flare, float frozen, az_clock_t clock) {
  // Body:
  for (int i = -1; i <= 1; i += 2) {
    glBegin(GL_QUAD_STRIP); {
      for (int x = 20; x >= -15; x -= 5) {
        const double y = i * 6.0 * (1 - pow(0.05 * x, 4) + 0.025 * x);
        if (x % 2) az_gl_color(inner1);
        else az_gl_color(inner2);
        glVertex2d(x, 0);
        az_gl_color(outer);
        glVertex2d(x, y);
      }
    } glEnd();
  }
  // Wings:
  for (int i = 0; i < 2; ++i) {
    for (double j = 1.8; j < 4.0; j += 2.0) {
      glPushMatrix(); {
        glTranslatef(10, 0, 0);
        glRotated(j * 6.0 * (1 + az_clock_zigzag(4, 1, clock)),
                  0, 0, 1);
        glBegin(GL_QUAD_STRIP); {
          glColor4f(1 - frozen, 1 - flare, 1 - flare, 0.25);
          glVertex2f(-2.5, 0);
          glColor4f(0.5 - 0.5 * frozen + 0.5 * flare, 1 - flare,
                    1 - flare, 0.35);
          glVertex2f(2.5, 0);
          glVertex2f(-5.5, 14); glVertex2f(5.5, 14);
          glVertex2f(-4.5, 18);
          glColor4f(flare, 1 - flare, 1 - flare, 0.35);
          glVertex2f(4.5, 18);
          glVertex2f(-1, 21); glVertex2f(1, 21);
        } glEnd();
      } glPopMatrix();
    }
    glScalef(1, -1, 1);
  }
}

static void draw_heat_ray(
    const az_baddie_t *baddie, az_clock_t clock, bool lamp,
    az_color_t dark, az_color_t medium, az_color_t light) {
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(dark); glVertex2f(0, 7); glVertex2f(15, 7);
    az_gl_color(light); glVertex2f(0, 0); glVertex2f(15, 0);
    az_gl_color(dark); glVertex2f(0, -7); glVertex2f(15, -7);
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(dark); glVertex2f(-10, 20); glVertex2f(0, 15);
    az_gl_color(light); glVertex2f(-10, 8); glVertex2f(0, 5);
    az_gl_color(dark); glVertex2f(-10, -20); glVertex2f(0, -15);
  } glEnd();
  glPushMatrix(); {
    glTranslatef(2, -7, 0);
    glBegin(GL_TRIANGLE_FAN); {
      if (lamp) glColor3f(1, 0.2, 0.1);
      else glColor3f(0.3, 0.3, 0.3);
      glVertex2f(0, 0);
      glColor3f(0.1, 0.1, 0.1);
      for (int i = 0; i <= 360; i += 20) {
        glVertex2d(5 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_QUAD_STRIP); {
      for (int i = 0; i <= 360; i += 20) {
        az_gl_color(dark);
        glVertex2d(7 * cos(AZ_DEG2RAD(i)), 7 * sin(AZ_DEG2RAD(i)));
        az_gl_color(medium);
        glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/
// Oth baddies:

static const az_vector_t oth_crab_triangles[] = {
  // Body:
  {0, 0}, {15, 10}, {0, 20},
  {15, -10}, {15, 10}, {0, 0},
  {0, -20}, {15, -10}, {0, 0},
  {-15, 7}, {0, 0}, {0, 20},
  {0, 0}, {-15, 7}, {-15, -7},
  {0, -20}, {0, 0}, {-15, -7},
  // Left arm:
  {15, 10}, {5, 16.66}, {24, 17},
  {24, 17}, {5, 16.66}, {22, 25},
  {22, 25}, {24, 17}, {41, 12},
  // Right arm:
  {5, -16.66}, {15, -10}, {24, -17},
  {5, -16.66}, {24, -17}, {22, -25},
  {24, -17}, {22, -25}, {41, -12},
  // Left fang:
  {25, 3}, {15, 10}, {15, 5},
  // Right fang:
  {15, -10}, {25, -3}, {15, -5},
  // Left leg:
  {-5, 16}, {-10, 11}, {-25, 22},
  // Right leg:
  {-10, -11}, {-5, -16}, {-25, -22}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_crab_triangles) % 3 == 0);

static const az_vector_t oth_orb_triangles[] = {
  {0, 0}, {20, 0}, {14.1, 14.1},
  {0, 0}, {14.1, 14.1}, {0, 20},
  {0, 0}, {0, 20}, {-14.1, 14.1},
  {0, 0}, {-14.1, 14.1}, {-20, 0},
  {0, 0}, {-20, 0}, {-14.1, -14.1},
  {0, 0}, {-14.1, -14.1}, {0, -20},
  {0, 0}, {0, -20}, {14.1, -14.1},
  {0, 0}, {14.1, -14.1}, {20, 0}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_orb_triangles) % 3 == 0);

static const az_vector_t oth_snapdragon_triangles[] = {
  // Body:
  {0, 0}, {22.5, 15}, {0, 30},
  {22.5, -15}, {22.5, 15}, {0, 0},
  {0, -30}, {22.5, -15}, {0, 0},
  {-22.5, 15}, {0, 0}, {0, 30},
  {0, 0}, {-22.5, 15}, {-22.5, -15},
  {0, -30}, {0, 0}, {-22.5, -15},
  // Tail:
  {-22.5, 0}, {-22.5, 15}, {-37.5, 7.5},
  {-22.5, -15}, {-22.5, 0}, {-37.5, -7.5},
  // Left arm:
  {22.5, 15}, {6, 48}, {7.5, 24.9},
  // Right arm:
  {6, -48}, {22.5, -15}, {7.5, -24.9},
  // Left legs:
  {0, 30}, {-15, 48}, {-7.5, 24.9},
  {-7.5, 24.9}, {-22.5, 37.5}, {-15, 19.95},
  {-15, 19.95}, {-30, 25.5}, {-22.5, 15},
  // Right legs:
  {0, -30}, {-15, -48}, {-7.5, -24.9},
  {-7.5, -24.9}, {-22.5, -37.5}, {-15, -19.95},
  {-15, -19.95}, {-30, -25.5}, {-22.5, -15},
  // Left pincer:
  {22.5, 3}, {33, 9}, {22.5, 15},
  {33, 9}, {37.5, 19.5}, {22.5, 15},
  {48, 3}, {37.5, 19.5}, {33, 9},
  // Right pincer:
  {33, -9}, {22.5, -3}, {22.5, -15},
  {37.5, -19.5}, {33, -9}, {22.5, -15},
  {37.5, -19.5}, {48, -3}, {33, -9}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_snapdragon_triangles) % 3 == 0);

static const az_vector_t oth_razor_triangles[] = {
  {12, 0}, {1.5, 2.59808}, {1.5, -2.59808},
  {-6, 10.3923}, {-3, 0}, {1.5, 2.59808},
  {-6, -10.3923}, {1.5, -2.59808}, {-3, 0}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_razor_triangles) % 3 == 0);

static const az_vector_t oth_gunship_triangles[] = {
  // Main body:
  {20, 0}, {15, -4}, {5, 0},
  {15, 4}, {20, 0}, {5, 0},
  {-14, 4}, {5, 0}, {15, 4},
  {5, 0}, {-14, -4}, {15, -4},
  {-14, -4}, {5, 0}, {-14, 4},
  // Port strut:
  {-7, 7}, {-7, 4}, {1, 7},
  {1, 4}, {1, 7}, {-7, 4},
  // Starboard strut:
  {-7, -4}, {-7, -7}, {1, -7},
  {1, -7}, {1, -4}, {-7, -4},
  // Port engine:
  {6, 12}, {8, 7}, {-10, 12},
  {-10, 12}, {-11, 7}, {8, 7},
  // Starboard engine:
  {8, -7}, {6, -12}, {-10, -12},
  {-10, -12}, {8, -7}, {-11, -7}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_gunship_triangles) % 3 == 0);

static void draw_oth(
    const az_baddie_t *baddie, GLfloat flare, GLfloat frozen, az_clock_t clock,
    const az_vector_t *vertices, int num_vertices) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  assert(num_vertices % 3 == 0);
  const int num_triangles = num_vertices / 3;
  const bool spin = (baddie->kind != AZ_BAD_OTH_GUNSHIP);
  for (int i = 0; i < num_triangles; ++i) {
    const az_vector_t *vs = vertices + i * 3;
    const az_vector_t center =
      az_vdiv(az_vadd(az_vadd(vs[0], vs[1]), vs[2]), 3);
    glPushMatrix(); {
      glTranslated(center.x, center.y, 0);
      if (spin) glRotated(az_clock_mod(360, 1, clock) -
                          AZ_RAD2DEG(baddie->angle * 8), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        for (int j = 0; j < 3; ++j) {
          const az_clock_t clk = clock + 2 * j;
          const GLfloat r = (az_clock_mod(6, 2, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 2, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 2, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor3f(r + flare * (1.0f - r), (1.0f - 0.5f * flare) * g,
                    (1.0f - flare) * b + frozen * (1.0f - b));
          const az_vector_t rel = az_vsub(vs[j], center);
          glVertex2d(rel.x, rel.y);
        }
      } glEnd();
    } glPopMatrix();
  }
}

/*===========================================================================*/

static void draw_baddie_internal(const az_baddie_t *baddie, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  const float frozen = (baddie->frozen <= 0.0 ? 0.0 :
                        baddie->frozen >= 0.2 ? 0.5 + 0.5 * baddie->frozen :
                        az_clock_mod(3, 2, clock) < 2 ? 0.6 : 0.0);
  if (baddie->frozen > 0.0) clock = 0;
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_MARKER:
      glColor3f(1, 0, 1); // magenta
      glBegin(GL_LINE_STRIP); {
        glVertex2f(-20, 0); glVertex2f(20, 0); glVertex2f(0, 20);
        glVertex2f(0, -20); glVertex2f(20, 0);
      } glEnd();
      break;
    case AZ_BAD_CRAWLING_TURRET:
      glBegin(GL_QUADS); {
        const int zig = az_clock_zigzag(5, 6, clock);
        glColor3f(0.25 + 0.1 * flare - 0.1 * frozen,
                  0.25 - 0.1 * flare - 0.1 * frozen,
                  0.25 - 0.1 * flare + 0.1 * frozen);
        glVertex2f(0, 10); glVertex2f(-21, -10 + zig);
        glColor3f(0.4 + 0.25 * flare - 0.25 * frozen,
                  0.4 - 0.25 * flare - 0.25 * frozen,
                  0.4 - 0.25 * flare + 0.25 * frozen);
        glVertex2f(-20, -20 + zig); glVertex2f(0, -10);
        glVertex2f(0, 10); glVertex2f(-20, 20 - zig);
        glColor3f(0.25 + 0.1 * flare - 0.1 * frozen,
                  0.25 - 0.1 * flare - 0.1 * frozen,
                  0.25 - 0.1 * flare + 0.1 * frozen);
        glVertex2f(-21, 10 - zig); glVertex2f(0, -10);
      } glEnd();
      // fallthrough
    case AZ_BAD_TURRET:
      draw_turret(baddie,
          color3(0.25 + 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare + 0.1 * frozen),
          color3(0.35 + 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare + 0.15 * frozen),
          color3(0.5 + 0.25 * flare - 0.25 * frozen,
                 0.5 - 0.25 * flare - 0.25 * frozen,
                 0.5 - 0.25 * flare + 0.25 * frozen),
          color3(0.6 + 0.4 * flare - 0.3 * frozen,
                 0.6 - 0.3 * flare - 0.3 * frozen,
                 0.6 - 0.3 * flare + 0.4 * frozen),
          color3(0.25 + 0.25 * flare, 0.25, 0.25 + 0.25 * frozen),
          color3(0.75 + 0.25 * flare, 0.75, 0.75 + 0.25 * frozen));
      break;
    case AZ_BAD_ZIPPER:
      draw_zipper(color3(0.5 + 0.5 * flare - 0.5 * frozen, 1 - flare, frozen),
                  color3(0.4 - 0.4 * frozen, 0.4, frozen),
                  color3(0.4 * flare, 0.5, frozen), flare, frozen, clock);
      break;
    case AZ_BAD_BOUNCER:
      glBegin(GL_TRIANGLE_FAN); {
        const unsigned int zig = az_clock_zigzag(15, 1, clock);
        glColor3f(1 - 0.75 * frozen, 0.25 + 0.01 * zig + 0.5 * flare,
                  0.25 + 0.75 * frozen); // red
        glVertex2f(0, 0);
        glColor3f(0.25 + 0.02 * zig - 0.25 * frozen, 0.5 * flare,
                  0.25 * frozen); // dark red
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5, 0.25, 1);
        glVertex2f(0, 7);
        glColor4f(0.5, 0.25, 1, 0);
        glVertex2f(3, 2); glVertex2f(3, 12); glVertex2f(-3, 12);
        glVertex2f(-3, 2); glVertex2f(3, 2);
      } glEnd();
      break;
    case AZ_BAD_ATOM:
      for (int i = 0; i < baddie->data->num_components; ++i) {
        const az_component_t *component = &baddie->components[i];
        if (component->angle >= 0.0) continue;
        draw_atom_electron(baddie->data->components[i].bounding_radius,
                           component->position, component->angle);
      }
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(flare, 1 - 0.5 * flare, frozen); // green
        glVertex2f(-2, 2);
        glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.1 + 0.4 * frozen);
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      for (int i = 0; i < baddie->data->num_components; ++i) {
        const az_component_t *component = &baddie->components[i];
        if (component->angle < 0.0) continue;
        draw_atom_electron(baddie->data->components[i].bounding_radius,
                           component->position, component->angle);
      }
      break;
    case AZ_BAD_SPINER:
      if (baddie->cooldown < 1.0) {
        for (int i = 0; i < 360; i += 45) {
          glPushMatrix(); {
            glRotated(i, 0, 0, 1);
            glTranslated(18.0 - 8.0 * baddie->cooldown, 0, 0);
            draw_spiner_spine(0, frozen);
          } glPopMatrix();
        }
      }
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1 - frozen, 1 - 0.5 * flare, frozen); // yellow
        glVertex2f(-2, 2);
        glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
        for (int i = 0; i <= 360; i += 15) {
          double radius = baddie->data->main_body.bounding_radius +
            0.2 * az_clock_zigzag(10, 3, clock) - 1.0;
          if (i % 45 == 0) radius -= 2.0;
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      for (int i = 0; i < 360; i += 45) {
        glPushMatrix(); {
          glRotated(i + 22.5, 0, 0, 1);
          glTranslated(16 + 0.5 * az_clock_zigzag(6, 5, clock), 0, 0);
          draw_spiner_spine(0, frozen);
        } glPopMatrix();
      }
      for (int i = 0; i < 360; i += 45) {
        glPushMatrix(); {
          glRotated(i + 11.25, 0, 0, 1);
          glTranslated(8 + 0.5 * az_clock_zigzag(6, 7, clock), 0, 0);
          draw_spiner_spine(0, frozen);
        } glPopMatrix();
      }
      break;
    case AZ_BAD_BOX:
      assert(frozen == 0.0);
      draw_box(false, flare);
      break;
    case AZ_BAD_ARMORED_BOX:
      assert(frozen == 0.0);
      draw_box(true, flare);
      break;
    case AZ_BAD_CLAM:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75 - 0.75 * frozen, 0.25 + 0.5 * flare, 0.5); // reddish
        glVertex2d(0.5 * baddie->data->main_body.bounding_radius, 0);
        glColor3f(0.25 - 0.25 * frozen, 0.5 * flare,
                  0.25 * frozen + 0.5 * flare); // dark red
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      for (int i = 0; i < baddie->data->num_components; ++i) {
        glPushMatrix(); {
          const az_component_t *component = &baddie->components[i];
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          az_polygon_t polygon = baddie->data->components[i].polygon;
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.75, 0.25, 1);
            glVertex2f(0, 0);
            glColor3f(0.2 + 0.4 * flare, 0.2 - 0.3 * flare, 0.2 - 0.3 * flare);
            for (int j = 0; j < polygon.num_vertices; ++j) {
              glVertex2d(polygon.vertices[j].x, polygon.vertices[j].y);
            }
          } glEnd();
          glBegin(GL_QUADS); {
            const int n = polygon.num_vertices;
            glVertex2d(polygon.vertices[0].x, polygon.vertices[0].y);
            glVertex2d(polygon.vertices[n - 1].x, polygon.vertices[n - 1].y);
            glColor4f(0.25, 0, 0.5, 0);
            glVertex2d(polygon.vertices[n - 2].x, polygon.vertices[n - 2].y);
            glVertex2d(polygon.vertices[1].x, polygon.vertices[1].y);
          } glEnd();
        } glPopMatrix();
      }
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0.5, 0.4, 0.3, 0);
        glVertex2f(-4, 0);
        for (int i = 135; i <= 225; i += 15) {
          if (i == 225) glColor4f(0.5, 0.4, 0.3, 0);
          glVertex2d(15 * cos(AZ_DEG2RAD(i)), 13 * sin(AZ_DEG2RAD(i)));
          if (i == 135) glColor3f(0.2, 0.15, 0.3);
        }
      } glEnd();
      break;
    case AZ_BAD_NIGHTBUG:
      glPushMatrix(); {
        const double invis = fmax(fmax(baddie->param, flare), frozen);
        assert(0.0 <= invis && invis <= 1.0);
        glBegin(GL_TRIANGLES); {
          glColor4f(0.25, 0.12, frozen, invis); // dark brown
          glVertex2f(-16, 0);
          glColor4f(0.5, 0.2, frozen, invis * invis); // reddish-brown
          glVertex2f(12, 6);
          glVertex2f(12, -6);
        } glEnd();
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(0.8, 0.4, 0.1 + 0.9 * frozen, invis); // light red-brown
          glVertex2f(10, 0);
          glColor4f(0.5, 0.2, frozen, invis * invis); // reddish-brown
          for (int i = -90; i <= 90; i += 30) {
            glVertex2d(10 + 7 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        for (int i = 0; i < 2; ++i) {
          if (i == 1) glScaled(1, -1, 1);
          glColor4f(0.25, 0.12, 0.5 * frozen, invis); // dark brown
          glBegin(GL_TRIANGLES); {
            const GLfloat zig = 0.5f * az_clock_zigzag(8, 3, clock) - 2.0f;
            for (int j = 0; j < 3; ++j) {
              glVertex2f(12 - 7 * j, 7 - j);
              glVertex2f(5 - 7 * j, 7 - j);
              glVertex2f(3 - 7 * j + ((j + i) % 2 ? zig : -zig), 15 - j);
            }
          } glEnd();
          glBegin(GL_TRIANGLE_FAN); {
            glColor4f(0.75 + 0.25 * flare - 0.75 * frozen, 0.5, frozen,
                      fmax(0.08, invis)); // yellow-brown
            glVertex2f(6, 4);
            glColor4f(0.4 + 0.4 * flare - 0.4 * frozen, 0.2, frozen,
                      fmax(0.08, invis * invis * invis)); // brown
            const GLfloat zig = 0.3f * az_clock_zigzag(5, 2, clock);
            glVertex2f(10, 0.25); glVertex2f(13, 3);
            glVertex2f(12, 7); glVertex2f(10, 10);
            glVertex2f(-5, 8 + zig); glVertex2f(-12, 4 + zig);
            glVertex2f(-10, 0.5 + zig); glVertex2f(10, 0.25);
          } glEnd();
        }
      } glPopMatrix();
      break;
    case AZ_BAD_SPINE_MINE:
    case AZ_BAD_URCHIN:
      for (int i = 0; i < 18; ++i) {
        glPushMatrix(); {
          glRotated(i * 20, 0, 0, 1);
          glTranslated(7.5 + 0.5 * az_clock_zigzag(6, 3, clock + i * 7), 0, 0);
          draw_spiner_spine(flare, frozen);
        } glPopMatrix();
      }
      for (int i = 0; i < 9; ++i) {
        glPushMatrix(); {
          glRotated(i * 40 + 30, 0, 0, 1);
          glTranslated(4 + 0.5 * az_clock_zigzag(7, 3, clock - i * 13), 0, 0);
          draw_spiner_spine(flare, frozen);
        } glPopMatrix();
      }
      break;
    case AZ_BAD_BROKEN_TURRET:
      draw_turret(baddie,
          color3(0.30 + 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare - 0.1 * frozen,
                 0.20 - 0.1 * flare + 0.1 * frozen),
          color3(0.40 + 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare - 0.15 * frozen,
                 0.30 - 0.15 * flare + 0.15 * frozen),
          color3(0.55 + 0.25 * flare - 0.25 * frozen,
                 0.50 - 0.25 * flare - 0.25 * frozen,
                 0.45 - 0.25 * flare + 0.25 * frozen),
          color3(0.60 + 0.40 * flare - 0.30 * frozen,
                 0.55 - 0.25 * flare - 0.25 * frozen,
                 0.50 - 0.30 * flare + 0.40 * frozen),
          color3(0.30 + 0.25 * flare, 0.25, 0.20 + 0.25 * frozen),
          color3(0.75 + 0.25 * flare, 0.75, 0.75 + 0.25 * frozen));
      break;
    case AZ_BAD_ZENITH_CORE:
      // TODO: Make real graphics for the Zenith Core.
      glColor3f(1 - frozen, 0, 1 - 0.75 * flare); // magenta
      glBegin(GL_POLYGON); {
        az_polygon_t polygon = baddie->data->main_body.polygon;
        for (int i = 0; i < polygon.num_vertices; ++i) {
          glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
        }
      } glEnd();
      break;
    case AZ_BAD_ARMORED_TURRET:
      draw_turret(baddie,
          color3(0.20 + 0.1 * flare - 0.1 * frozen,
                 0.20 - 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare + 0.1 * frozen),
          color3(0.30 + 0.15 * flare - 0.15 * frozen,
                 0.30 - 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare + 0.15 * frozen),
          color3(0.4 + 0.25 * flare - 0.25 * frozen,
                 0.4 - 0.20 * flare - 0.20 * frozen,
                 0.5 - 0.25 * flare + 0.25 * frozen),
          color3(0.5 + 0.4 * flare - 0.3 * frozen,
                 0.5 - 0.2 * flare - 0.2 * frozen,
                 0.6 - 0.3 * flare + 0.4 * frozen),
          color3(0.2 + 0.25 * flare, 0.2, 0.25 + 0.25 * frozen),
          color3(0.6 + 0.25 * flare, 0.6, 0.75 + 0.25 * frozen));
      break;
    case AZ_BAD_DRAGONFLY:
      // Antennae:
      glColor3f(0.5, 0.25, 0.25);
      glBegin(GL_LINE_STRIP); {
        glVertex2f(23, 4); glVertex2f(16, 0); glVertex2f(23, -4);
      } glEnd();
      // Body:
      for (int i = -1; i <= 1; i += 2) {
        glBegin(GL_QUAD_STRIP); {
          for (int x = 20; x >= -15; x -= 5) {
            if (x % 2) glColor3f(1 - frozen, 0.5 - 0.5 * flare, frozen);
            else glColor3f(1 - frozen, 0.25, frozen);
            const double y = i * 4.0 * (1 - pow(0.05 * x, 4) + 0.025 * x);
            glVertex2d(x, 0);
            glColor3f(0.4 + 0.4 * flare, 0, frozen);
            glVertex2d(x, y);
          }
        } glEnd();
      }
      // Wings:
      for (int i = 0; i < 2; ++i) {
        for (double j = -1.1; j < 2.0; j += 3.0) {
          glPushMatrix(); {
            glTranslatef(5, 0, 0);
            glRotated(j * 6.0 * (1 + az_clock_zigzag(4, 1, clock)),
                      0, 0, 1);
            glBegin(GL_QUAD_STRIP); {
              glColor4f(1 - frozen, 1 - flare, 1 - flare, 0.25);
              glVertex2f(-2.5, 0);
              glColor4f(0.5 - 0.5 * frozen + 0.5 * flare, 1 - flare,
                        1 - flare, 0.35);
              glVertex2f(2.5, 0);
              glVertex2f(-5.5, 14); glVertex2f(5.5, 14);
              glVertex2f(-4.5, 18);
              glColor4f(flare, 1 - flare, 1 - flare, 0.35);
              glVertex2f(4.5, 18);
              glVertex2f(-1, 21); glVertex2f(1, 21);
            } glEnd();
          } glPopMatrix();
        }
        glScalef(1, -1, 1);
      }
      break;
    case AZ_BAD_CAVE_CRAWLER:
      // Feet:
      glBegin(GL_QUADS); {
        const GLfloat offset = 0.8f * (az_clock_zigzag(5, 5, clock) - 2.0f);
        for (int i = 0; i < 4; ++i) {
          glColor3f(0.5f, 0.15f, 0.1f + 0.6f * frozen);
          glVertex2f(0, 5); glVertex2f(0, -5);
          const GLfloat x = (i == 0 || i == 3 ? -18.0f : -20.0f);
          const GLfloat y = -12.0f + 8.0f * i + (2 * (i % 2) - 1) * offset;
          glVertex2f(x, y - 2);
          glColor3f(0.2f, 0.1f, 0.4f + 0.6f * frozen);
          glVertex2f(x, y + 2);
        }
      } glEnd();
      // Body:
      glPushMatrix(); {
        glTranslatef(-0.5f * az_clock_zigzag(5, 5, clock), 0, 0);
        glColor3f(0.3f + 0.7f * flare, 0.2, 0.4 + 0.6f * frozen);
        glBegin(GL_TRIANGLE_FAN); {
          glVertex2f(-15.0f, az_clock_zigzag(9, 3, clock) - 4.0f);
          for (int i = -120; i <= 120; i += 5) {
            if (i % 3 == 0) {
              glColor3f(0.2f + 0.6f * flare, 0, 0.3f + 0.6f * frozen);
            } else glColor3f(0.4, 0, 0.2 + 0.6f * frozen);
            const double rr = 1.0 +
              0.1 * (sin(AZ_DEG2RAD(i) * 2500) +
                     cos(AZ_DEG2RAD(i) * 777 *
                         (1 + az_clock_zigzag(7, 12, clock)))) +
              0.01 * az_clock_zigzag(10, 3, clock);
            glVertex2d(15 * rr * cos(AZ_DEG2RAD(i)) - 3,
                       17 * rr * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_BAD_HORNET:
      // Antennae:
      glColor3f(0.5, 0.5, 0.25);
      glBegin(GL_LINE_STRIP); {
        glVertex2f(23, 4); glVertex2f(16, 0); glVertex2f(23, -4);
      } glEnd();
      // Body:
      for (int i = -1; i <= 1; i += 2) {
        glBegin(GL_QUAD_STRIP); {
          for (int x = 20; x >= -15; x -= 5) {
            if (x % 2) glColor3f(1 - frozen, 1 - flare, frozen);
            else glColor3f(1 - frozen, 0.5, frozen);
            const double y = i * 4.0 * (1 - pow(0.05 * x, 4) + 0.025 * x);
            glVertex2d(x, 0);
            glColor3f(0.4 + 0.4 * flare, 0.4, frozen);
            glVertex2d(x, y);
          }
        } glEnd();
      }
      // Wings:
      for (int i = 0; i < 2; ++i) {
        for (double j = -1.1; j < 2.0; j += 3.0) {
          glPushMatrix(); {
            glTranslatef(5, 0, 0);
            glRotated(j * 6.0 * (1 + az_clock_zigzag(4, 1, clock)),
                      0, 0, 1);
            glBegin(GL_QUAD_STRIP); {
              glColor4f(1 - frozen, 1 - flare, 1 - flare, 0.25);
              glVertex2f(-2.5, 0);
              glColor4f(0.5 - 0.5 * frozen + 0.5 * flare, 1 - flare,
                        1 - flare, 0.35);
              glVertex2f(2.5, 0);
              glVertex2f(-5.5, 14); glVertex2f(5.5, 14);
              glVertex2f(-4.5, 18);
              glColor4f(flare, 1 - flare, 1 - flare, 0.35);
              glVertex2f(4.5, 18);
              glVertex2f(-1, 21); glVertex2f(1, 21);
            } glEnd();
          } glPopMatrix();
        }
        glScalef(1, -1, 1);
      }
      break;
    case AZ_BAD_BEAM_SENSOR:
      glBegin(GL_TRIANGLE_FAN); {
        if (baddie->state == 0) {
          glColor3f(0.75, 0.25 + 0.5 * flare, 0.5); // reddish
        } else glColor3f(0.25 + 0.5 * flare, 0.75, 0.5); // greenish
        glVertex2f(8, 0);
        if (baddie->state == 0) {
          glColor3f(0.25, 0.5 * flare, 0.5 * flare); // dark red
        } else glColor3f(0.25 * flare, 0.25 + 0.25 * flare, 0.5 * flare);
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5, 0.5, 0.5);
        glVertex2f(-5, 0);
        glColor3f(0.2, 0.2, 0.2);
        const az_component_data_t *component = &baddie->data->components[0];
        for (int i = 0, j = component->polygon.num_vertices; i >= 0; i = --j) {
          const az_vector_t vertex = component->polygon.vertices[i];
          glVertex2d(vertex.x, vertex.y);
        }
      } glEnd();
      break;
    case AZ_BAD_ROCKWYRM:
      // Head:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0.5f - 0.5f * flare, 0.1);
        glVertex2f(0, 0);
        glColor3f(0.5f, 0.25f - 0.25f * flare, 0.05);
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Jaws:
      for (int i = 0; i < 2; ++i) {
        const az_component_t *component = &baddie->components[i];
        glPushMatrix(); {
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(1, 0, 1);
            glVertex2f(0, 0);
            glColor3f(0.5f, 0, 0.05);
            const az_polygon_t polygon = baddie->data->components[i].polygon;
            for (int j = polygon.num_vertices - 1, k = 0;
                 j < polygon.num_vertices; j = k++) {
              glVertex2d(polygon.vertices[j].x, polygon.vertices[j].y);
            }
          } glEnd();
        } glPopMatrix();
      }
      // Body/tail:
      for (int i = baddie->data->num_components - 1; i >= 2; --i) {
        const float hurt = 1 - baddie->health / baddie->data->max_health;
        const bool last = i == baddie->data->num_components - 1;
        const az_component_data_t *data = &baddie->data->components[i];
        const az_component_t *component = &baddie->components[i];
        glPushMatrix(); {
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          // Spikes:
          const int limit = (last ? 7 : 5);
          for (int j = -limit; j <= limit; j += 2) {
            if (j == -1 || j == 1) continue;
            glPushMatrix(); {
              glRotated(j * 22.5, 0, 0, 1);
              glBegin(GL_TRIANGLE_FAN); {
                const float radius = data->bounding_radius;
                glColor3f(0.75, 1, 0.25);
                glVertex2f(radius * 0.9, 0);
                glColor3f(0.25, 0.35, 0.125);
                glVertex2f(radius * 0.9, -4);
                glVertex2f(radius * 1.25, 0);
                glVertex2f(radius * 0.9, 4);
              } glEnd();
            } glPopMatrix();
          }
          // Body segment:
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.75, 1.0f - hurt, 0.25 + 0.1f * hurt);
            glVertex2f(0, 0);
            const double radius = data->bounding_radius * 1.05;
            for (int j = 0; j <= 360; j += 30) {
              if (j % (last ? 360 : 180) == 0) {
                glColor3f(0.5f, 0.75f - 0.7f * hurt, 0.15f + 0.1f * hurt);
              } else {
                glColor3f(0.25, 0.35 - 0.3f * hurt, 0.125f);
              }
              glVertex2d(radius * cos(AZ_DEG2RAD(j)),
                         radius * sin(AZ_DEG2RAD(j)));
            }
          } glEnd();
        } glPopMatrix();
      }
      break;
    case AZ_BAD_WYRM_EGG:
      glBegin(GL_TRIANGLE_FAN); {
        const double radius = baddie->data->main_body.bounding_radius;
        if (baddie->state != 1 || frozen > 0.0f ||
            (baddie->cooldown < 1.0 ? az_clock_mod(2, 4, clock) :
             az_clock_mod(2, 8, clock))) {
          glColor3f(0.6f + 0.4f * flare,
                    0.9f - 0.4f * flare - 0.4f * frozen,
                    0.9f - 0.5f * flare);
        } else glColor3f(0, 1, 0.25);
        glVertex2d(0.35 * radius, 0);
        glColor3f(0.3f, 0.4f - 0.2f * flare - 0.2f * frozen,
                  0.4f - 0.2f * flare);
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_BAD_WYRMLING:
      // Head:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0.5f - 0.5f * flare, 0.1);
        glVertex2f(0, 0);
        glColor3f(0.5f, 0.25f - 0.25f * flare, 0.05);
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Body/tail:
      for (int i = baddie->data->num_components - 1; i >= 0; --i) {
        const bool last = i == baddie->data->num_components - 1;
        const az_component_data_t *data = &baddie->data->components[i];
        const az_component_t *component = &baddie->components[i];
        glPushMatrix(); {
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.75, 1, 0.25);
            glVertex2f(0, 0);
            const double radius = data->bounding_radius * 1.05;
            for (int j = 0; j <= 360; j += 30) {
              if (j % (last ? 360 : 180) == 0) glColor3f(0.5, 0.75, 0.15);
              else glColor3f(0.25, 0.35, 0.125);
              glVertex2d(radius * cos(AZ_DEG2RAD(j)),
                         radius * sin(AZ_DEG2RAD(j)));
            }
          } glEnd();
        } glPopMatrix();
      }
      break;
    case AZ_BAD_TRAPDOOR:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.8f - 0.8f * frozen, 0.5, 0.5f + 0.5f * frozen);
        glVertex2f(0, 0);
        glColor3f(0.4f - 0.4f * frozen, 0.2, 0.2f + 0.3f * frozen);
        const az_polygon_t polygon = baddie->data->main_body.polygon;
        for (int i = polygon.num_vertices - 1, j = 0;
             i < polygon.num_vertices; i = j++) {
          glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
        }
      } glEnd();
      glPushMatrix(); {
        assert(!az_vnonzero(baddie->components[0].position));
        glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
        glBegin(GL_POLYGON); {
          const az_component_data_t *data = &baddie->data->components[0];
          glColor3f(0.5f - 0.1f * frozen, 0.5, 0.5f + 0.1f * frozen);
          for (int i = 0; i < data->polygon.num_vertices; ++i) {
            if (i == 2) glColor3f(0.4f - 0.1f * frozen, 0.3,
                                  0.3f + 0.1f * frozen);
            glVertex2d(data->polygon.vertices[i].x,
                       data->polygon.vertices[i].y);
          }
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_BAD_SWOOPER:
      // Feet:
      glBegin(GL_QUADS); {
        for (int i = 0; i < 2; ++i) {
          glColor3f(0.5f, 0.15f, 0.1f + 0.6f * frozen);
          glVertex2f(0, 2); glVertex2f(0, -2);
          const GLfloat x = (baddie->state == 1 ? -20.0f : -17.0f);
          const GLfloat y = -4.0f + 8.0f * i;
          glVertex2f(x, y - 2);
          glColor3f(0.2f, 0.1f, 0.4f + 0.6f * frozen);
          glVertex2f(x, y + 2);
        }
      } glEnd();
      // Body:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.2f + 0.8f * flare, 0.4, 0.3f + 0.7f * frozen);
        glVertex2f(-6, 0);
        glColor3f(0.2 + 0.4f * flare, 0.25, 0.4f * frozen);
        glVertex2f(8, 0); glVertex2f(7, 3);
        glVertex2f(-10, 8); glVertex2f(-16, 6);
        glVertex2f(-14, 0);
        glVertex2f(-16, -6); glVertex2f(-10, -8);
        glVertex2f(7, -3); glVertex2f(8, 0);
      } glEnd();
      // Eyes:
      glBegin(GL_TRIANGLES); {
        glColor3f(0.6, 0, frozen);
        glVertex2f(6, 2); glVertex2f(4, 3); glVertex2f(4, 1);
        glVertex2f(6, -2); glVertex2f(4, -3); glVertex2f(4, -1);
      } glEnd();
      // Wings:
      for (int i = -1; i <= 1; i += 2) {
        const GLfloat expand =
          (baddie->state == 1 ? 0.0f :
           (baddie->state == 0 ? 1.0f : 2.0f) *
           (GLfloat)sin((AZ_PI / 48.0) * az_clock_zigzag(24, 1, clock)));
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.4f + 0.6f * flare, 0.6, 0.4 + 0.6f * frozen);
          glVertex2f(15.0f - 3.5f * expand, i * (2.0f + 6.0f * expand));
          for (int j = 0; j < 7; ++j) {
            if (j % 2) glColor3f(0, 0.2, 0.1);
            else glColor3f(0.2f + 0.4f * flare, 0.5, 0.2f + 0.4f * frozen);
            glVertex2f((j % 2 ? -10.0f : -14.0f) + 0.5f * j,
                       i * (1.0f + (3.0f + expand) * j * j * 0.2f));
          }
        } glEnd();
      }
      break;
    case AZ_BAD_ICE_CRAWLER:
      // Feet:
      glBegin(GL_QUADS); {
        const GLfloat offset = 0.8f * (az_clock_zigzag(5, 6, clock) - 2.0f);
        for (int i = 0; i < 4; ++i) {
          glColor3f(0.5f + 0.5f * flare, 0.15f, 0.5f);
          glVertex2f(0, 5); glVertex2f(0, -5);
          const GLfloat x = (i == 0 || i == 3 ? -19.0f : -20.0f);
          const GLfloat y = -12.0f + 8.0f * i + (2 * (i % 2) - 1) * offset;
          glVertex2f(x, y - 2);
          glColor3f(0.2f + 0.2f * flare, 0.1f, 0.4f);
          glVertex2f(x, y + 2);
        }
      } glEnd();
      // Body:
      glPushMatrix(); {
        glTranslatef(-0.2f * az_clock_zigzag(5, 5, clock), 0, 0);
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.5f + 0.5f * flare, 0.2, 0.4);
          glVertex2f(-15, 0);
          glColor3f(0.4f + 0.6f * flare, 0, 0.2);
          for (int i = -135; i <= 135; i += 5) {
            glVertex2d(13 * cos(AZ_DEG2RAD(i)) - 4, 14 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        // Shell:
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(0.35, 1, 1, 0.6);
          glVertex2f(-4, 0);
          glColor4f(0.06, 0.125, 0.2, 0.8);
          const az_component_data_t *component = &baddie->data->components[0];
          for (int i = 0, j = component->polygon.num_vertices;
               i >= 0; i = --j) {
            const az_vector_t vertex = component->polygon.vertices[i];
            glVertex2d(vertex.x, vertex.y);
          }
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_BAD_BEAM_TURRET:
      draw_turret(baddie,
          color3(0.20 + 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare - 0.1 * frozen,
                 0.20 - 0.1 * flare + 0.1 * frozen),
          color3(0.30 + 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare - 0.15 * frozen,
                 0.30 - 0.15 * flare + 0.15 * frozen),
          color3(0.4 + 0.25 * flare - 0.25 * frozen,
                 0.5 - 0.20 * flare - 0.20 * frozen,
                 0.4 - 0.25 * flare + 0.25 * frozen),
          color3(0.5 + 0.4 * flare - 0.3 * frozen,
                 0.6 - 0.2 * flare - 0.2 * frozen,
                 0.5 - 0.3 * flare + 0.4 * frozen),
          color3(0.2 + 0.25 * flare, 0.25, 0.2 + 0.25 * frozen),
          color3(0.6 + 0.25 * flare, 0.75, 0.6 + 0.25 * frozen));
      break;
    case AZ_BAD_OTH_CRAB_1:
    case AZ_BAD_OTH_CRAB_2:
      draw_oth(baddie, flare, frozen, clock, oth_crab_triangles,
               AZ_ARRAY_SIZE(oth_crab_triangles));
      break;
    case AZ_BAD_OTH_ORB:
      draw_oth(baddie, flare, frozen, clock, oth_orb_triangles,
               AZ_ARRAY_SIZE(oth_orb_triangles));
      break;
    case AZ_BAD_OTH_SNAPDRAGON:
      draw_oth(baddie, flare, frozen, clock, oth_snapdragon_triangles,
               AZ_ARRAY_SIZE(oth_snapdragon_triangles));
      break;
    case AZ_BAD_OTH_RAZOR:
      draw_oth(baddie, flare, frozen, clock, oth_razor_triangles,
               AZ_ARRAY_SIZE(oth_razor_triangles));
      break;
    case AZ_BAD_GUN_SENSOR:
      glBegin(GL_TRIANGLE_FAN); {
        if (baddie->state == 0) {
          glColor3f(0.75, 0.4 + 0.35 * flare, 0.5);
        } else glColor3f(0.25 + 0.5 * flare, 0.75, 0.5);
        glVertex2f(5, 0);
        if (baddie->state == 0) {
          glColor4f(0.25, 0.2 + 0.3 * flare, 0.5 * flare, 0.5);
        } else glColor4f(0.1 + 0.15 * flare, 0.25 + 0.25 * flare,
                         0.5 * flare, 0.5);
        for (int i = 0, j = baddie->data->main_body.polygon.num_vertices;
             i >= 0; i = --j) {
          const az_vector_t v = baddie->data->main_body.polygon.vertices[i];
          glVertex2d(v.x, v.y);
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5, 0.5, 0.5);
        glVertex2f(-7, 0);
        glColor3f(0.2, 0.2, 0.2);
        const az_component_data_t *component = &baddie->data->components[0];
        for (int i = 0, j = component->polygon.num_vertices; i >= 0; i = --j) {
          const az_vector_t vertex = component->polygon.vertices[i];
          glVertex2d(vertex.x, vertex.y);
        }
      } glEnd();
      break;
    case AZ_BAD_SECURITY_DRONE:
      draw_turret_body_outer_edge(baddie,
          color3(0.25 + 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare - 0.1 * frozen,
                 0.25 - 0.1 * flare + 0.1 * frozen),
          color3(0.35 + 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare + 0.15 * frozen));
      glPushMatrix(); {
        glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
        // Gun barrels:
        const az_color_t gun_edge =
          color3(0.25 + 0.25 * flare, 0.25, 0.25 + 0.25 * frozen);
        const az_color_t gun_middle =
          color3(0.75 + 0.25 * flare, 0.75, 0.75 + 0.25 * frozen);
        for (int i = -1; i <= 1; i += 2) {
          glBegin(GL_QUAD_STRIP); {
            az_gl_color(gun_edge);
            glVertex2f( 0,  2 * i); glVertex2f(30,  2 * i);
            az_gl_color(gun_middle);
            glVertex2f( 0,  6 * i); glVertex2f(30,  6 * i);
            az_gl_color(gun_edge);
            glVertex2f( 0, 10 * i); glVertex2f(30, 10 * i);
          } glEnd();
        }
        // Counterweight:
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.25 + 0.1 * flare - 0.1 * frozen,
                    0.25 - 0.1 * flare - 0.1 * frozen,
                    0.25 - 0.1 * flare + 0.1 * frozen);
          glVertex2f(0, 0);
          glVertex2f(-16, 15); glVertex2f(-24, 0); glVertex2f(-16, -15);
        } glEnd();
      } glPopMatrix();
      draw_turret_body_center(baddie,
          color3(0.35 + 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare - 0.15 * frozen,
                 0.35 - 0.15 * flare + 0.15 * frozen),
          color3(0.5 + 0.25 * flare - 0.25 * frozen,
                 0.5 - 0.25 * flare - 0.25 * frozen,
                 0.5 - 0.25 * flare + 0.25 * frozen),
          color3(0.6 + 0.4 * flare - 0.3 * frozen,
                 0.6 - 0.3 * flare - 0.3 * frozen,
                 0.6 - 0.3 * flare + 0.4 * frozen));
      break;
    case AZ_BAD_SMALL_TRUCK:
      // Thruster exhaust:
      if (baddie->state == 1 && frozen == 0.0f) {
        const GLfloat zig = (GLfloat)az_clock_zigzag(10, 1, clock);
        for (int i = -1; i <= 1; i += 2) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor4f(0, 1, 1, 0.9);
            glVertex2f(-30, i * 9);
            glColor4f(0, 0.75, 1, 0);
            glVertex2f(-30, i * 12);
            glVertex2f(-40.0f - zig, i * 9);
            glVertex2f(-30, i * 7);
          } glEnd();
          glBegin(GL_TRIANGLE_FAN); {
            glColor4f(0, 1, 1, 0.9);
            glVertex2f(10, i * 17);
            glColor4f(0, 0.75, 1, 0);
            glVertex2f(10, i * 19);
            glVertex2f(-0.5f * zig, i * 17);
            glVertex2f(10, i * 15);
          } glEnd();
        }
      }
      // Panels:
      glBegin(GL_QUADS); {
        // Front:
        glColor3f(0.2f + 0.8f * flare, 0.25f, 0.25f + 0.75f * frozen);
        glVertex2f(32, -12); glVertex2f(32, 12);
        glColor3f(0.5f + 0.5f * flare, 0.55f, 0.55f + 0.45f * frozen);
        glVertex2f(10, 20); glVertex2f(10, -20);
        // Rear:
        glVertex2f(-30, -14); glVertex2f(-30, 14); glVertex2f(-10, 14);
        glColor3f(0.35f + 0.5f * flare, 0.4f, 0.4f + 0.45f * frozen);
        glVertex2f(-10, -14);
      } glEnd();
      // Body siding:
      glBegin(GL_QUAD_STRIP); {
        const az_color_t outer =
          color3(0.15f + 0.85f * flare, 0.25f, 0.2f + 0.8f * frozen);
        const az_color_t inner =
          color3(0.4f + 0.6f * flare, 0.45f, 0.45f + 0.55f * frozen);
        az_gl_color(outer); glVertex2f(10, 14);
        az_gl_color(inner); glVertex2f(10, 9);
        az_gl_color(outer); glVertex2f(-30, 14);
        az_gl_color(inner); glVertex2f(-25, 9);
        az_gl_color(outer); glVertex2f(-30, -14);
        az_gl_color(inner); glVertex2f(-25, -9);
        az_gl_color(outer); glVertex2f(10, -14);
        az_gl_color(inner); glVertex2f(10, -9);
      } glEnd();
      // Cab siding:
      glBegin(GL_TRIANGLES); {
        // Left side:
        glColor3f(0.15f + 0.85f * flare, 0.2f, 0.2f + 0.8f * frozen);
        glVertex2f(32, 12);
        glVertex2f(10, 20);
        glColor3f(0.55f + 0.45f * flare, 0.55f, 0.55f + 0.45f * frozen);
        glVertex2f(10, 8);
        // Right side:
        glVertex2f(10, -8);
        glColor3f(0.15f + 0.85f * flare, 0.2f, 0.2f + 0.8f * frozen);
        glVertex2f(32, -12);
        glVertex2f(10, -20);
      } glEnd();
      break;
    case AZ_BAD_HEAT_RAY:
      assert(frozen == 0.0);
      draw_heat_ray(baddie, clock, baddie->state == 0 ||
                    (baddie->cooldown <= 1.0 && az_clock_mod(2, 4, clock)),
                    color3(0.35 + 0.25 * flare, 0.25, 0.25),
                    color3(0.55 + 0.25 * flare, 0.45, 0.45),
                    color3(0.85 + 0.15 * flare, 0.65, 0.65));
      break;
    case AZ_BAD_NUCLEAR_MINE:
      // Arms:
      glPushMatrix(); {
        for (int i = 0; i < 3; ++i) {
          glBegin(GL_QUADS); {
            glColor3f(0.55 + 0.4 * flare, 0.55, 0.5 + 0.5 * frozen);
            glVertex2f(0, 1.5); glVertex2f(18, 1.5);
            glColor3f(0.35 + 0.3 * flare, 0.35, 0.3 + 0.3 * frozen);
            glVertex2f(18, -1.5); glVertex2f(0, -1.5);
          } glEnd();
          glRotatef(120, 0, 0, 1);
        }
      } glPopMatrix();
      // Body:
      glBegin(GL_POLYGON); {
        glColor3f(0.65 + 0.3 * flare - 0.3 * frozen, 0.65 - 0.3 * flare,
                  0.5 - 0.3 * flare + 0.5 * frozen);
        for (int i = 0; i <= 360; i += 60) {
          glVertex2d(8 * cos(AZ_DEG2RAD(i)), 8 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        for (int i = 0; i <= 360; i += 60) {
          glColor3f(0.65 + 0.3 * flare - 0.3 * frozen, 0.65 - 0.3 * flare,
                    0.5 - 0.3 * flare + 0.5 * frozen);
          glVertex2d(8 * cos(AZ_DEG2RAD(i)), 8 * sin(AZ_DEG2RAD(i)));
          glColor3f(0.35f + 0.3f * flare, 0.35f, 0.15f + 0.3f * frozen);
          glVertex2d(12 * cos(AZ_DEG2RAD(i)), 12 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Radiation symbol:
      if (baddie->state == 1 && az_clock_mod(2, 3, clock)) glColor3f(1, 0, 0);
      else glColor3f(0, 0, 0.5f * frozen);
      glBegin(GL_TRIANGLE_FAN); {
        glVertex2d(0, 0);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(1.5 * cos(AZ_DEG2RAD(i)), 1.5 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      for (int j = 60; j < 420; j += 120) {
        glBegin(GL_QUAD_STRIP); {
          for (int i = j - 30; i <= j + 30; i += 10) {
            glVertex2d(3 * cos(AZ_DEG2RAD(i)), 3 * sin(AZ_DEG2RAD(i)));
            glVertex2d(8 * cos(AZ_DEG2RAD(i)), 8 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      }
      break;
    case AZ_BAD_BEAM_WALL:
      glBegin(GL_QUADS); {
        // Interior:
        glColor3f(0.3, 0.3, 0.3);
        glVertex2f(50, 15); glVertex2f(-50, 15);
        glVertex2f(-50, -15); glVertex2f(50, -15);
        // Diagonal struts:
        for (int i = 0; i < 3; ++i) {
          const float x = -50 + 32 * i;
          for (int j = 0; j < 2; ++j) {
            const float y = (j ? -12 : 12);
            glColor3f(0.75 + 0.25 * flare, 0.75, 0.75);
            glVertex2f(x, y); glVertex2f(x + 32, -y);
            glColor3f(0.35 + 0.35 * flare, 0.4, 0.35);
            glVertex2f(x + 32 + 4, -y); glVertex2f(x + 4, y);
          }

        }
        // Top and bottom struts:
        for (int y = -10; y <= 15; y += 25) {
          glColor3f(0.75 + 0.25 * flare, 0.75, 0.75);
          glVertex2f(52, y); glVertex2f(-52, y);
          glColor3f(0.35 + 0.35 * flare, 0.4, 0.35);
          glVertex2f(-52, y - 5); glVertex2f(52, y - 5);
        }
      } glEnd();
      break;
    case AZ_BAD_SPARK:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.8);
        glVertex2f(0, 0);
        glColor4f(0, 1, 0, 0);
        for (int i = 0; i <= 360; i += 45) {
          const double radius =
            (i % 2 ? 1.0 : 0.5) * (8.0 + 0.25 * az_clock_zigzag(8, 3, clock));
          const double theta = AZ_DEG2RAD(i + 7 * az_clock_mod(360, 1, clock));
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      break;
    case AZ_BAD_MOSQUITO:
      // Antennae:
      glColor3f(0.5, 0.25, 0.25);
      glBegin(GL_LINE_STRIP); {
        glVertex2f(11.5, 2); glVertex2f(8, 0); glVertex2f(11.5, -2);
      } glEnd();
      // Body:
      for (int i = -1; i <= 1; i += 2) {
        glBegin(GL_QUAD_STRIP); {
          for (int x = 20; x >= -15; x -= 5) {
            if (x % 2) glColor3f(1 - frozen, 0.5 - 0.5 * flare, frozen);
            else glColor3f(1 - frozen, 0.25, frozen);
            const double y = i * 8.0 * (1 - pow(0.05 * x, 4) + 0.025 * x);
            glVertex2d(0.5 * x, 0);
            glColor3f(0.4 + 0.4 * flare, 0, frozen);
            glVertex2d(0.5 * x, 0.5 * y);
          }
        } glEnd();
      }
      // Wings:
      for (int i = 0; i < 2; ++i) {
        for (double j = -1.1; j < 2.0; j += 3.0) {
          glPushMatrix(); {
            glTranslatef(2.5, 0, 0);
            glRotated(j * 6.0 * (1 + az_clock_zigzag(4, 1, clock)),
                      0, 0, 1);
            glBegin(GL_QUAD_STRIP); {
              glColor4f(1 - frozen, 1 - flare, 1 - flare, 0.25);
              glVertex2f(-1.25, 0);
              glColor4f(0.5 - 0.5 * frozen + 0.5 * flare, 1 - flare,
                        1 - flare, 0.35);
              glVertex2f(1.25, 0);
              glVertex2f(-2.75, 7); glVertex2f(2.75, 7);
              glVertex2f(-2.25, 9);
              glColor4f(flare, 1 - flare, 1 - flare, 0.35);
              glVertex2f(2.25, 9);
              glVertex2f(-0.5, 10.5); glVertex2f(0.5, 10.5);
            } glEnd();
          } glPopMatrix();
        }
        glScalef(1, -1, 1);
      }
      break;
    case AZ_BAD_ARMORED_ZIPPER:
      draw_zipper(color3(0.7 + 0.25 * flare - 0.5 * frozen, 0.75 - flare,
                         0.7 + 0.3 * frozen),
                  color3(0, 0.4, 0.4 + 0.6 * frozen),
                  color3(0.2 + 0.4 * flare, 0.3, 0.2 + 0.8 * frozen),
                  flare, frozen, clock);
      break;
    case AZ_BAD_FORCEFIEND:
      // TODO: Make real graphics for the Forcefiend.
      glColor3f(0.5 + 0.5 * flare, 0, 1 - 0.5 * flare);
      glBegin(GL_LINE_LOOP); {
        const az_polygon_t poly = baddie->data->main_body.polygon;
        for (int i = 0; i < poly.num_vertices; ++i) {
          glVertex2d(poly.vertices[i].x, poly.vertices[i].y);
        }
      } glEnd();
      for (int j = 0; j < baddie->data->num_components; ++j) {
        glPushMatrix(); {
          const az_component_t *component = &baddie->components[j];
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          glBegin(GL_LINE_LOOP); {
            const az_polygon_t poly = baddie->data->components[j].polygon;
            for (int i = 0; i < poly.num_vertices; ++i) {
              glVertex2d(poly.vertices[i].x, poly.vertices[i].y);
            }
          } glEnd();
        } glPopMatrix();
      }
      break;
    case AZ_BAD_STALK_PLANT:
      // Stalk:
      for (int j = 3; j < baddie->data->num_components; ++j) {
        glPushMatrix(); {
          const az_component_t *component = &baddie->components[j];
          glTranslated(component->position.x, component->position.y, 0);
          glRotated(AZ_RAD2DEG(component->angle), 0, 0, 1);
          // Thorns:
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.75, 0.75, 0.75); glVertex2f(0, 0);
            glColor3f(0.3, 0.3, 0.3); glVertex2f(0, 8);
            glVertex2f(-4, 0); glVertex2f(0, -8); glVertex2f(4, 0);
          } glEnd();
          // Segment:
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
            glVertex2f(-12, 4); glVertex2f(12, 4);
            glColor3f(0.5 - 0.5 * frozen, 1 - 0.5 * flare, frozen);
            glVertex2f(-15, 0); glVertex2f(15, 0);
            glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
            glVertex2f(-12, -4); glVertex2f(12, -4);
          } glEnd();
        } glPopMatrix();
      }
      // Base of stalk:
      glPushMatrix(); {
        const az_component_t *base = &baddie->components[0];
        glTranslated(base->position.x, base->position.y, 0);
        glRotated(AZ_RAD2DEG(base->angle), 0, 0, 1);
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.5 - 0.5 * frozen, 1 - 0.5 * flare, frozen);
          glVertex2f(0, 0);
          glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
          glVertex2f(0, -14); glVertex2f(8, -11); glVertex2f(14, -4);
          glColor3f(0.5 - 0.5 * frozen, 1 - 0.5 * flare, frozen);
          glVertex2f(14, 0);
          glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
          glVertex2f(14, 4); glVertex2f(8, 11); glVertex2f(0, 14);
        } glEnd();
      } glPopMatrix();
      // Inside mouth:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75 - 0.75 * frozen, 0.25 + 0.5 * flare, 0.5); // reddish
        glVertex2d(0.5 * baddie->data->main_body.bounding_radius, 0);
        glColor3f(0.25 - 0.25 * frozen, 0.5 * flare,
                  0.25 * frozen + 0.5 * flare); // dark red
        const double radius = baddie->data->main_body.bounding_radius;
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Teeth:
      for (int i = 1; i <= 2; ++i) {
        glPushMatrix(); {
          const az_component_t *pincer = &baddie->components[i];
          glTranslated(pincer->position.x, pincer->position.y, 0);
          glRotated(AZ_RAD2DEG(pincer->angle), 0, 0, 1);
          glBegin(GL_TRIANGLES); {
            glColor3f(0.5, 0.5, 0.5);
            const GLfloat y = (i % 2 ? -4 : 4);
            for (GLfloat x = 25 - 3 * i; x > 5; x -= 6) {
              glVertex2d(x + 2, 0); glVertex2d(x, y); glVertex2d(x - 2, 0);
            }
          } glEnd();
        } glPopMatrix();
      }
      // Pincers:
      glColor3f(0.25 + 0.75 * flare, 1 - 0.5 * flare, frozen);
      for (int i = 1; i <= 2; ++i) {
        glPushMatrix(); {
          const az_component_t *pincer = &baddie->components[i];
          glTranslated(pincer->position.x, pincer->position.y, 0);
          glRotated(AZ_RAD2DEG(pincer->angle), 0, 0, 1);
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5 - 0.5 * frozen, 1 - 0.5 * flare, frozen);
            glVertex2f(0, 0);
            glColor3f(0.4 * flare, 0.3 - 0.3 * flare, 0.4 * frozen);
            const az_polygon_t poly = baddie->data->components[i].polygon;
            for (int j = 0, k = poly.num_vertices; j >= 0; j = --k) {
              glVertex2d(poly.vertices[j].x, poly.vertices[j].y);
            }
          } glEnd();
        } glPopMatrix();
      }
      break;
    case AZ_BAD_COPTER:
      // Rotor blades:
      glBegin(GL_QUADS); {
        GLfloat y = 6 * az_clock_zigzag(5, 1, clock);
        glColor4f(0.5, 0.5, 0.5, 0.6);
        glVertex2f(-16, y); glVertex2f(-18, y);
        glVertex2f(-18, -y); glVertex2f(-16, -y);
        y = 6 * az_clock_zigzag(5, 1, clock + 2);
        glVertex2f(-19, y); glVertex2f(-21, y);
        glVertex2f(-21, -y); glVertex2f(-19, -y);
      } glEnd();
      // Panels:
      glBegin(GL_QUADS); {
        // Top:
        glColor3f(0.2f + 0.8f * flare, 0.25f, 0.25f + 0.75f * frozen);
        glVertex2f(10, 6);
        glColor3f(0.5f + 0.5f * flare, 0.55f, 0.55f + 0.45f * frozen);
        glVertex2f(10, 17); glVertex2f(-10, 17); glVertex2f(-10, 6);
        // Bottom:
        glVertex2f(-10, -17); glVertex2f(10, -17); glVertex2f(10, -6);
        glColor3f(0.35f + 0.5f * flare, 0.4f, 0.4f + 0.45f * frozen);
        glVertex2f(-10, -6);
      } glEnd();
      {
        const az_color_t outer =
          color3(0.15f + 0.85f * flare, 0.25f, 0.2f + 0.8f * frozen);
        const az_color_t inner =
          color3(0.4f + 0.6f * flare, 0.45f, 0.45f + 0.55f * frozen);
        // Rotor hub:
        glBegin(GL_QUAD_STRIP); {
          az_gl_color(outer); glVertex2f(-21, 2); glVertex2f(-14, 2);
          az_gl_color(inner); glVertex2f(-22, 0); glVertex2f(-14, 0);
          az_gl_color(outer); glVertex2f(-21, -2); glVertex2f(-14, -2);
        } glEnd();
        // Body siding:
        glBegin(GL_QUAD_STRIP); {
          az_gl_color(outer); glVertex2f(14, 21);
          az_gl_color(inner); glVertex2f(10, 17);
          az_gl_color(outer); glVertex2f(-14, 21);
          az_gl_color(inner); glVertex2f(-10, 17);
          az_gl_color(outer); glVertex2f(-14, -21);
          az_gl_color(inner); glVertex2f(-10, -17);
          az_gl_color(outer); glVertex2f(14, -21);
          az_gl_color(inner); glVertex2f(10, -17);
          az_gl_color(outer); glVertex2f(14, 21);
          az_gl_color(inner); glVertex2f(10, 17);
        } glEnd();
      }
      break;
    case AZ_BAD_BOSS_DOOR:
      // Eye:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(1, 0.75 - 0.75 * flare, 0.5 - 0.5 * flare);
        glVertex2f(8, 8);
        glColor3f(0.8, 0.4 - 0.4 * flare, 0);
        const double radius = baddie->data->components[0].bounding_radius;
        for (int i = 0; i <= 360; i += 10) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glPushMatrix(); {
        glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
        glTranslatef(15, 0, 0);
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(baddie->param, 0, 0.25, 1);
          glVertex2f(0, 0);
          glColor4f(baddie->param, 0, 0.25, 0.6);
          for (int i = 0; i <= 360; i += 10) {
            glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
      } glPopMatrix();
      // Eyelids:
      for (int i = 1; i <= 2; ++i) {
        glPushMatrix(); {
          glRotated(AZ_RAD2DEG(baddie->components[i].angle), 0, 0, 1);
          glBegin(GL_TRIANGLE_FAN); {
            const az_polygon_t poly = baddie->data->components[i].polygon;
            glColor3f(0.75, 0.75, 0.75);
            glVertex2d(poly.vertices[0].x, poly.vertices[0].y);
            glColor3f(0.25, 0.25, 0.25);
            for (int j = 1; j < poly.num_vertices; ++j) {
              glVertex2d(poly.vertices[j].x, poly.vertices[j].y);
            }
          } glEnd();
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0.2, 0.2, 0.2);
            glVertex2f(22, 0); glVertex2f(22, 1);
            glColor3f(0.4, 0.4, 0.4);
            glVertex2f(0, 0); glVertex2f(2, 1);
            glColor3f(0.2, 0.2, 0.2);
            glVertex2f(0, 22); glVertex2f(1, 22);
          } glEnd();
        } glPopMatrix();
      }
      // Body:
      glBegin(GL_TRIANGLES); {
        glColor3f(0.15, 0.15, 0.15);
        glVertex2f(4, 28); glVertex2f(2, 32); glVertex2f(7, 32.5);
        glVertex2f(4, -28); glVertex2f(2, -32); glVertex2f(7, -32.5);
        glVertex2f(-8, 43); glVertex2f(-12, 45); glVertex2f(-7.5, 49);
        glVertex2f(-8, -43); glVertex2f(-12, -45); glVertex2f(-7.5, -49);
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        const az_polygon_t poly = baddie->data->main_body.polygon;
        const int half = poly.num_vertices / 2;
        glColor3f(0.6, 0.6, 0.6);
        glVertex2f(-20, 0);
        glColor3f(0.2, 0.2, 0.2);
        for (int i = 0; i < half; ++i) {
          glVertex2d(poly.vertices[i].x, poly.vertices[i].y);
        }
        glColor3f(0.4, 0.4, 0.4);
        glVertex2f(7, 0);
        glColor3f(0.2, 0.2, 0.2);
        for (int i = half; i < poly.num_vertices; ++i) {
          glVertex2d(poly.vertices[i].x, poly.vertices[i].y);
        }
      } glEnd();
      glBegin(GL_QUAD_STRIP); {
        glColor3f(0.2, 0.2, 0.2); glVertex2f(4, 23); glVertex2f(8, 22);
        glColor3f(0.6, 0.6, 0.6); glVertex2f(4, 0); glVertex2f(8, 0);
        glColor3f(0.2, 0.2, 0.2); glVertex2f(4, -23); glVertex2f(8, -22);
      } glEnd();
      break;
    case AZ_BAD_ROCKET_TURRET:
      draw_turret(baddie,
          color3(0.20 + 0.1 * flare - 0.1 * frozen,
                 0.15 - 0.1 * flare - 0.1 * frozen,
                 0.15 - 0.1 * flare + 0.1 * frozen),
          color3(0.30 + 0.15 * flare - 0.15 * frozen,
                 0.25 - 0.15 * flare - 0.15 * frozen,
                 0.25 - 0.15 * flare + 0.15 * frozen),
          color3(0.4 + 0.25 * flare - 0.25 * frozen,
                 0.3 - 0.20 * flare - 0.20 * frozen,
                 0.3 - 0.25 * flare + 0.25 * frozen),
          color3(0.5 + 0.4 * flare - 0.3 * frozen,
                 0.4 - 0.2 * flare - 0.2 * frozen,
                 0.4 - 0.3 * flare + 0.4 * frozen),
          color3(0.2 + 0.25 * flare, 0.15, 0.15 + 0.25 * frozen),
          color3(0.65 + 0.25 * flare, 0.5, 0.5 + 0.25 * frozen));
      break;
    case AZ_BAD_MINI_ARMORED_ZIPPER:
      glPushMatrix(); {
        glScalef(0.7, 0.7, 1);
        draw_zipper(color3(0.7 + 0.25 * flare - 0.5 * frozen, 0.75 - flare,
                           0.7 + 0.3 * frozen),
                    color3(0, 0.4, 0.4 + 0.6 * frozen),
                    color3(0.2 + 0.4 * flare, 0.3, 0.2 + 0.8 * frozen),
                    flare, frozen, clock);
      } glPopMatrix();
      break;
    case AZ_BAD_SPINED_CRAWLER:
      // Feet:
      glBegin(GL_QUADS); {
        const GLfloat offset = 0.8f * (baddie->state == 3 ? 0.0f :
                                       (az_clock_zigzag(5, 5, clock) - 2.0f));
        for (int i = 0; i < 4; ++i) {
          glColor3f(0.5f, 0.15f, 0.1f + 0.6f * frozen);
          glVertex2f(0, 5); glVertex2f(0, -5);
          const GLfloat x = (i == 0 || i == 3 ? -18.0f : -20.0f);
          const GLfloat y = -12.0f + 8.0f * i + (2 * (i % 2) - 1) * offset;
          glVertex2f(x, y - 2);
          glColor3f(0.2f, 0.1f, 0.4f + 0.6f * frozen);
          glVertex2f(x, y + 2);
        }
      } glEnd();
      // Body:
      glPushMatrix(); {
        glTranslatef((baddie->state == 3 ? -2.5f :
                      -0.5f * az_clock_zigzag(5, 5, clock)), 0, 0);
        for (int i = -82; i <= 82; i += 41) {
          glPushMatrix(); {
            glTranslatef(-12, 0, 0);
            glScalef(1, 0.85, 1);
            glRotatef(i, 0, 0, 1);
            glTranslatef((baddie->state == 3 ? 21 : 18), 0, 0);
            glScalef(0.7, 1, 1);
            draw_spiner_spine(flare, frozen);
          } glPopMatrix();
        }
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.2f + 0.8f * flare, 0.6f - 0.3f * flare,
                    0.4f + 0.6f * frozen);
          glVertex2f(-13, 0);
          glColor3f(0.06 + 0.5f * flare, 0.24f - 0.1f * flare,
                    0.12f + 0.5f * frozen);
          glVertex2f(-15, 0);
          for (int i = -120; i <= 120; i += 30) {
            glVertex2d(13 * cos(AZ_DEG2RAD(i)) - 7, 16 * sin(AZ_DEG2RAD(i)));
          }
          glVertex2f(-15, 0);
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_BAD_DEATH_RAY:
      assert(frozen == 0.0f);
      draw_heat_ray(baddie, clock, baddie->state == 1 ||
                    (baddie->state == 0 && az_clock_mod(2, 10, clock)),
                    color3(0.2 + 0.25 * flare, 0.2, 0.1),
                    color3(0.3 + 0.25 * flare, 0.3, 0.2),
                    color3(0.4 + 0.15 * flare, 0.4, 0.3));
      break;
    case AZ_BAD_OTH_GUNSHIP:
      draw_oth(baddie, flare, frozen, clock, oth_gunship_triangles,
               AZ_ARRAY_SIZE(oth_gunship_triangles));
      break;
    case AZ_BAD_FIREBALL_MINE:
      glPushMatrix(); {
        glScalef(1, 1.07, 1);
        const GLfloat blink =
          fmax(flare, (baddie->state == 1 &&
                       az_clock_mod(2, 4, clock) == 0 ? 0.5 : 0.0));
        const double radius = baddie->data->main_body.bounding_radius;
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(0.6f + 0.4f * blink, 0.6f, 0.6f);
          glVertex2d(-0.15 * radius, 0.2 * radius);
          glColor3f(0.2f + 0.3f * blink, 0.2f, 0.2f);
          for (int i = 0; i <= 360; i += 15) {
            glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                       radius * sin(AZ_DEG2RAD(i)));
          }
        } glEnd();
        for (int i = 0; i < 10; ++i) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.4f + 0.3f * blink, 0.4f, 0.4f);
            glVertex2d(radius - 4, 0);
            glColor3f(0.2f + 0.3f * blink, 0.2f, 0.2f);
            glVertex2d(radius - 1, 2); glVertex2d(radius + 6, 0);
            glVertex2d(radius - 1, -2);
          } glEnd();
          glRotatef(36, 0, 0, 1);
        }
        glRotatef(18, 0, 0, 1);
        for (int i = 0; i < 5; ++i) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.6f + 0.4f * blink, 0.6f, 0.6f);
            glVertex2d(radius - 9, 0);
            glColor3f(0.4f + 0.3f * blink, 0.4f, 0.4f);
            glVertex2d(radius - 8, 2); glVertex2d(radius - 3, 0);
            glVertex2d(radius - 8, -2);
          } glEnd();
          glRotatef(72, 0, 0, 1);
        }
      } glPopMatrix();
      break;
    case AZ_BAD_LEAPER: {
      const double tilt_degrees =
        (baddie->state != 0 ? 0.0 :
         (1.0 - fmin(baddie->cooldown / 0.5, 1.0)) * 10.0 +
         az_clock_zigzag(3, 8, clock));
      // Legs:
      for (int flip = 0; flip < 2; ++flip) {
        glPushMatrix(); {
          if (flip) glScalef(1, -1, 1);
          // Upper leg:
          glPushMatrix(); {
            glTranslated(-0.5 * tilt_degrees, 0, 0);
            if (baddie->state == 0) glRotatef(48 - tilt_degrees, 0, 0, 1);
            else glRotatef(70, 0, 0, 1);
            glBegin(GL_QUAD_STRIP); {
              glColor3f(0, 0.2, 0.1); glVertex2d(0, 5); glVertex2d(21, 2);
              glColor3f(0, 0.3, 0.2); glVertex2d(0, 0); glVertex2d(23, 0);
              glColor3f(0, 0.2, 0.1); glVertex2d(0, -4); glVertex2d(21, -3);
            } glEnd();
          } glPopMatrix();
          // Lower leg:
          if (baddie->state == 0) {
            glTranslated(-20, 20 + az_clock_zigzag(3, 8, clock), 0);
            glRotated(-tilt_degrees, 0, 0, 1);
          } else {
            glTranslated(-30, 18, 0);
            glRotated(5, 0, 0, 1);
          }
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0, 0.25, 0.1); glVertex2d(2, 5);
            glColor3f(0.25, 0.15, 0); glVertex2d(35, 4);
            glColor3f(0.5f * flare, 0.6, 0.4f + 0.6f * frozen);
            glVertex2d(0, 0);
            glColor3f(0.3f + 0.3f * flare, 0.2, frozen);
            glVertex2d(35, 0);
            glColor3f(0, 0.25, 0.1); glVertex2d(2, -6);
            glColor3f(0.25, 0.15, 0); glVertex2d(35, -4);
          } glEnd();
          glBegin(GL_QUAD_STRIP); {
            glColor3f(0, 0.25, 0.1); glVertex2d(18, 6); glVertex2d(35, 4);
            glColor3f(0.5f * flare, 0.6f + 0.4f * flare, 0.4f + 0.6f * frozen);
            glVertex2d(16, 0); glVertex2d(35, 0);
            glColor3f(0, 0.25, 0.1); glVertex2d(18, -6); glVertex2d(35, -4);
          } glEnd();
          // Foot:
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5, 0.5, 0.5); glVertex2d(0, -1);
            glColor3f(0.2, 0.3, 0.3);
            for (int i = -105; i <= 105; i += 30) {
              glVertex2d(5 * cos(AZ_DEG2RAD(i)), 7 * sin(AZ_DEG2RAD(i)) - 1);
            }
          } glEnd();
          // Knee knob:
          glTranslatef(35, 0, 0);
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5f * flare, 0.6, 0.4 + 0.6f * frozen);
            glVertex2d(0, 0);
            glColor3f(0, 0.25, 0.1);
            for (int i = -135; i <= 135; i += 30) {
              glVertex2d(6 * cos(AZ_DEG2RAD(i)), 5 * sin(AZ_DEG2RAD(i)));
            }
          } glEnd();
          // Knee spike:
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5, 0.5, 0.5); glVertex2f(4, 0);
            glColor3f(0.25, 0.25, 0.25);
            glVertex2f(5, 2); glVertex2f(10, 0); glVertex2f(5, -2);
          } glEnd();
        } glPopMatrix();
      }
      glPushMatrix(); {
        glTranslated(-0.5 * tilt_degrees, 0, 0);
        // Teeth:
        const int x = (baddie->state == 0 ? 0 : 3);
        for (int y = -2; y <= 2; y += 4) {
          glBegin(GL_TRIANGLE_FAN); {
            glColor3f(0.5, 0.5, 0.5); glVertex2i(8 + x, y);
            glColor3f(0.25, 0.25, 0.25);
              glVertex2i(9 + x, 2 + y); glVertex2i(15 + x, y);
              glVertex2i(9 + x, -2 + y);
          } glEnd();
        }
        // Body:
        glBegin(GL_TRIANGLE_FAN); {
          glColor3f(flare, 0.9, 0.5f + 0.5f * frozen); glVertex2d(-3, 0);
          glColor3f(0.5f * flare, 0.25, 0.1f + 0.9f * frozen);
          glVertex2d(-10, 0);
          for (int i = -135; i <= 135; i += 30) {
            glVertex2d(12 * cos(AZ_DEG2RAD(i)), 9 * sin(AZ_DEG2RAD(i)));
          }
          glVertex2d(-10, 0);
        } glEnd();
        // Eye:
        glBegin(GL_POLYGON); {
          glColor4f(1, 0, 0, 0.4);
          glVertex2f(10, 1); glVertex2f(9, 2); glVertex2f(7, 0);
          glVertex2f(9, -2), glVertex2f(10, -1);
        } glEnd();
      } glPopMatrix();
    } break;
    case AZ_BAD_BOUNCER_90:
      glBegin(GL_TRIANGLE_FAN); {
        const unsigned int zig = az_clock_zigzag(30, 1, clock);
        glColor3f(0.25f + 0.75f * flare, 1 - frozen, 1 - flare);
        glVertex2f(0, 0);
        glColor3f(0.5f * flare + 0.01f * zig, 0.25f + 0.25f * flare,
                  0.25f + 0.25f * frozen);
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.5, 0.25, 1);
        glVertex2f(0, 7);
        glColor4f(0.5, 0.25, 1, 0);
        glVertex2f(3, 2); glVertex2f(3, 12); glVertex2f(-3, 12);
        glVertex2f(-3, 2); glVertex2f(3, 2);
      } glEnd();
      break;
  }
}

void az_draw_baddie(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  glPushMatrix(); {
    glTranslated(baddie->position.x, baddie->position.y, 0);
    glRotated(AZ_RAD2DEG(baddie->angle), 0, 0, 1);
    draw_baddie_internal(baddie, clock);
  } glPopMatrix();
}

void az_draw_baddies(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(baddie, state->baddies) {
    if (baddie->kind == AZ_BAD_NOTHING ||
        baddie->kind == AZ_BAD_MARKER) continue;
    az_draw_baddie(baddie, state->clock);
  }
}

/*===========================================================================*/
