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
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/ship.h"
#include "azimuth/view/string.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void draw_moving_stars_layer(
    double spacing, double scale, double speed, double total_time,
    GLfloat alpha) {
  const double modulus = AZ_SCREEN_WIDTH + spacing;
  const double scroll = fmod(total_time * speed, modulus);
  az_random_seed_t seed = {1, 1};
  glBegin(GL_LINES); {
    for (double xoff = 0.0; xoff < modulus; xoff += spacing) {
      for (double yoff = 0.0; yoff < modulus; yoff += spacing) {
        const double x =
          fmod(xoff + 3.0 * spacing * az_rand_udouble(&seed) + scroll,
               modulus);
        const double y = yoff + 3.0 * spacing * az_rand_udouble(&seed);
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
  az_random_seed_t seed = {1, 1};
  glBegin(GL_POINTS); {
    int i = 0;
    for (int xoff = 0; xoff < AZ_SCREEN_WIDTH; xoff += STAR_SPACING) {
      for (int yoff = 0; yoff < AZ_SCREEN_HEIGHT; yoff += STAR_SPACING) {
        const int twinkle = az_clock_zigzag(10, 4, clock + i);
        glColor4f(1, 1, 1, (twinkle * 0.02) + 0.3 * az_rand_udouble(&seed));
        const double x = xoff + 3 * STAR_SPACING * az_rand_udouble(&seed);
        const double y = yoff + 3 * STAR_SPACING * az_rand_udouble(&seed);
        glVertex2d(x, y);
        ++i;
      }
    }
  } glEnd();
}

#define BASE_PLANET_RADIUS 130.0

static double planet_radius(double theta, double deform) {
  assert(0.0 <= deform && deform <= 1.0);
  return (deform <= 0.0 ? BASE_PLANET_RADIUS :
          BASE_PLANET_RADIUS * (1.0 - pow(deform, 5)) *
          (1.0 + 0.5 * pow(deform, 3) *
           (cos(17 * theta + AZ_TWO_PI * deform) +
            sin(7 * theta))));
}

static void draw_zenith_planet_internal(double blacken, double create,
                                        double deform, az_clock_t clock) {
  assert(0.0 <= blacken && blacken <= 1.0);
  assert(0.0 <= create && create <= 1.0);
  assert(0.0 <= deform && deform <= 1.0);
  glPushMatrix(); {
    glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);

    if (create < 1.0) {
      // Blacken circle:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0, 0, 0, blacken);
        glVertex2f(0, 0);
        for (int i = 0; i <= 360; i += 3) {
          const double theta = AZ_DEG2RAD(i);
          const double radius = planet_radius(theta, deform);
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();

      // Draw fragments:
      if (blacken > 0.0 && blacken < 1.0) {
        for (int i = 0; i < 360; i += 20) {
          glPushMatrix(); {
            az_gl_translated(az_vpolar(sqrt(blacken) * BASE_PLANET_RADIUS,
                                       AZ_DEG2RAD(i)));
            az_gl_rotated(2 * AZ_TWO_PI * blacken);
            glBegin(GL_TRIANGLES); {
              const double radius = 10.0 * (1.0 - blacken);
              for (int j = 0; j < 3; ++j) {
                const az_clock_t clk = clock + 2 * j;
                glColor3f((az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f),
                          (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f),
                          (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f));
                glVertex2d(radius * cos(AZ_DEG2RAD(j * 120)),
                           radius * sin(AZ_DEG2RAD(j * 120)));
              }
            } glEnd();
          } glPopMatrix();
        }
      }

      // Draw tendrils:
      const double overall_progress = 0.25 * blacken + 0.75 * create;
      const int num_tendrils = 21;
      for (int i = 1; i <= num_tendrils; ++i) {
        const double progress =
          cbrt((overall_progress - (double)i / num_tendrils) /
               (1.0 - (double)i / num_tendrils));
        if (progress > 0.0) {
          const az_clock_t clk = clock + 7 * i;
          const GLfloat r = (az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f);
          const GLfloat g = (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f);
          const GLfloat b = (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f);
          glColor4f(r, g, b, 0.5f);
          glBegin(GL_TRIANGLE_STRIP); {
            const double angle = AZ_DEG2RAD(108) * i;
            az_random_seed_t seed = {i, i};
            az_vector_t vec = AZ_VZERO;
            const az_vector_t step = az_vpolar(1.2, angle);
            const az_vector_t side = az_vrot90ccw(step);
            for (double j = 0.0; j <= progress; j += 0.01) {
              const az_vector_t edge =
                az_vmul(side, 3.0 * (progress - j) / progress);
              const az_vector_t wobble = {
                0, atan(0.02 * vec.x) * 10.0 *
                sin(overall_progress * 30.0 + vec.x / 10.0)};
              az_gl_vertex(az_vadd(az_vadd(vec, edge), wobble));
              az_gl_vertex(az_vadd(az_vsub(vec, edge), wobble));
              az_vpluseq(&vec, step);
              az_vpluseq(&vec, az_vmul(side, 2 * az_rand_sdouble(&seed)));
            }
          } glEnd();
        }
      }

      // Draw portal:
      glBegin(GL_TRIANGLE_FAN); {
        const GLfloat r = (az_clock_mod(6, 1, clock)     < 3 ? 1.0f : 0.25f);
        const GLfloat g = (az_clock_mod(6, 1, clock + 2) < 3 ? 1.0f : 0.25f);
        const GLfloat b = (az_clock_mod(6, 1, clock + 4) < 3 ? 1.0f : 0.25f);
        glColor4f(r, g, b, 1);
        glVertex2f(0, 0);
        glColor4f(r, g, b, 0);
        const double radius =
          blacken * (30 + 0.15 * az_clock_zigzag(90, 1, clock));
        for (int i = 0; i <= 360; i += 10) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)) * 0.8);
        }
      } glEnd();
    }

    // Draw the planet itself:
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(0.5, 0.3, 0.5, create);
      const double spot_theta = AZ_DEG2RAD(-125);
      const double spot_radius = 0.15 * planet_radius(spot_theta, deform);
      glVertex2d(spot_radius * cos(spot_theta), spot_radius * sin(spot_theta));
      glColor4f(0.25, 0.15, 0.15, create);
      for (int i = 0; i <= 360; i += 3) {
        const double theta = AZ_DEG2RAD(i);
        const double radius = planet_radius(theta, deform);
        glVertex2d(radius * cos(theta), radius * sin(theta));
      }
    } glEnd();

    // Draw planet "atmosphere":
    const double atmosphere_thickness =
      create * (18.0 + (1.0 - deform) * az_clock_zigzag(10, 8, clock));
    glBegin(GL_TRIANGLE_STRIP); {
      for (int i = 0; i <= 360; i += 3) {
        const double theta = AZ_DEG2RAD(i);
        const double radius = planet_radius(theta, deform);
        glColor4f(0, 1, 1, 0.5);
        glVertex2d(radius * cos(theta), radius * sin(theta));
        glColor4f(0, 1, 1, 0);
        glVertex2d((radius + atmosphere_thickness) * cos(theta),
                   (radius + atmosphere_thickness) * sin(theta));
      }
    } glEnd();
  } glPopMatrix();
}

