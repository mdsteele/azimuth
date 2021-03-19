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

#include "azimuth/view/baddie_bouncer.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <SDL_opengl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

void az_draw_bad_bouncer(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_BOUNCER);
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    const int zig = az_clock_zigzag(15, 1, clock);
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
}

void az_draw_bad_bouncer_90(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_BOUNCER_90);
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    const int zig = az_clock_zigzag(30, 1, clock);
    glColor3f(0.25f + 0.75f * flare, 1 - frozen, 1 - flare);
    glVertex2f(0, 0);
    glColor3f(0.5f * flare + 0.01f * zig, 0.25f + 0.25f * flare,
              0.25f + 0.25f * frozen);
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.5, 0.25, 1); glVertex2f(0, 7);
    glColor4f(0.5, 0.25, 1, 0);
    glVertex2f(3, 2); glVertex2f(3, 12); glVertex2f(-3, 12);
    glVertex2f(-3, 2); glVertex2f(3, 2);
  } glEnd();
}

void az_draw_bad_fast_bouncer(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_FAST_BOUNCER);
  const float flare = baddie->armor_flare;
  glBegin(GL_TRIANGLE_FAN); {
    const int zig = az_clock_zigzag(15, 1, clock);
    glColor3f(1.0f - 0.75f * frozen, 0.5f + 0.01f * zig + 0.3f * flare,
              0.25f + 0.75f * frozen);
    glVertex2f(0, 0);
    glColor3f(0.25f + 0.02f * zig - 0.25f * frozen, 0.1f + 0.5f * flare,
              0.25f * frozen);
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(15 * cos(AZ_DEG2RAD(i)), 15 * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  glBegin(GL_TRIANGLE_FAN); {
    glColor3f(0.25, 0.5, 1);
    glVertex2f(0, 7);
    glColor4f(0.25, 0.5, 1, 0);
    glVertex2f(3, 2); glVertex2f(3, 12); glVertex2f(-3, 12);
    glVertex2f(-3, 2); glVertex2f(3, 2);
  } glEnd();
}

/*===========================================================================*/

static void draw_electron(double radius, az_vector_t position, double angle) {
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

static void draw_atom(const az_baddie_t *baddie, az_color_t inner,
                      az_color_t outer) {
  for (int i = 0; i < baddie->data->num_components; ++i) {
    const az_component_t *component = &baddie->components[i];
    if (component->angle >= 0.0) continue;
    draw_electron(baddie->data->components[i].bounding_radius,
                  component->position, component->angle);
  }
  glBegin(GL_TRIANGLE_FAN); {
    az_gl_color(inner);
    glVertex2f(-2, 2);
    az_gl_color(outer);
    const double radius = baddie->data->main_body.bounding_radius;
    for (int i = 0; i <= 360; i += 15) {
      glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
  for (int i = 0; i < baddie->data->num_components; ++i) {
    const az_component_t *component = &baddie->components[i];
    if (component->angle < 0.0) continue;
    draw_electron(baddie->data->components[i].bounding_radius,
                  component->position, component->angle);
  }
}

void az_draw_bad_atom(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_ATOM);
  const float flare = baddie->armor_flare;
  draw_atom(baddie, az_color3f(flare, 1.0f - 0.5f * flare, frozen),
            az_color3f(0.4f * flare, 0.3f - 0.3f * flare,
                       0.1f + 0.4f * frozen));
}

void az_draw_bad_red_atom(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_RED_ATOM);
  const float flare = baddie->armor_flare;
  draw_atom(baddie,
            az_color3f(1.0f - 0.75f * frozen, 0.75f * flare + 0.25f * frozen,
                       frozen),
            az_color3f(0.3f - 0.3f * frozen, 0.3f * flare,
                       0.1f + 0.4f * frozen));
}

/*===========================================================================*/
