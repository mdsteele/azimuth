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

#include "azimuth/view/baddie_oth.h"

#include <assert.h>
#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/state/baddie_oth.h"
#include "azimuth/util/bezier.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/particle.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static const az_vector_t oth_brawler_triangles[] = {
  // Body:
  {0, 0}, {15, 10}, {0, 20},
  {15, -10}, {15, 10}, {0, 0},
  {0, -20}, {15, -10}, {0, 0},
  {-15, 7}, {0, 0}, {0, 20},
  {0, 0}, {-15, 7}, {-15, -7},
  {0, -20}, {0, 0}, {-15, -7},
  // Left arm:
  {10, 13.33}, {0, 20}, {19, 22},
  {19, 22}, {0, 20}, {17, 30},
  {17, 30}, {19, 22}, {32, 26},
  // Right arm:
  {0, -20}, {10, -13.33}, {19, -22},
  {0, -20}, {19, -22}, {17, -30},
  {19, -22}, {17, -30}, {32, -26},
  // Center arm:
  {15, 8}, {15, 0}, {25, 5},
  {15, 0}, {15, -8}, {25, -5},
  {25, 5}, {25, -5}, {15, 0},
  {25, -5}, {25, 5}, {36, 0},
  // Left leg:
  {0, 20}, {-10, 11.33}, {-29, 16},
  // Right leg:
  {-10, -11.33}, {0, -20}, {-29, -16}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_brawler_triangles) % 3 == 0);

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

static const az_vector_t oth_crawler_triangles[] = {
  // Body:
  {0, 0}, {0, 20}, {12, 10},
  {12, 10}, {12, -10}, {0, 0},
  {0, -20}, {0, 0}, {12, -10},
  {0, 0}, {0, -20}, {-12, -10},
  {-12, -10}, {-12, 10}, {0, 0},
  {0, 20}, {0, 0}, {-12, 10},
  // Spines:
  {0, 20}, {6, 15}, {9, 23},
  {6, 15}, {12, 10}, {15, 18},
  {12, 10}, {12, 0}, {20, 5},
  {12, -10}, {12, 0}, {20, -5},
  {6, -15}, {12, -10}, {15, -18},
  {0, -20}, {6, -15}, {9, -23},
  // Feet:
  {-12, 10}, {-12, 5}, {-20, 12},
  {-12, 5}, {-12, 0}, {-22, 3},
  {-12, -5}, {-12, 0}, {-22, -3},
  {-12, -10}, {-12, -5}, {-20, -12}
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_crawler_triangles) % 3 == 0);

