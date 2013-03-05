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

static void draw_turret(const az_baddie_t *baddie,
                        az_color_t far_edge, az_color_t mid_edge,
                        az_color_t near_edge, az_color_t center,
                        az_color_t gun_edge, az_color_t gun_middle) {
  glBegin(GL_QUAD_STRIP); {
    for (int i = 0; i <= 360; i += 60) {
      az_gl_color(mid_edge);
      glVertex2d(18 * cos(AZ_DEG2RAD(i)), 18 * sin(AZ_DEG2RAD(i)));
      az_gl_color(far_edge);
      glVertex2d(20 * cos(AZ_DEG2RAD(i)), 20 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
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

static void draw_spiner_spine(double flare, double frozen) {
  glBegin(GL_TRIANGLE_STRIP); {
    glColor4f(0.5 * flare, 0.3, 0, 0);
    glVertex2f(-3, 3);
    glColor3f(0.6 + 0.4 * flare, 0.7, 0.6);
    glVertex2f(5, 0);
    glColor3f(0.6 + 0.4 * flare, 0.7, frozen);
    glVertex2f(-5, 0);
    glColor4f(0 * flare, 0.3, 0, 0);
    glVertex2f(-3, -3);
  } glEnd();
}

static void draw_box(bool armored, double flare) {
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

/*===========================================================================*/

static void draw_baddie_internal(const az_baddie_t *baddie, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  const float frozen = (baddie->frozen <= 0.0 ? 0.0 :
                        baddie->frozen >= 0.2 ? 0.5 + 0.5 * baddie->frozen :
                        az_clock_mod(3, 2, clock) < 2 ? 0.6 : 0.0);
  if (baddie->frozen > 0.0) clock = 0;
  switch (baddie->kind) {
    case AZ_BAD_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_BAD_LUMP:
      glColor3f(1 - frozen, 0, 1 - 0.75 * flare); // magenta
      glBegin(GL_POLYGON); {
        az_polygon_t polygon = baddie->data->main_body.polygon;
        for (int i = 0; i < polygon.num_vertices; ++i) {
          glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
        }
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
      // Body:
      for (int i = -1; i <= 1; i += 2) {
        glBegin(GL_QUAD_STRIP); {
          for (int x = 20; x >= -15; x -= 5) {
            if (x % 2) glColor3f(0.5 + 0.5 * flare - 0.5 * frozen,
                                 1 - flare, frozen);
            else glColor3f(0.4 - 0.4 * frozen, 0.4, frozen);
            const double y = i * 6.0 * (1 - pow(0.05 * x, 4) + 0.025 * x);
            glVertex2d(x, 0);
            glColor3f(0.4 * flare, 0.5, frozen);
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
            for (int i = 0; i < polygon.num_vertices; ++i) {
              glVertex2d(polygon.vertices[i].x, polygon.vertices[i].y);
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
    case AZ_BAD_CRAWLER:
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
    case AZ_BAD_STINGER:
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
        glColor3f(0.75, 0.25 + 0.5 * flare, 0.5); // reddish
        glVertex2f(8, 0);
        glColor3f(0.25, 0.5 * flare, 0.5 * flare); // dark red
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
    if (baddie->kind == AZ_BAD_NOTHING) continue;
    az_draw_baddie(baddie, state->clock);
  }
}

/*===========================================================================*/
