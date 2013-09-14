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

#include "azimuth/view/projectile.h"

#include <math.h>

#include <GL/gl.h>

#include "azimuth/state/projectile.h"
#include "azimuth/state/space.h"
#include "azimuth/util/misc.h"
#include "azimuth/util/vector.h"
#include "azimuth/view/gravfield.h"

/*===========================================================================*/

static void draw_rocket(az_clock_t clock, az_color_t color) {
  glColor3ub(color.r, color.g, color.b);
  glBegin(GL_QUADS); {
    const int y = 2 - az_clock_mod(6, 2, clock);
    glVertex2i(-11, y);
    glVertex2i(-11, y + 2);
    glVertex2i(-4, y + 2);
    glVertex2i(-4, y);
  } glEnd();
  glBegin(GL_QUAD_STRIP); {
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2i(-9, -2);
    glVertex2i(2, -2);
    glColor3f(0.75, 0.75, 0.75); // light gray
    glVertex2i(-9, 0);
    glVertex2i(4, 0);
    glColor3f(0.25, 0.25, 0.25); // dark gray
    glVertex2i(-9, 2);
    glVertex2i(2, 2);
  } glEnd();
  glColor3ub(color.r, color.g, color.b);
  glBegin(GL_QUADS); {
    const int y = -4 + az_clock_mod(6, 2, clock);
    glVertex2i(-11, y);
    glVertex2i(-11, y + 2);
    glVertex2i(-4, y + 2);
    glVertex2i(-4, y);
  } glEnd();
}

static void draw_oth_projectile(const az_projectile_t* proj, double radius,
                                az_clock_t clock) {
  const double turn_degrees = 1440.0 * proj->age;
  glBegin(GL_TRIANGLES); {
    for (int i = 0; i < 3; ++i) {
      const az_clock_t clk = clock + 2 * i;
      glColor3f((az_clock_mod(6, 1, clk)     < 3 ? 1.0f : 0.25f),
                (az_clock_mod(6, 1, clk + 2) < 3 ? 1.0f : 0.25f),
                (az_clock_mod(6, 1, clk + 4) < 3 ? 1.0f : 0.25f));
      glVertex2d(radius * cos(AZ_DEG2RAD(i * 120 + turn_degrees)),
                 radius * sin(AZ_DEG2RAD(i * 120 + turn_degrees)));
    }
  } glEnd();
}

static void draw_spark(double age, double radius, az_color_t color) {
  glBegin(GL_TRIANGLE_FAN); {
    glColor4f(1, 1, 1, 0.8);
    glVertex2f(0, 0);
    glColor4ub(color.r, color.g, color.b, color.a);
    for (int i = 0; i <= 360; i += 45) {
      const double r = (i % 2 ? radius : 0.5 * radius);
      const double theta = AZ_DEG2RAD(i + 400 * age);
      glVertex2d(r * cos(theta), r * sin(theta));
    }
  } glEnd();
}