void az_draw_zenith_planet(az_clock_t clock) {
  draw_zenith_planet_internal(0, 1, 0, clock);
}

void az_draw_zenith_planet_formation(double blacken, double create,
                                     az_clock_t clock) {
  draw_zenith_planet_internal(blacken, create, 0.0, clock);
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
  assert(cutscene->scene == AZ_SCENE_ARRIVAL);
  az_draw_planet_starfield(clock);
  az_draw_zenith_planet(clock);
  glPushMatrix(); {
    const az_vector_t ctrl0 = {700, 20}, ctrl1 = {500, 600};
    const az_vector_t ctrl2 = {-100, 420}, ctrl3 = {220, 290};
    const double t = 1.0 - exp(-7.0 * cutscene->param1);
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
    const double scale = fmax(0.05, exp(1.5 - 12.0 * cutscene->param1));
    glScaled(scale, scale, 1);
    az_ship_t ship = {
      .position = {0, 0}, .angle = angle, .thrusters = AZ_THRUST_FORWARD
    };
    az_draw_ship_body(&ship, clock);
  } glPopMatrix();
}

static void draw_zenith_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_ZENITH);
  az_draw_planet_starfield(clock);
  az_draw_zenith_planet(clock);
}

static void draw_escape_scene(
    const az_cutscene_state_t *cutscene, az_clock_t clock) {
  assert(cutscene->scene == AZ_SCENE_ESCAPE);
  az_draw_planet_starfield(clock);
  if (cutscene->step == 0) {
    // Step 0: Deform planet
    draw_zenith_planet_internal(0, 1, cutscene->param1, clock);
  } else if (cutscene->step == 1) {
    // Step 1: Asplode planet, with ship escaping
    glPushMatrix(); {
      // Explosion:
      glTranslated(AZ_SCREEN_WIDTH/2, AZ_SCREEN_HEIGHT/2, 0);
      glBegin(GL_TRIANGLE_FAN); {
        const double blast = pow(fmin(1.0, 3 * cutscene->param1), 3);
        glColor4f(blast, 1, 1, 0.5 + 0.5 * blast);
        glVertex2f(0, 0);
        glColor4f(blast, 1, 1, blast);
        for (int i = 0; i <= 360; i += 3) {
          const double theta = AZ_DEG2RAD(i);
          const double radius = 18.0 + blast * AZ_SCREEN_RADIUS;
          glVertex2d(1.5 * radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      // Escaping ship shadow:
      glPushMatrix(); {
        const double translation = 1.2 * pow(cutscene->param1, 4);
        glTranslated(-320 * translation, 270 * translation, 0);
        const GLfloat scale_factor = 3.0 * translation;
        glScalef(scale_factor, 0.66f * scale_factor, 1);
        az_gl_rotated(AZ_DEG2RAD(125));
        glColor3f(0, 0, 0);
        // Engines:
        glBegin(GL_QUADS); {
          // Struts:
          glVertex2f( 1,  9); glVertex2f(-7,  9);
          glVertex2f(-7, -9); glVertex2f( 1, -9);
          // Port engine:
          glVertex2f(-10,  12); glVertex2f(  6,  12);
          glVertex2f(  8,   7); glVertex2f(-11,   7);
          // Starboard engine:
          glVertex2f(  8,  -7); glVertex2f(-11,  -7);
          glVertex2f(-10, -12); glVertex2f(  6, -12);
        } glEnd();
        // Main body:
        glBegin(GL_TRIANGLE_FAN); {
          glVertex2f(20, 0); glVertex2f( 15,  4); glVertex2f(-14,  4);
          glVertex2f(-15,  3); glVertex2f(-16,  0); glVertex2f(-15, -3);
          glVertex2f(-14, -4); glVertex2f( 15, -4);
        } glEnd();
      } glPopMatrix();
    } glPopMatrix();
  } else if (cutscene->step == 2) {
    // Step 2: Explosion fades away
    glBegin(GL_TRIANGLE_FAN); {
      glColor4f(1, 1, 1, 1.0 - cutscene->param1);
      glVertex2i(0, 0); glVertex2i(AZ_SCREEN_WIDTH, 0);
      glVertex2i(AZ_SCREEN_WIDTH, AZ_SCREEN_HEIGHT);
      glVertex2i(0, AZ_SCREEN_HEIGHT);
    } glEnd();
  }
}

/*===========================================================================*/

void az_draw_cutscene(const az_space_state_t *state) {
  const az_cutscene_state_t *cutscene = &state->cutscene;
  assert(cutscene->scene != AZ_SCENE_NOTHING);
  switch (cutscene->scene) {
    case AZ_SCENE_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_SCENE_TEXT:
      assert(cutscene->scene_text != NULL);
      az_draw_paragraph(16, AZ_ALIGN_CENTER, AZ_SCREEN_WIDTH/2,
                        AZ_SCREEN_HEIGHT/2 - 8, 20, -1,
                        state->prefs, cutscene->scene_text);
      break;
    case AZ_SCENE_CRUISING:
      draw_cruising_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_MOVE_OUT:
      draw_move_out_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ARRIVAL:
      draw_arrival_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ZENITH:
      draw_zenith_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_ESCAPE:
      draw_escape_scene(cutscene, state->clock);
      break;
    case AZ_SCENE_HOMECOMING: break; // TODO
    case AZ_SCENE_BLACK: break;
  }
  if (cutscene->fade_alpha > 0.0) tint_screen(0, cutscene->fade_alpha);
}

/*===========================================================================*/
