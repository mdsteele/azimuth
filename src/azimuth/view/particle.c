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

#include "azimuth/view/particle.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <GL/gl.h>

#include "azimuth/state/particle.h"
#include "azimuth/state/space.h"
#include "azimuth/util/clock.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/random.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/util.h"

/*===========================================================================*/

static void with_color_alpha(az_color_t color, double alpha_factor) {
  glColor4ub(color.r, color.g, color.b, color.a * alpha_factor);
}

static void draw_bolt_glowball(az_color_t color, double cx, az_clock_t clock) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(1, 1, 1, 0.75);
    glVertex2d(cx, 0);
    with_color_alpha(color, 0);
    const double rad = 8 + az_clock_zigzag(5, 8, clock);
    for (int i = 0; i <= 360; i += 30) {
      glVertex2d(rad * cos(AZ_DEG2RAD(i)) + cx, rad * sin(AZ_DEG2RAD(i)));
    }
  } glEnd();
}

/*===========================================================================*/

static void draw_particle(const az_particle_t *particle, az_clock_t clock) {
  assert(particle->kind != AZ_PAR_NOTHING);
  assert(particle->age <= particle->lifetime);
  switch (particle->kind) {
    case AZ_PAR_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_PAR_BOOM:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 0);
        glVertex2d(0, 0);
        const double ratio = particle->age / particle->lifetime;
        with_color_alpha(particle->color, 1 - ratio * ratio);
        const double radius = particle->param1 * ratio;
        for (int i = 0; i <= 16; ++i) {
          glVertex2d(radius * cos(i * AZ_PI_EIGHTHS),
                     radius * sin(i * AZ_PI_EIGHTHS));
        }
      } glEnd();
      break;
    case AZ_PAR_BEAM:
      glBegin(GL_QUAD_STRIP); {
        const double alpha = (particle->lifetime <= 0.0 ? 1.0 :
                              1.0 - particle->age / particle->lifetime);
        with_color_alpha(particle->color, 0);
        glVertex2d(0, particle->param2);
        glVertex2d(particle->param1, particle->param2);
        with_color_alpha(particle->color, alpha);
        glVertex2d(0, 0);
        glVertex2d(particle->param1, 0);
        with_color_alpha(particle->color, 0);
        glVertex2d(0, -particle->param2);
        glVertex2d(particle->param1, -particle->param2);
      } glEnd();
      break;
    case AZ_PAR_EMBER:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0);
        const double radius =
          particle->param1 * (1.0 - particle->age / particle->lifetime);
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PAR_EXPLOSION:
      glBegin(GL_QUAD_STRIP); {
        const double tt = 1.0 - particle->age / particle->lifetime;
        const double inner_alpha = tt * tt;
        const double outer_alpha = tt;
        const double inner_radius = particle->param1 * (1.0 - tt * tt * tt);
        const double outer_radius = particle->param1;
        for (int i = 0; i <= 360; i += 6) {
          const double c = cos(AZ_DEG2RAD(i)), s = sin(AZ_DEG2RAD(i));
          with_color_alpha(particle->color, inner_alpha);
          glVertex2d(inner_radius * c, inner_radius * s);
          with_color_alpha(particle->color, outer_alpha);
          glVertex2d(outer_radius * c, outer_radius * s);
        }
      } glEnd();
      break;
    case AZ_PAR_FIRE_BOOM: {
      const int i_step = 10;
      const double liveness = 1.0 - particle->age / particle->lifetime;
      const double x_radius = particle->param1;
      const double y_radius = particle->param1 * liveness;
      for (int i = 0; i < 180; i += i_step) {
        glBegin(GL_TRIANGLE_STRIP); {
          const int limit = 180 * liveness;
          for (int j = 0; j < limit; j += 20) {
            glColor4f(1.0, 0.75 * j / limit, 0.0,
                      0.35 + 0.25 * sin(AZ_DEG2RAD(i)) * sin(AZ_DEG2RAD(j)) -
                      0.35 * j / limit);
            const double x = x_radius * cos(AZ_DEG2RAD(j));
            glVertex2d(x, y_radius * cos(AZ_DEG2RAD(i)) *
                       sin(AZ_DEG2RAD(j)));
            glVertex2d(x, y_radius * cos(AZ_DEG2RAD(i + i_step)) *
                       sin(AZ_DEG2RAD(j)));
          }
          glVertex2d(x_radius * cos(AZ_DEG2RAD(limit)),
                     y_radius * cos(AZ_DEG2RAD(i + i_step/2)) *
                     sin(AZ_DEG2RAD(limit)));
        } glEnd();
      }
    } break;
    case AZ_PAR_LIGHTNING_BOLT:
      if (particle->age >= particle->param2) {
        const int num_steps = az_imax(2, round(particle->param1 / 10.0));
        const double step = particle->param1 / num_steps;
        az_random_seed_t seed = { clock / 5, 194821.0 * particle->angle };
        az_vector_t prev = {0, 0};
        for (int i = 1; i <= num_steps; ++i) {
          const az_vector_t next =
            (i == num_steps ? (az_vector_t){particle->param1, 0} :
             (az_vector_t){3.0 * az_rand_sdouble(&seed) + i * step,
                           10.0 * az_rand_sdouble(&seed)});
          const az_vector_t side =
            az_vwithlen(az_vrot90ccw(az_vsub(next, prev)), 4);
          glBegin(GL_TRIANGLE_STRIP); {
            with_color_alpha(particle->color, 0);
            az_gl_vertex(az_vadd(prev, side));
            az_gl_vertex(az_vadd(next, side));
            glColor4f(1, 1, 1, 0.5);
            az_gl_vertex(prev); az_gl_vertex(next);
            with_color_alpha(particle->color, 0);
            az_gl_vertex(az_vsub(prev, side));
            az_gl_vertex(az_vsub(next, side));
          } glEnd();
          prev = next;
        }
        draw_bolt_glowball(particle->color, particle->param1, clock);
      }
      draw_bolt_glowball(particle->color, 0, clock);
      break;
    case AZ_PAR_OTH_FRAGMENT:
      glRotated(particle->age * AZ_RAD2DEG(particle->param2), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        const double radius =
          particle->param1 * (1.0 - particle->age / particle->lifetime);
        for (int i = 0; i < 3; ++i) {
          const az_clock_t clk = clock + 2 * i;
          glColor3f((az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f),
                    (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f),
                    (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f));
          glVertex2d(radius * cos(AZ_DEG2RAD(i * 120)),
                     radius * sin(AZ_DEG2RAD(i * 120)));
        }
      } glEnd();
      break;
    case AZ_PAR_NPS_PORTAL:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0.5);
        const double radius = particle->param1 *
          (1.0 - 2.0 * fabs((particle->age / particle->lifetime) - 0.5));
        for (int i = 0; i <= 360; i += 5) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PAR_SHARD:
      glScaled(particle->param1, particle->param1, 1);
      glRotated(particle->age * AZ_RAD2DEG(particle->param2), 0, 0, 1);
      glBegin(GL_TRIANGLES); {
        az_color_t color = particle->color;
        const double alpha = 1.0 - particle->age / particle->lifetime;
        with_color_alpha(color, alpha);
        glVertex2f(2, 3);
        color.r *= 0.6; color.g *= 0.6; color.b *= 0.6;
        with_color_alpha(color, alpha);
        glVertex2f(-2, 4);
        color.r *= 0.6; color.g *= 0.6; color.b *= 0.6;
        with_color_alpha(color, alpha);
        glVertex2f(0, -4);
      } glEnd();
      break;
    case AZ_PAR_SPARK:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.8);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0);
        const double radius =
          particle->param1 * (1.0 - particle->age / particle->lifetime);
        const double theta_offset = particle->param2 * particle->age;
        for (int i = 0; i <= 360; i += 45) {
          const double rho = (i % 2 ? 1.0 : 0.5) * radius;
          const double theta = AZ_DEG2RAD(i) + theta_offset;
          glVertex2d(rho * cos(theta), rho * sin(theta));
        }
      } glEnd();
      break;
    case AZ_PAR_SPLOOSH:
      glBegin(GL_TRIANGLE_FAN); {
        with_color_alpha(particle->color, 1);
        glVertex2f(0, 0);
        with_color_alpha(particle->color, 0);
        const GLfloat height =
          particle->param1 * sin(AZ_PI * particle->age / particle->lifetime);
        glVertex2f(0, 14);
        glVertex2f(0.3f * height, 7);
        glVertex2f(height, 0);
        glVertex2f(0.3f * height, -7);
        glVertex2f(0, -14);
        glVertex2f(-0.05f * height, 0);
        glVertex2f(0, 14);
      } glEnd();
      break;
  }
}

void az_draw_particles(const az_space_state_t *state) {
  AZ_ARRAY_LOOP(particle, state->particles) {
    if (particle->kind == AZ_PAR_NOTHING) continue;
    glPushMatrix(); {
      az_gl_translated(particle->position);
      az_gl_rotated(particle->angle);
      draw_particle(particle, state->clock);
    } glPopMatrix();
  }
}

/*===========================================================================*/
