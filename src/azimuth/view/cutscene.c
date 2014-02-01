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

#include "azimuth/view/cutscene.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <GL/gl.h>

#include "azimuth/constants.h"
#include "azimuth/state/ship.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/ship.h"

/*===========================================================================*/

// Below is a simple random number generator, based on the MWC and UNI
// algorithms from George Marsaglia's post to sci.stat.math on 12 Jan 1999,
// which can be found here:
//   https://groups.google.com/forum/#!topic/sci.stat.math/5yb0jwf1stw
// Using this rather than az_random() allows us to easily produce the same
// "random" sequence over and over, which we use to generate a starfield that
// looks random, but can be consistently regenerated on each frame.
static uint32_t sr_z, sr_w;
static void reset_simple_random(void) { sr_z = sr_w = 1; }
static double simple_random(void) {
  sr_z = 36969 * (sr_z & 0xffff) + (sr_z >> 16);
  sr_w = 18000 * (sr_w & 0xffff) + (sr_w >> 16);
  return ((sr_z << 16) + sr_w + 1.0) * 2.328306435454494e-10;
}

static void draw_moving_stars_layer(
    double spacing, double scale, double speed, double total_time,
    GLfloat alpha) {
  const double modulus = AZ_SCREEN_WIDTH + spacing;
  const double scroll = fmod(total_time * speed, modulus);
  reset_simple_random();
  glBegin(GL_LINES); {
    for (double xoff = 0.0; xoff < modulus; xoff += spacing) {
      for (double yoff = 0.0; yoff < modulus; yoff += spacing) {
        const double x =
          fmod(xoff + 3.0 * spacing * simple_random() + scroll, modulus);
        const double y = yoff + 3.0 * spacing * simple_random();
        glColor4f(1, 1, 1, alpha);
        glVertex2d(x, y);
        glColor4f(1, 1, 1, 0);
        glVertex2d(x - spacing * scale, y);
      }
    }
  } glEnd();
}

static void draw_moving_starfield(double time, double scale) {
  draw_moving_stars_layer(30, scale,  450, time, 0.15f);
  draw_moving_stars_layer(45, scale,  600, time, 0.25f);
  draw_moving_stars_layer(80, scale,  900, time, 0.40f);
  draw_moving_stars_layer(95, scale, 1200, time, 0.50f);
}

#define STAR_SPACING 12

void az_draw_planet_starfield(az_clock_t clock) {
  reset_simple_random();
  glBegin(GL_POINTS); {
    int i = 0;
    for (int xoff = 0; xoff < AZ_SCREEN_WIDTH; xoff += STAR_SPACING) {
      for (int yoff = 0; yoff < AZ_SCREEN_HEIGHT; yoff += STAR_SPACING) {
        const int twinkle = az_clock_zigzag(10, 4, clock + i);
        glColor4f(1, 1, 1, (twinkle * 0.02) + 0.3 * simple_random());
        const double x = xoff + 3 * STAR_SPACING * simple_random();
        const double y = yoff + 3 * STAR_SPACING * simple_random();
        glVertex2d(x, y);
        ++i;
      }
    }
  } glEnd();
}

#define PLANET_RADIUS 130.0

void az_draw_zenith_planet(az_clock_t clock) {
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);

    // Draw the planet itself:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0.1, 0, 0.1, 1);
      glVertex2d(0.15 * PLANET_RADIUS, -0.23 * PLANET_RADIUS);
      glColor4f(0.25, 0.15, 0.15, 1);
      for (int theta = 0; theta <= 360; theta += 6) {
        glVertex2d(PLANET_RADIUS * cos(AZ_DEG2RAD(theta)),
                   PLANET_RADIUS * sin(AZ_DEG2RAD(theta)));
      }
    } glEnd();

    // Draw planet "atmosphere":
    const double atmosphere_thickness = 18.0 + az_clock_zigzag(10, 8, clock);
    glBegin(GL_TRIANGLE_STRIP); {
      for (int theta = 0; theta <= 360; theta += 6) {
        glColor4f(0, 1, 1, 0.5);
        glVertex2d(PLANET_RADIUS * cos(AZ_DEG2RAD(theta)),
                   PLANET_RADIUS * sin(AZ_DEG2RAD(theta)));
        glColor4f(0, 1, 1, 0);
        glVertex2d((PLANET_RADIUS + atmosphere_thickness) *
                   cos(AZ_DEG2RAD(theta + 3)),
                   (PLANET_RADIUS + atmosphere_thickness) *
                   sin(AZ_DEG2RAD(theta + 3)));
      }
    } glEnd();
  } glPopMatrix();
}