static const az_vector_t oth_minicrab_triangles[] = {
  // Body:
  {0, 0}, {15, 9}, {0, 14},
  {15, -9}, {15, 9}, {0, 0},
  {0, -14}, {15, -9}, {0, 0},
  {-15, 7}, {0, 0}, {0, 14},
  {0, 0}, {-15, 7}, {-15, -7},
  {0, -14}, {0, 0}, {-15, -7},
  // Fangs:
  {25, 3}, {15, 9}, {15, 4},
  {15, -9}, {25, -3}, {15, -4},
  // Legs:
  {-15, 7}, {-15, 0}, {-29, 9},
  {-15, 0}, {-29, -9}, {-15, -7},
  // Arms:
  {15, 9}, {0, 14}, {30, 17},
  {15, -9}, {0, -14}, {30, -17},
};
AZ_STATIC_ASSERT(AZ_ARRAY_SIZE(oth_minicrab_triangles) % 3 == 0);

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
  {18, 0}, {2.25, 3.89712}, {2.25, -3.89712},
  {-9, 15.58845}, {-4.5, 0}, {2.25, 3.89712},
  {-9, -15.58845}, {2.25, -3.89712}, {-4.5, 0}
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
    const az_baddie_t *baddie, GLfloat frozen, az_clock_t clock,
    const az_vector_t *vertices, int num_vertices) {
  assert(baddie->kind != AZ_BAD_NOTHING);
  assert(num_vertices % 3 == 0);
  const int num_triangles = num_vertices / 3;
  const bool spin = (baddie->kind != AZ_BAD_OTH_GUNSHIP);

  const double hurt_ratio = 1.0 - baddie->health / baddie->data->max_health;
  const GLfloat flare = fmax(baddie->armor_flare, 0.5 * hurt_ratio);

  for (int i = 0; i < num_triangles; ++i) {
    const az_vector_t *vs = vertices + i * 3;
    const az_vector_t center =
      az_vdiv(az_vadd(az_vadd(vs[0], vs[1]), vs[2]), 3);
    glPushMatrix(); {
      az_gl_translated(center);
      if (spin) {
        glRotated((baddie->kind == AZ_BAD_OTH_RAZOR ?
                   (baddie->state % 2 ? 6 : -6) : 1) *
                  az_clock_mod(360, 1, clock) -
                  AZ_RAD2DEG(baddie->angle * 8), 0, 0, 1);
      }
      glBegin(GL_TRIANGLES); {
        for (int j = 0; j < 3; ++j) {
          const az_clock_t clk = clock + 2 * j;
          const GLfloat r = (az_clock_mod(6, 2, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 2, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 2, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor3f(r + flare * (1.0f - r), (1.0f - 0.5f * flare) * g,
                    (1.0f - flare) * b + frozen * (1.0f - b));
          az_gl_vertex(az_vsub(vs[j], center));
        }
      } glEnd();
    } glPopMatrix();
  }
}

static void next_tendril_color(int *color_index, az_clock_t clock) {
  const az_clock_t clk = clock + 2 * (*color_index++ % 3);
  const GLfloat r = (az_clock_mod(6, 2, clk)     < 3 ? 0.75f : 0.25f);
  const GLfloat g = (az_clock_mod(6, 2, clk + 2) < 3 ? 0.75f : 0.25f);
  const GLfloat b = (az_clock_mod(6, 2, clk + 4) < 3 ? 0.75f : 0.25f);
  glColor4f(r, g, b, 0.5f);
}

static void draw_tendrils(const az_baddie_t *baddie,
                          const az_oth_tendrils_data_t *tendrils,
                          az_clock_t clock) {
  assert(tendrils->num_tendrils <= AZ_MAX_BADDIE_COMPONENTS);
  for (int i = 0; i < tendrils->num_tendrils; ++i) {
    const az_component_t *tip =
      &baddie->components[AZ_MAX_BADDIE_COMPONENTS - 1 - i];
    const az_vector_t base = tendrils->tendril_bases[i];
    const double dist = az_vdist(base, tip->position);
    const az_vector_t ctrl1 = az_vadd(az_vwithlen(base, 0.4 * dist), base);
    const az_vector_t ctrl2 = az_vadd(az_vpolar(-0.4 * dist, tip->angle),
                                      tip->position);
    glBegin(GL_TRIANGLE_STRIP); {
      int color_index = 0;
      for (double t = 0.0; t <= 1.0; t += 0.0625) {
        const az_vector_t point = az_cubic_bezier_point(
            base, ctrl1, ctrl2, tip->position, t);
        const double angle = az_cubic_bezier_angle(
            base, ctrl1, ctrl2, tip->position, t);
        const az_vector_t perp =
          az_vpolar(tendrils->semithick * (1.0 - t), angle + AZ_HALF_PI);
        next_tendril_color(&color_index, clock);
        az_gl_vertex(az_vadd(point, perp));
        next_tendril_color(&color_index, clock);
        az_gl_vertex(az_vsub(point, perp));
      }
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_bad_oth_brawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_BRAWLER);
  draw_tendrils(baddie, &AZ_OTH_BRAWLER_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_brawler_triangles,
           AZ_ARRAY_SIZE(oth_brawler_triangles));
}

void az_draw_bad_oth_crab(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_CRAB_1 ||
         baddie->kind == AZ_BAD_OTH_CRAB_2);
  draw_tendrils(baddie, &AZ_OTH_CRAB_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_crab_triangles,
           AZ_ARRAY_SIZE(oth_crab_triangles));
}

void az_draw_bad_oth_crawler(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_CRAWLER);
  draw_tendrils(baddie, &AZ_OTH_CRAWLER_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_crawler_triangles,
           AZ_ARRAY_SIZE(oth_crawler_triangles));
}

void az_draw_bad_oth_gunship(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_GUNSHIP);
  const az_component_t *tractor_component = &baddie->components[6];
  if (tractor_component->angle != 0) {
    const az_vector_t start = tractor_component->position;
    const az_vector_t delta = az_vsub(baddie->position, start);
    const double dist = az_vnorm(delta);
    const double thick = 4;
    const float redblue = az_clock_mod(2, 2, clock) == 0 ? 1.0 : 0.0;
    glPushMatrix(); {
      az_gl_rotated(-baddie->angle);
      az_gl_translated(az_vneg(delta));
      az_gl_rotated(az_vtheta(delta));
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(redblue, 1, redblue, 0.5); glVertex2d(0, 0);
        glColor4f(redblue, 1, redblue, 0);
        for (int i = 90; i <= 270; i += 20) {
          glVertex2d(thick * cos(AZ_DEG2RAD(i)), thick * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_TRIANGLE_STRIP); {
        glColor4f(redblue, 1, redblue, 0);
        glVertex2d(0,  thick); glVertex2d(dist,  thick);
        glColor4f(redblue, 1, redblue, 0.5);
        glVertex2d(0,      0); glVertex2d(dist,      0);
        glColor4f(redblue, 1, redblue, 0);
        glVertex2d(0, -thick); glVertex2d(dist, -thick);
      } glEnd();
    } glPopMatrix();
  }
  draw_tendrils(baddie, &AZ_OTH_GUNSHIP_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_gunship_triangles,
           AZ_ARRAY_SIZE(oth_gunship_triangles));
}

void az_draw_bad_oth_minicrab(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_MINICRAB);
  draw_tendrils(baddie, &AZ_OTH_MINICRAB_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_minicrab_triangles,
           AZ_ARRAY_SIZE(oth_minicrab_triangles));
}