static void draw_projectile(const az_projectile_t* proj, az_clock_t clock) {
  switch (proj->kind) {
    case AZ_PROJ_NOTHING: AZ_ASSERT_UNREACHABLE();
    case AZ_PROJ_GUN_NORMAL:
    case AZ_PROJ_GUN_SHRAPNEL:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.75); // white
        glVertex2f( 0.0,  0.0);
        glVertex2f( 2.0,  0.0);
        glVertex2f( 1.5,  1.5);
        glVertex2f( 0.0,  2.0);
        glVertex2f(-1.5,  1.5);
        glColor4f(1, 1, 1, 0); // transparent white
        glVertex2f(-10.0 * proj->power, 0.0);
        glColor3f(1, 1, 1); // white
        glVertex2f(-1.5, -1.5);
        glVertex2f( 0.0, -2.0);
        glVertex2f( 1.5, -1.5);
        glVertex2f( 2.0,  0.0);
      } glEnd();
      break;
    case AZ_PROJ_GUN_CHARGED_NORMAL:
    case AZ_PROJ_GUN_CHARGED_TRIPLE:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 1, 1, 0.75); // white
        glVertex2f( 0,  0);
        glVertex2f( 4,  0); glVertex2f( 3,  3);
        glVertex2f( 0,  4); glVertex2f(-3,  3);
        glColor4f(1, 1, 1, 0); // transparent white
        glVertex2f(-20 * proj->power, 0);
        glColor3f(1, 1, 1); // white
        glVertex2f(-3, -3); glVertex2f( 0, -4);
        glVertex2f( 3, -3); glVertex2f( 4,  0);
      } glEnd();
      break;
    case AZ_PROJ_GUN_FREEZE:
    case AZ_PROJ_GUN_CHARGED_FREEZE:
    case AZ_PROJ_GUN_FREEZE_HOMING:
    case AZ_PROJ_GUN_FREEZE_SHRAPNEL:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(0.5, 1, 1, 0.75); // cyan
        glVertex2f(0, 0);
        for (int i = 0; i <= 12; ++i) {
          if (i % 2) glColor4f(0.5, 0.5, 1, 0.75); // blue
          else glColor4f(0.5, 1, 1, 0.75); // cyan
          double r = 5.0 - 2.0 * (i % 2);
          if (proj->kind == AZ_PROJ_GUN_CHARGED_FREEZE) r *= 1.5;
          else if (proj->kind == AZ_PROJ_GUN_FREEZE_SHRAPNEL) r *= 0.75;
          double t = AZ_DEG2RAD(30 * i + 3 * az_clock_mod(120, 1, clock));
          glVertex2d(r * cos(t), r * sin(t));
        }
      } glEnd();
      break;
    case AZ_PROJ_GUN_HOMING:
    case AZ_PROJ_GUN_HOMING_SHRAPNEL:
      glBegin(GL_TRIANGLES); {
        glColor3f(0, 0.25, 1);
        glVertex2f(4, 0); glVertex2f(-4, 2); glVertex2f(-4, -2);
      } glEnd();
      break;
    case AZ_PROJ_GUN_CHARGED_HOMING:
      glBegin(GL_TRIANGLES); {
        glColor3f(0, 0.25, 1);
        glVertex2f(8, 0); glVertex2f(-8, 4); glVertex2f(-8, -4);
      } glEnd();
      break;
    case AZ_PROJ_GUN_PHASE:
    case AZ_PROJ_GUN_FREEZE_PHASE:
    case AZ_PROJ_GUN_HOMING_PHASE:
    case AZ_PROJ_GUN_PHASE_BURST:
    case AZ_PROJ_GUN_PHASE_PIERCE:
    case AZ_PROJ_SONIC_WAVE:
      glBegin(GL_QUADS); {
        const double r1 = proj->age * proj->data->speed;
        const double w1 = r1 * tan(AZ_DEG2RAD(0.5));
        const double a = proj->age / proj->data->lifetime;
        const double r2 = fmin(r1, 30.0 * (1.0 - a * a));
        const double w2 = (r1 - r2) * tan(AZ_DEG2RAD(0.5));
        if (proj->kind == AZ_PROJ_GUN_FREEZE_PHASE) glColor3f(0.5, 0.75, 1);
        else if (proj->kind == AZ_PROJ_SONIC_WAVE) glColor4f(1, 1, 1, 0.5);
        else if (proj->kind == AZ_PROJ_GUN_PHASE_PIERCE &&
                 az_clock_mod(2, 2, clock)) glColor3f(1, 0, 1);
        else glColor3f(1, 1, 0.5);
        glVertex2d(0, -w1);
        glVertex2d(0, w1);
        if (proj->kind == AZ_PROJ_GUN_FREEZE_PHASE) glColor4f(0, 0.5, 1, 0);
        else if (proj->kind == AZ_PROJ_SONIC_WAVE) glColor4f(1, 1, 1, 0);
        else if (proj->kind == AZ_PROJ_GUN_PHASE_PIERCE &&
                 az_clock_mod(2, 2, clock)) glColor4f(1, 0, 1, 0);
        else glColor4f(1, 0.5, 0, 0);
        glVertex2d(-r2, w2);
        glVertex2d(-r2, -w2);
      } glEnd();
      break;
    case AZ_PROJ_GUN_CHARGED_PHASE:
      glBegin(GL_QUADS); {
        if (az_clock_mod(2, 2, clock)) glColor3f(1, 1, 0);
        else glColor3f(1, 0, 1);
        glVertex2f(2, -2);
        glVertex2f(2, 2);
        if (az_clock_mod(2, 2, clock)) glColor4f(1, 1, 0, 0);
        else glColor4f(1, 0, 1, 0);
        glVertex2d(-18 - 400 * proj->age, 2);
        glVertex2d(-18 - 400 * proj->age, -2);
      } glEnd();
      break;
    case AZ_PROJ_GUN_BURST:
    case AZ_PROJ_GUN_FREEZE_BURST:
    case AZ_PROJ_GUN_HOMING_BURST:
    case AZ_PROJ_GUN_BURST_PIERCE:
      glPushMatrix(); {
        glRotated(720.0 * proj->age, 0, 0, 1);
        glBegin(GL_QUADS); {
          glColor3f(0.75, 0.5, 0.25); // brown
          glVertex2f( 2, -3); glVertex2f( 5, 0); glVertex2f( 2,  3);
          glColor3f(0.5, 0.25, 0); // dark brown
          glVertex2f(-1, 0); glVertex2f( 1, 0);
          glColor3f(0.75, 0.5, 0.25); // brown
          glVertex2f(-2,  3); glVertex2f(-5, 0); glVertex2f(-2, -3);
        } glEnd();
      } glPopMatrix();
      break;
    case AZ_PROJ_GUN_PIERCE:
    case AZ_PROJ_GUN_FREEZE_PIERCE:
      {
        const GLfloat red =
          (proj->kind == AZ_PROJ_GUN_FREEZE_PIERCE ? 0.3 : 1.0);
        const GLfloat green =
          (proj->kind == AZ_PROJ_GUN_FREEZE_PIERCE ? 0.8 : 0.0);
        glBegin(GL_TRIANGLE_FAN); {
          glColor4f(red, green, 1, 0.75); // magenta
          glVertex2f(2, 0);
          glColor4f(red, green, 1, 0); // transparent magenta
          glVertex2f(0, 4);
          glVertex2f(-50, 0);
          glVertex2f(0, -4);
        } glEnd();
        glBegin(GL_TRIANGLE_FAN); {
          glVertex2f(-2, 0);
          glColor4f(red, green, 1, 0.75); // magenta
          glVertex2f(-6, 8);
          glVertex2f(2, 0);
          glVertex2f(-6, -8);
        } glEnd();
      }
      break;
    case AZ_PROJ_GUN_CHARGED_PIERCE:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0, 1, 0.8);
        glVertex2f(0, 0);
        glColor4f(1, 0, 1, 0);
        for (int i = 0; i <= 360; i += 45) {
          const double radius = (i % 2 ? 20.0 : 10.0);
          const double theta = AZ_DEG2RAD(i + 400 * proj->age);
          glVertex2d(radius * cos(theta), radius * sin(theta));
        }
      } glEnd();
      break;
    case AZ_PROJ_GUN_HOMING_PIERCE:
    case AZ_PROJ_GUN_PIERCE_SHRAPNEL:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0, 1, 0.5);
        glVertex2f(-5, 0);
        glColor3f(1, 0, 1);
        glVertex2f(-8, 5); glVertex2f(6, 0); glVertex2f(-8, -5);
      } glEnd();
      break;
    case AZ_PROJ_GUN_CHARGED_BEAM:
      glBegin(GL_TRIANGLE_FAN); {
        const double ratio = proj->age / proj->data->lifetime;
        const double radius = proj->data->splash_radius * ratio;
        glColor4f(1, 0, 0, 0);
        glVertex2f(0, 0);
        glColor4f(1, 0, 0, 1 - ratio);
        for (int i = 0; i <= 360; i += 15) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_ROCKET:
      draw_rocket(clock, (az_color_t){128, 0, 0, 255});
      break;
    case AZ_PROJ_HYPER_ROCKET:
    case AZ_PROJ_MISSILE_PIERCE:
      draw_rocket(clock, (az_color_t){192, 0, 192, 255});
      break;
    case AZ_PROJ_MISSILE_FREEZE:
      draw_rocket(clock, (az_color_t){0, 192, 192, 255});
      break;
    case AZ_PROJ_MISSILE_BARRAGE: break; // invisible
    case AZ_PROJ_MISSILE_TRIPLE:
      draw_rocket(clock, (az_color_t){0, 192, 0, 255});
      break;
    case AZ_PROJ_MISSILE_HOMING:
      draw_rocket(clock, (az_color_t){0, 0, 255, 255});
      break;
    case AZ_PROJ_MISSILE_PHASE:
      draw_rocket(clock, (az_color_t){192, 192, 0, 255});
      break;
    case AZ_PROJ_MISSILE_BURST:
      draw_rocket(clock, (az_color_t){192, 96, 0, 255});
      break;
    case AZ_PROJ_MISSILE_BEAM:
      glBegin(GL_TRIANGLE_FAN); {
        glColor4f(1, 0, 0, 0.8);
        glVertex2f(0, 0);
        glColor4f(1, 0, 0, 0);
        for (int i = 0; i <= 360; i += 45) {
          const double radius = (i % 2 ? 30.0 : 10.0);
          glVertex2d(radius * cos(AZ_DEG2RAD(i)),
                     radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_BOMB:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, 0);
        const double radius = 4.0;
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          if (blue) glColor3f(0, 0, 0.75); // blue
          else glColor3f(0.5, 0.5, 0.5); // gray
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_MEGA_BOMB:
      glBegin(GL_TRIANGLE_FAN); {
        const bool blink = proj->age < 2.0 ?
          ((int)ceil(4.0 * proj->age) % 2 == 1) :
          ((int)ceil(12.0 * proj->age) % 2 == 1);
        if (blink) glColor3f(1, 1, 0.5); // yellow
        else glColor3f(0.5, 0.5, 0.5); // gray
        glVertex2i(0, 0);
        const double radius = 6.0;
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          if (blue) glColor3f(0, 0.5, 0.75); // cyan
          else if (blink) glColor3f(0.75, 0.75, 0.25); // yellow
          else glColor3f(0.25, 0.25, 0.25); // dark gray
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_ORION_BOMB:
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.75, 0.75, 0.75); // light gray
        glVertex2i(0, 0);
        const double radius = 5.0;
        for (int i = 0, blue = 0; i <= 360; i += 60, blue = !blue) {
          if (blue) glColor3f(0, 0.5, 0.75); // blue-green
          else glColor3f(0.5, 0.5, 0.5); // gray
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_ORION_BLAST:
      glBegin(GL_QUAD_STRIP); {
        const double factor = proj->age / proj->data->lifetime;
        const double outer = proj->data->splash_radius * factor * factor;
        const double inner = fmax(0.0, outer - 100 * (1.0 - factor));
        for (int i = 0; i <= 360; i += 10) {
          glColor4f(1, 1, 1, 0.7);
          glVertex2d(outer * cos(AZ_DEG2RAD(i)),
                     0.7 * outer * sin(AZ_DEG2RAD(i)));
          glColor4f(0.5, 0.75, 1, 0.3);
          glVertex2d(inner * cos(AZ_DEG2RAD(i)),
                     0.7 * inner * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_FIREBALL_FAST:
    case AZ_PROJ_FIREBALL_SLOW:
      glBegin(GL_TRIANGLE_FAN); {
        const bool blink = az_clock_mod(2, 2, clock);
        if (blink) glColor3f(1, 0.75, 0.5); // orange
        else glColor3f(1, 0.25, 0.25); // red
        glVertex2i(0, 0);
        if (blink) glColor4f(0.5, 0.375, 0.25, 0); // orange
        else glColor4f(0.5, 0.125, 0.125, 0); // red
        const double radius = 6.0;
        for (int i = 0; i <= 360; i += 30) {
          glVertex2d(radius * cos(AZ_DEG2RAD(i)), radius * sin(AZ_DEG2RAD(i)));
        }
      } glEnd();
      break;
    case AZ_PROJ_FORCE_WAVE:
      glBegin(GL_QUADS); {
        const GLfloat factor = fmin(1.0, 2.0 * proj->age);
        glColor4f(0, 0.25, 0.5, 0.75);
        glVertex2f(0, -50 * factor);
        glVertex2f(0, 50 * factor);
        glColor4f(0, 0, 0.5, 0);
        glVertex2f(-150 * factor, 50 * factor);
        glVertex2f(-150 * factor, -50 * factor);
      } glEnd();
      break;
    case AZ_PROJ_GRENADE:
      glBegin(GL_QUADS); {
        glColor3f(0.4, 0.25, 0.25); glVertex2f(2, 3); glVertex2f(-2, 3);
        glVertex2f(-2, -3); glVertex2f(2, -3);
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.7, 0.7, 0.7); glVertex2f(2, 0); glColor3f(0.5, 0.5, 0.5);
        glVertex2f(2, 4); glVertex2f(4, 0); glVertex2f(2, -4);
      } glEnd();
      glBegin(GL_TRIANGLE_FAN); {
        glColor3f(0.7, 0.7, 0.7); glVertex2f(-2, 0); glColor3f(0.5, 0.5, 0.5);
        glVertex2f(-2, 4); glVertex2f(-4, 0); glVertex2f(-2, -4);
      } glEnd();
      break;
    case AZ_PROJ_GRAVITY_TORPEDO:
      draw_spark(proj->age, 8.0, (az_color_t){0, 128, 255, 0});
      break;
    case AZ_PROJ_GRAVITY_TORPEDO_WELL:
      {
        const double init_strength = 800.0;
        const double progress = proj->age / proj->data->lifetime;
        az_gravfield_t gravfield = {
          .kind = AZ_GRAV_SECTOR_PULL,
          .strength = init_strength * (1.0 - progress),
          .size.sector = { .thickness = 100.0 },
          .age = init_strength * proj->age * (1.0 - 0.5 * progress)
        };
        az_draw_gravfield_no_transform(&gravfield);
      }
      break;
    case AZ_PROJ_LASER_PULSE:
      glBegin(GL_QUADS); {
        glColor3f(1, 0.3, 0);
        glVertex2d(0, -1.5); glVertex2d(0, 1.5);
        glColor4f(1, 0.3, 0, 0);
        glVertex2d(-20, 1.5); glVertex2d(-20, -1.5);
      } glEnd();
      break;
    case AZ_PROJ_NUCLEAR_EXPLOSION: break; // invisible
    case AZ_PROJ_OTH_HOMING:
      draw_oth_projectile(proj, 4.0, clock);
      break;
    case AZ_PROJ_OTH_ROCKET:
      draw_oth_projectile(proj, 9.0, clock);
      break;
    case AZ_PROJ_OTH_SPRAY:
      draw_oth_projectile(proj, 5.0, clock);
      break;
    case AZ_PROJ_SPARK:
      draw_spark(proj->age, 8.0, (az_color_t){0, 255, 0, 0});
      break;
    case AZ_PROJ_SPINE:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0, 0.3, 0);
        glVertex2f(-3, 3);
        glColor3f(0.6, 0.7, 0.6);
        glVertex2f(5, 0);
        glColor3f(0.6, 0.7, 0);
        glVertex2f(-5, 0);
        glColor3f(0, 0.3, 0);
        glVertex2f(-3, -3);
      } glEnd();
      break;
    case AZ_PROJ_STINGER:
      glBegin(GL_TRIANGLE_STRIP); {
        glColor3f(0.3, 0.15, 0);
        glVertex2f(-3, 3);
        glColor3f(0.6, 0.7, 0.3);
        glVertex2f(5, 0);
        glColor3f(0.8, 0.6, 0);
        glVertex2f(-5, 0);
        glColor3f(0.3, 0.15, 0);
        glVertex2f(-3, -3);
      } glEnd();
      break;
    case AZ_PROJ_TRINE_TORPEDO:
    case AZ_PROJ_TRINE_TORPEDO_FIREBALL:
      draw_spark(proj->age, 10.0, (az_color_t){255, 128, 0, 0});
      break;
    case AZ_PROJ_TRINE_TORPEDO_EXPANDER:
      draw_spark(proj->age, 8.0, (az_color_t){255, 128, 0, 0});
      break;
  }
}

void az_draw_projectiles(const az_space_state_t* state) {
  AZ_ARRAY_LOOP(proj, state->projectiles) {
    if (proj->kind == AZ_PROJ_NOTHING) continue;
    glPushMatrix(); {
      glTranslated(proj->position.x, proj->position.y, 0);
      glRotated(AZ_RAD2DEG(proj->angle), 0, 0, 1);
      draw_projectile(proj, state->clock);
    } glPopMatrix();
  }
}

/*===========================================================================*/