static void tint_screen(GLfloat gray, GLfloat alpha) {
  glColor4f(gray, gray, gray, alpha);
  glBegin(GL_QUADS); {
    glVertex2i(0, 0);
    glVertex2i(0, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
    glVertex2i(AZ_SCREEN_WIDTH, 0);
  } glEnd();
}

/*===========================================================================*/

static void draw_cruising_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_CRUISING);
  draw_moving_starfield(cutscene->time, 1.0);
  az_ship_t ship = {
    .position = {320, 240}, .angle = AZ_PI, .thrusters = AZ_THRUST_FORWARD
  };
  az_draw_ship_body(&ship, clock);
}

static void draw_move_out_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_MOVE_OUT);
  const double accel = cutscene->param1;
  draw_moving_starfield(cutscene->time, 1.0 + 5.0 * accel * accel);
  az_ship_t ship = {
    .position = {320 - 350 * (2 * accel * accel - accel), 240},
    .angle = AZ_PI, .thrusters = AZ_THRUST_FORWARD
  };
  az_draw_ship_body(&ship, clock);
  if (cutscene->param2 > 0.0) tint_screen(1, cutscene->param2);
}

static void draw_arrival_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  az_draw_planet_starfield(clock);
  az_draw_zenith_planet(clock);
  glPushMatrix(); {
    const az_vector_t ctrl0 = {700, 20}, ctrl1 = {600, 300};
    const az_vector_t ctrl2 = {0, 520}, ctrl3 = {220, 290};
    const double t = 1.0 - exp(-5.0 * cutscene->param1);
    const double s = 1.0 - t;
    const az_vector_t pos =
      az_vadd(az_vadd(az_vmul(ctrl0, s*s*s),
                      az_vmul(ctrl1, 3*s*s*t)),
              az_vadd(az_vmul(ctrl2, 3*s*t*t),
                      az_vmul(ctrl3, t*t*t)));
    const double angle = az_vtheta(
      az_vadd(az_vadd(az_vmul(ctrl0, -3*s*s),
                      az_vmul(ctrl1, 3*s*s - 6*s*t)),
              az_vadd(az_vmul(ctrl2, 6*s*t - 3*t*t),
                      az_vmul(ctrl3, 3*t*t))));
    glTranslatef(pos.x, pos.y, 0);
    const double scale = fmax(0.05, exp(1.5 - 8.0 * cutscene->param1));
    glScaled(scale, scale, 1);
    az_ship_t ship = {
      .position = {0, 0}, .angle = angle, .thrusters = AZ_THRUST_FORWARD
    };
    az_draw_ship_body(&ship, clock);
  } glPopMatrix();
}

/*===========================================================================*/

void az_draw_cutscene(const az_space_state_t *state) {
  const az_cutscene_state_t *cutscene = &state->cutscene;
  assert(cutscene->scene != AZ_SCENE_NOTHING);
  switch (cutscene->scene) {
    case AZ_SCENE_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_SCENE_CRUISING:
      draw_cruising_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_MOVE_OUT:
      draw_move_out_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ARRIVAL:
      draw_arrival_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ESCAPE: break; // TODO
    case AZ_SCENE_HOMECOMING: break; // TODO
  }
  if (cutscene->fade_alpha > 0.0) tint_screen(0, cutscene->fade_alpha);
}

/*===========================================================================*/
