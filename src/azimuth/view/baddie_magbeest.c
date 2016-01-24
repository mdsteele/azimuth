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

#include "azimuth/view/baddie_magbeest.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/util/misc.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

#define MAGNET_FUSION_BEAM_CHARGE_STATE 5

// Comparison function for use with qsort.  Sorts doubles by their cosines.
static int compare_cosines(const void *v1, const void *v2) {
  const double c1 = cos(*(double*)v1);
  const double c2 = cos(*(double*)v2);
  return (c1 < c2 ? -1 : c1 > c2 ? 1 : 0);
}

static void draw_magbeest_leg(const az_baddie_t *baddie, int leg_index) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  assert(0 <= leg_index && leg_index < baddie->data->num_components);
  const az_component_t *leg = &baddie->components[leg_index];
  const double length =
    baddie->data->components[leg_index].polygon.vertices[1].x;
  glPushMatrix(); {
    az_gl_translated(leg->position);
    az_gl_rotated(leg->angle);
    // Leg:
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.2, 0.2, 0.2); glVertex2f(0,  10); glVertex2f(length,  10);
      glColor3f(0.4, 0.5, 0.5); glVertex2f(0,   0); glVertex2f(length,   0);
      glColor3f(0.2, 0.2, 0.2); glVertex2f(0, -10); glVertex2f(length, -10);
    } glEnd();
    if (leg_index % 2 == 0) {
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.2, 0.2, 0.2);
        glVertex2f(length - 10, 10); glVertex2f(length - 10, -10);
        glVertex2f(length, -10); glVertex2f(length, 10);
      } glEnd();
      // Foot:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.3, 0.35, 0.35); glVertex2f(0, 0);
        glColor3f(0.2, 0.2, 0.2);
        for (int i = -90; i <= 90; i += 15) {
          glVertex2d(-5 * cos(AZ_DEG2RAD(i)), 10 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
    } else {
      // Knee:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.3, 0.3, 0.3);
        glVertex2f(length - 10, 10); glVertex2f(length - 10, -10);
        for (int i = -90; i <= 90; i += 15) {
          glVertex2d(10 * cos(AZ_DEG2RAD(i)) + length,
                     10 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      // Screw:
      // TODO: Make the screw turn with the other leg segment
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.35, 0.4, 0.4);
        for (int i = 0; i < 360; i += 30) {
          glVertex2d(5 * cos(AZ_DEG2RAD(i)) + length,
                     5 * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      glBegin(GL_LINES); {
        glColor3f(0.2, 0.2, 0.2);
        glVertex2f(length, -5); glVertex2f(length, 5);
      } glEnd();
    }
  } glPopMatrix();
}

static void draw_magbeest_legs_base(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L ||
         baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  // Casing:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.4, 0.5, 0.5);
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int i = 0; i < polygon.num_vertices; ++i) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
  // Rim:
  glBegin(GL_TRIANGLE_STRIP); {
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      glColor3f(0.15, 0.2, 0.2);
      az_gl_vertex(polygon.vertices[i]);
      glColor3f(0.4, 0.5, 0.5);
      az_gl_vertex(az_vmul(polygon.vertices[i], 0.8));
    }
  } glEnd();
  // Leg screws:
  // TODO: Make the screw turn with the leg segment
  for (int j = 0; j < 2; ++j) {
    const double y = -25 + 50 * j;
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.35, 0.4, 0.4);
      for (int i = 0; i < 360; i += 30) {
        glVertex2d(5 * cos(AZ_DEG2RAD(i)) - 20,
                   5 * sin(AZ_DEG2RAD(i)) + y);
      }
    } glEnd();
    glBegin(GL_LINES); {
      glColor3f(0.2, 0.2, 0.2);
      glVertex2f(-20, y - 5); glVertex2f(-20, y + 5);
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_bad_magbeest_head(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_HEAD);
  const float flare = baddie->armor_flare;
  const double hurt =
    (baddie->data->max_health - baddie->health) / baddie->data->max_health;
  // Pistons:
  for (int i = 0; i < 10; ++i) {
    const az_component_t *component = &baddie->components[i];
    const az_vector_t vertex = baddie->data->components[i].polygon.vertices[0];
    const float x_max = vertex.x;
    const float thick = vertex.y;
    glPushMatrix(); {
      az_gl_translated(component->position);
      az_gl_rotated(component->angle);
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.2, 0.2, 0.2);
        glVertex2f(-80,  thick); glVertex2f(x_max,  thick);
        glColor3f(0.4, 0.5, 0.5);
        glVertex2f(-80,      0); glVertex2f(x_max,      0);
        glColor3f(0.2, 0.2, 0.2);
        glVertex2f(-80, -thick); glVertex2f(x_max, -thick);
      } glEnd();
    } glPopMatrix();
  }
  // Casing:
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.4, 0.5, 0.5);
    const az_polygon_t polygon = baddie->data->main_body.polygon;
    for (int i = 0; i < polygon.num_vertices; ++i) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
  // Eye:
  glPushMatrix(); {
    const az_component_t *eye = &baddie->components[10];
    assert(!az_vnonzero(eye->position));
    az_gl_rotated(eye->angle);
    const double radius = baddie->data->components[10].bounding_radius;
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.25f + 0.75f * flare, 0.25f, 0.25f); glVertex2f(0, 0);
      glColor3f(0.07f + 0.3f * flare, 0.07f, 0.07f);
      for (int i = 0; i <= 360; i += 20) {
        glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(1, 0.3, 0); glVertex2f(15, 0); glColor4f(1, 0.3, 0, 0);
      for (int i = 0; i <= 360; i += 30) {
        glVertex2d(15 + 4 * cos(AZ_DEG2RAD(i)), 6 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    const az_color_t cracks_color = {128, 64, 0, 64};
    for (int i = 0; i < 4; ++i) {
      const double angle = -AZ_DEG2RAD(50) + i * AZ_DEG2RAD(80) +
        i * i * AZ_DEG2RAD(2);
      az_draw_cracks_with_color(az_vpolar(-radius, angle), angle,
                                6 * hurt * (1 - 0.25 * i), cracks_color);
    }
    for (int i = 0; i < 3; ++i) {
      const double angle = AZ_DEG2RAD(85) - i * AZ_DEG2RAD(92);
      az_draw_cracks_with_color(az_vpolar(-radius, angle), angle,
                                fmax(0, 3 * (hurt - 0.25)), cracks_color);
    }
  } glPopMatrix();
}

void az_draw_bad_magbeest_legs_l(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_L);
  // Legs:
  draw_magbeest_leg(baddie, 2);
  draw_magbeest_leg(baddie, 1);
  draw_magbeest_leg(baddie, 4);
  draw_magbeest_leg(baddie, 3);
  // Base:
  draw_magbeest_legs_base(baddie);
  // Magnet:
  glPushMatrix(); {
    const az_component_t *magnet = &baddie->components[0];
    az_gl_translated(magnet->position);
    az_gl_rotated(magnet->angle);
    // Magnetic field:
    if (baddie->state >= MAGNET_FUSION_BEAM_CHARGE_STATE &&
        az_clock_mod(2, 2, clock)) {
      const float scale = (baddie->state == MAGNET_FUSION_BEAM_CHARGE_STATE ?
                           baddie->cooldown / 3.0 : 1.0);
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1.0f - scale, 0.5f * scale, scale, 0.15);
        glVertex2f(25, 0); glVertex2f(25, 15 * scale);
        glVertex2f(1500, 264 * scale); glVertex2f(1500, -264 * scale);
        glVertex2f(25, -15 * scale);
      } glEnd();
    }
    // Casing:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6, 0.6, 0.6);
      glVertex2f(0, 0); glVertex2f(25, -15); glVertex2f(25, 15);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25); glVertex2f(25, -20);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(25, -15);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-5, 0);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(0, 0);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(25, 20);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(25, 15);
    } glEnd();
    // Hinge:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(1, 1, 1); glVertex2f(0, 0);
      glColor3f(0.25, 0.25, 0.25);
      for (int i = 30; i <= 330; i += 30) {
        glVertex2d(7 * cos(AZ_DEG2RAD(i)), 7 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    // Screw:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.35, 0.4, 0.4);
      for (int i = 0; i < 360; i += 30) {
        glVertex2d(4 * cos(AZ_DEG2RAD(i)), 4 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
    // TODO: Make the screw stay fixed to the casing
    glBegin(GL_LINES); {
      glColor3f(0.2, 0.2, 0.2);
      glVertex2f(0, -4); glVertex2f(0, 4);
    } glEnd();
    // Dish:
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = -35; i <= 35; i += 5) {
        glColor3f(0.25, 0.25, 0.25);
        glVertex2d(55 - 30 * cos(AZ_DEG2RAD(i)), 30 * sin(AZ_DEG2RAD(i)));
        glColor3f(0.7, 0.7, 0.7);
        glVertex2d(55 - 38 * cos(AZ_DEG2RAD(i)), 38 * sin(AZ_DEG2RAD(i)));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_magbeest_legs_r(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGBEEST_LEGS_R);
  // Legs:
  draw_magbeest_leg(baddie, 2);
  draw_magbeest_leg(baddie, 1);
  draw_magbeest_leg(baddie, 4);
  draw_magbeest_leg(baddie, 3);
  // Base:
  draw_magbeest_legs_base(baddie);
  // Gatling gun:
  glPushMatrix(); {
    const az_component_t *gun = &baddie->components[0];
    az_gl_translated(gun->position);
    az_gl_rotated(gun->angle);
    // Barrels:
    double thetas[5];
    const int num_thetas = AZ_ARRAY_SIZE(thetas);
    for (int i = 0; i < num_thetas; ++i) {
      thetas[i] = baddie->param + i * AZ_TWO_PI / num_thetas;
    }
    qsort(thetas, num_thetas, sizeof(double), compare_cosines);
    for (int i = 0; i < num_thetas; ++i) {
      const double theta = thetas[i];
      const GLfloat y_offset = 7 * sin(theta);
      const GLfloat x_ext = 0.5 * cos(theta);
      const GLfloat z_fade = 0.6 + 0.4 * cos(theta);
      glBegin(GL_TRIANGLE_STRIP); {
        const GLfloat outer = 0.25f * z_fade;
        const GLfloat inner = 0.75f * z_fade;
        glColor3f(outer, outer, outer);
        glVertex2f(12, y_offset + 3); glVertex2f(50 + x_ext, y_offset + 3);
        glColor3f(inner, inner, inner);
        glVertex2f(12, y_offset);     glVertex2f(50 + x_ext, y_offset);
        glColor3f(outer, outer, outer);
        glVertex2f(12, y_offset - 3); glVertex2f(50 + x_ext, y_offset - 3);
      } glEnd();
    }
    // Case:
    glBegin(GL_TRIANGLE_FAN); {
      glColor3f(0.6, 0.6, 0.6);
      glVertex2f(12, 15); glVertex2f(-20, 15);
      glVertex2f(-20, -15); glVertex2f(12, -15);
    } glEnd();
    glBegin(GL_TRIANGLE_STRIP); {
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, 10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-17, 10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(-20, -15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(-17, -10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, -15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, -10);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(13, 0);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(11, 0);
      glColor3f(0.25, 0.25, 0.25); glVertex2f(15, 15);
      glColor3f(0.50, 0.50, 0.50); glVertex2f(12, 10);
    } glEnd();
  } glPopMatrix();
}

void az_draw_bad_magma_bomb(const az_baddie_t *baddie, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_MAGMA_BOMB);
  const float flare = baddie->armor_flare;
  const bool blink = (baddie->cooldown < 1.5 &&
                      0 != (int)(4 * baddie->cooldown) % 2);
  const double radius = baddie->data->main_body.bounding_radius;
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.35f + 0.6f * flare, 0.35f, 0.35f); glVertex2f(0, 0);
    glColor3f(0.1f + 0.4f * flare, 0.1f, 0.1f);
    for (int i = 0; i <= 360; i += 20) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_LINE_STRIP); {
    if (blink) glColor3f(0.8, 0.6, 0.6);
    else glColor3f(0.3, 0.1, 0.1);
    glVertex2f(0, radius);
    if (blink) glColor3f(1, 0.7, 0.7);
    else glColor3f(0.6, 0.4, 0.4);
    glVertex2f(0, 0);
    if (blink) glColor3f(0.8, 0.6, 0.6);
    else glColor3f(0.3, 0.1, 0.1);
    glVertex2f(0, -radius);
  } glEnd();
  const double hurt =
    (baddie->data->max_health - baddie->health) / baddie->data->max_health;
  for (int i = 0; i < 2; ++i) {
    const double angle = i * AZ_DEG2RAD(180);
    az_draw_cracks(az_vpolar(-radius, angle), angle, 4 * hurt);
  }
}

void az_draw_bad_scrap_metal(const az_baddie_t *baddie) {
  assert(baddie->kind == AZ_BAD_SCRAP_METAL);
  const az_polygon_t polygon = baddie->data->main_body.polygon;
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.75, 0.5, 1);
    glVertex2f(0, 0);
    glColor3f(0.2, 0.1, 0.3);
    for (int i = polygon.num_vertices - 1, j = 0;
         i < polygon.num_vertices; i = j++) {
      az_gl_vertex(polygon.vertices[i]);
    }
  } glEnd();
}

/*===========================================================================*/
