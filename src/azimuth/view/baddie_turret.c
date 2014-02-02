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

#include "azimuth/view/baddie_turret.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/baddie.h"
#include "azimuth/util/clock.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static az_color_t color3(float r, float g, float b) {
  return (az_color_t){r * 255, g * 255, b * 255, 255};
}

/*===========================================================================*/

static void draw_crawling_turret_legs(float flare, float frozen,
                                      az_clock_t clock) {
  glBegin(GL_QUADS); {
    const az_color_t dark = color3(
        0.25 + 0.1 * flare - 0.1 * frozen,
        0.25 - 0.1 * flare - 0.1 * frozen,
        0.25 - 0.1 * flare + 0.1 * frozen);
    const int zig = az_clock_zigzag(5, 6, clock);
    az_gl_color(dark); glVertex2f(0, 10); glVertex2f(-21, -10 + zig);
    glColor3f(0.4 + 0.25 * flare - 0.25 * frozen,
              0.4 - 0.25 * flare - 0.25 * frozen,
              0.4 - 0.25 * flare + 0.25 * frozen);
    glVertex2f(-20, -20 + zig); glVertex2f(0, -10);
    glVertex2f(0, 10); glVertex2f(-20, 20 - zig);
    az_gl_color(dark); glVertex2f(-21, 10 - zig); glVertex2f(0, -10);
  } glEnd();
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

/*===========================================================================*/

void az_draw_bad_normal_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
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
}

void az_draw_bad_armored_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
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
}

void az_draw_bad_beam_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
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
}

void az_draw_bad_broken_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
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
}

void az_draw_bad_heavy_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  const az_color_t far_edge = color3(
      0.20 + 0.1 * flare - 0.1 * frozen,
      0.20 - 0.1 * flare - 0.1 * frozen,
      0.25 - 0.1 * flare + 0.1 * frozen);
  const az_color_t mid_edge = color3(
      0.30 + 0.15 * flare - 0.15 * frozen,
      0.30 - 0.15 * flare - 0.15 * frozen,
      0.35 - 0.15 * flare + 0.15 * frozen);
  const az_color_t near_edge = color3(
      0.4 + 0.25 * flare - 0.25 * frozen,
      0.4 - 0.20 * flare - 0.20 * frozen,
      0.5 - 0.25 * flare + 0.25 * frozen);
  const az_color_t center = color3(
      0.5 + 0.4 * flare - 0.3 * frozen,
      0.5 - 0.2 * flare - 0.2 * frozen,
      0.6 - 0.3 * flare + 0.4 * frozen);
  const az_color_t gun_edge =
    color3(0.2 + 0.25 * flare, 0.2, 0.25 + 0.25 * frozen);
  const az_color_t gun_middle =
    color3(0.6 + 0.25 * flare, 0.6, 0.75 + 0.25 * frozen);
  draw_turret_body_outer_edge(baddie, far_edge, mid_edge);
  glPushMatrix(); {
    glRotated(AZ_RAD2DEG(baddie->components[0].angle), 0, 0, 1);
    for (int i = -1; i <= 1; i += 2) {
      glBegin(GL_QUAD_STRIP); {
        az_gl_color(gun_edge);
        glVertex2f(0,  2 * i); glVertex2f(30,  2 * i);
        az_gl_color(gun_middle);
        glVertex2f(0,  6 * i); glVertex2f(30,  6 * i);
        az_gl_color(gun_edge);
        glVertex2f(0, 10 * i); glVertex2f(30, 10 * i);
      } glEnd();
    }
  } glPopMatrix();
  draw_turret_body_center(baddie, mid_edge, near_edge, center);
}

void az_draw_bad_rocket_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
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
}

void az_draw_bad_crawling_turret(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  draw_crawling_turret_legs(baddie->armor_flare, frozen, clock);
  az_draw_bad_normal_turret(baddie, frozen, clock);
}

void az_draw_bad_crawling_mortar(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_crawling_turret_legs(flare, frozen, clock);
  draw_turret(baddie,
              color3(0.1 + 0.1 * flare - 0.1 * frozen, 0.1 - 0.05 * flare,
                     0.1 - 0.1 * flare + 0.1 * frozen),
              color3(0.2 + 0.15 * flare - 0.15 * frozen, 0.2 - 0.1 * flare,
                     0.2 - 0.15 * flare + 0.15 * frozen),
              color3(0.3 + 0.25 * flare - 0.25 * frozen, 0.3 - 0.15 * flare,
                     0.3 - 0.25 * flare + 0.25 * frozen),
              color3(0.4 + 0.4 * flare - 0.3 * frozen, 0.4 - 0.2 * flare,
                     0.4 - 0.3 * flare + 0.4 * frozen),
              color3(0.1 + 0.25 * flare, 0.1, 0.1 + 0.25 * frozen),
              color3(0.5 + 0.25 * flare, 0.5, 0.5 + 0.25 * frozen));
}

void az_draw_bad_security_drone(
    const az_baddie_t *baddie, float frozen, az_clock_t clock) {
  const float flare = baddie->armor_flare;
  draw_turret_body_outer_edge(
      baddie,
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
  draw_turret_body_center(
      baddie,
      color3(0.35 + 0.15 * flare - 0.15 * frozen,
             0.35 - 0.15 * flare - 0.15 * frozen,
             0.35 - 0.15 * flare + 0.15 * frozen),
      color3(0.5 + 0.25 * flare - 0.25 * frozen,
             0.5 - 0.25 * flare - 0.25 * frozen,
             0.5 - 0.25 * flare + 0.25 * frozen),
      color3(0.6 + 0.4 * flare - 0.3 * frozen,
             0.6 - 0.3 * flare - 0.3 * frozen,
             0.6 - 0.3 * flare + 0.4 * frozen));
}

/*===========================================================================*/
