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

#include "azimuth/view/baddie_vehicle.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/color.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

void az_draw_bad_copter(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_COPTER_HORZ ||
         baddie->kind == AZ_BAD_COPTER_VERT);
  const float flare = baddie->armor_flare;
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
  const az_color_t outer =
    az_color3f(0.15f + 0.85f * flare, 0.25f, 0.2f + 0.8f * frozen);
  const az_color_t inner =
    az_color3f(0.4f + 0.6f * flare, 0.45f, 0.45f + 0.55f * frozen);
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
  // Cracks:
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  az_draw_cracks((az_vector_t){4, 20}, AZ_DEG2RAD(-110), 4 * hurt);
  az_draw_cracks((az_vector_t){13, -11}, AZ_DEG2RAD(180), 3 * hurt);
  az_draw_cracks((az_vector_t){-6, -20}, AZ_DEG2RAD(80), 3 * hurt);
}

void az_draw_bad_small_truck(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SMALL_TRUCK);
  const float flare = baddie->armor_flare;
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
      az_color3f(0.15f + 0.85f * flare, 0.25f, 0.2f + 0.8f * frozen);
    const az_color_t inner =
      az_color3f(0.4f + 0.6f * flare, 0.45f, 0.45f + 0.55f * frozen);
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
  // Cracks:
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  az_draw_cracks((az_vector_t){32, -1}, AZ_DEG2RAD(180), 5 * hurt);
  az_draw_cracks((az_vector_t){-20, -14}, AZ_DEG2RAD(90), 3.5 * hurt);
  az_draw_cracks((az_vector_t){-30, 6}, AZ_DEG2RAD(0), 3.5 * hurt);
}

void az_draw_bad_small_auv(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  assert(baddie->kind == AZ_BAD_SMALL_AUV);
  const float flare = baddie->armor_flare;
  // Propeller blades:
  glBegin(GL_QUADS); {
    GLfloat y = 3 * az_clock_zigzag(5, 1, clock);
    glColor4f(0.5, 0.5, 0.5, 0.6);
    glVertex2f(-31, y); glVertex2f(-33, y);
    glVertex2f(-33, -y); glVertex2f(-31, -y);
    y = 3 * az_clock_zigzag(5, 1, clock + 2);
    glVertex2f(-34, y); glVertex2f(-36, y);
    glVertex2f(-36, -y); glVertex2f(-34, -y);
  } glEnd();
  // Propeller axle:
  const az_color_t outer =
    az_color3f(0.25f + 0.75f * flare, 0.35f, 0.2f + 0.8f * frozen);
  const az_color_t inner =
    az_color3f(0.5f + 0.5f * flare, 0.55f, 0.45f + 0.55f * frozen);
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(outer); glVertex2f(-36, 2); glVertex2f(-29, 2);
    az_gl_color(inner); glVertex2f(-37, 0); glVertex2f(-29, 0);
    az_gl_color(outer); glVertex2f(-36, -2); glVertex2f(-29, -2);
  } glEnd();
  // Panels:
  glBegin(GL_QUADS); {
    // Front:
    glColor3f(0.2f + 0.8f * flare, 0.25f, 0.25f + 0.75f * frozen);
    glVertex2f(30, -10); glVertex2f(30, 10);
    glColor3f(0.6f + 0.4f * flare, 0.65f, 0.55f + 0.45f * frozen);
    glVertex2f(10, 12); glVertex2f(10, -12);
    // Rear:
    glVertex2f(-25, -9); glVertex2f(-25, 9); glVertex2f(-10, 14);
    glColor3f(0.35f + 0.5f * flare, 0.4f, 0.4f + 0.45f * frozen);
    glVertex2f(-10, -14);
  } glEnd();
  // Cab siding:
  glBegin(GL_TRIANGLES); {
    // Left side:
    glColor3f(0.25f + 0.75f * flare, 0.3f, 0.2f + 0.8f * frozen);
    glVertex2f(30, 10);
    glVertex2f(10, 16);
    glColor3f(0.65f + 0.35f * flare, 0.65f, 0.55f + 0.45f * frozen);
    glVertex2f(10, 8);
    // Right side:
    glVertex2f(10, -8);
    glColor3f(0.25f + 0.75f * flare, 0.3f, 0.2f + 0.8f * frozen);
    glVertex2f(30, -10);
    glVertex2f(10, -16);
  } glEnd();
  // Body siding:
  glBegin(GL_QUAD_STRIP); {
    az_gl_color(outer); glVertex2f(10, 14);
    az_gl_color(inner); glVertex2f(10, 6);
    az_gl_color(outer); glVertex2f(-24, 14);
    az_gl_color(inner); glVertex2f(-22, 9);
    az_gl_color(outer); glVertex2f(-30, 8);
    az_gl_color(inner); glVertex2f(-25, 6);
    az_gl_color(outer); glVertex2f(-30, -8);
    az_gl_color(inner); glVertex2f(-25, -6);
    az_gl_color(outer); glVertex2f(-24, -14);
    az_gl_color(inner); glVertex2f(-22, -9);
    az_gl_color(outer); glVertex2f(10, -14);
    az_gl_color(inner); glVertex2f(10, -6);
  } glEnd();
  // Cracks:
  const double hurt = 1.0 - baddie->health / baddie->data->max_health;
  az_draw_cracks((az_vector_t){30, 3}, AZ_DEG2RAD(180), 5 * hurt);
  az_draw_cracks((az_vector_t){-20, -14}, AZ_DEG2RAD(90), 3.5 * hurt);
  az_draw_cracks((az_vector_t){-30, 6}, AZ_DEG2RAD(0), 3.5 * hurt);
}

/*===========================================================================*/