void az_draw_bad_oth_orb(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_ORB_1 ||
         baddie->kind == AZ_BAD_OTH_ORB_2);
  draw_tendrils(baddie, &AZ_OTH_ORB_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_orb_triangles,
           AZ_ARRAY_SIZE(oth_orb_triangles));
}

void az_draw_bad_oth_razor(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_RAZOR);
  draw_tendrils(baddie, &AZ_OTH_RAZOR_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_razor_triangles,
           AZ_ARRAY_SIZE(oth_razor_triangles));
}

void az_draw_bad_oth_snapdragon(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_OTH_SNAPDRAGON);
  draw_tendrils(baddie, &AZ_OTH_SNAPDRAGON_TENDRILS, clock);
  draw_oth(baddie, frozen, clock, oth_snapdragon_triangles,
           AZ_ARRAY_SIZE(oth_snapdragon_triangles));
}

void az_draw_bad_reflection(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_REFLECTION);
  if (baddie->param < 0.0) return;
  glPushMatrix(); {
    az_gl_translated(baddie->components[0].position);
    az_gl_rotated(baddie->components[0].angle);
    const GLfloat alpha = 0.2;
    // Struts:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f(-7,  7); glVertex2f( 1,  7);
      glVertex2f(-7,  4); glVertex2f( 1,  4);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f(-7, -4); glVertex2f( 1, -4);
      glVertex2f(-7, -7); glVertex2f( 1, -7);
    } glEnd();
    // Port engine:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f(-10, 12); glVertex2f(6, 12);
      glColor4f(0.75, 0.75, 0.75, alpha);
      glVertex2f(-11,  7); glVertex2f(8, 7);
    } glEnd();
    // Starboard engine:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0.75, 0.75, 0.75, alpha);
      glVertex2f(-11,  -7); glVertex2f(8,  -7);
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f(-10, -12); glVertex2f(6, -12);
    } glEnd();
    // Main body:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f( 15,  4); glVertex2f(-14,  4);
      glColor4f(0.75, 0.75, 0.75, alpha);
      glVertex2f( 20,  0); glVertex2f(-14,  0);
      glColor4f(0.25, 0.25, 0.25, alpha);
      glVertex2f( 15, -4); glVertex2f(-14, -4);
    } glEnd();
    // Windshield:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor4f(0, 0.5, 0.5, alpha); glVertex2f(15,  2);
      glColor4f(0, 1.0, 1.0, alpha); glVertex2f(18,  0); glVertex2f(12, 0);
      glColor4f(0, 0.5, 0.5, alpha); glVertex2f(15, -2);
    } glEnd();
    if (baddie->state != 0) {
      const double nps_lifetime = 3.0;
      az_particle_t particle = {
        .kind = AZ_PAR_NPS_PORTAL,
        .color = {128, 64, 255, 255},
        .age = 0.5 * nps_lifetime * baddie->param,
        .lifetime = nps_lifetime,
        .param1 = 50.0 * sqrt(nps_lifetime)
      };
      az_draw_particle(&particle, clock);
    }
  } glPopMatrix();
}

/*===========================================================================*/
