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

#include "azimuth/view/cursor.h"

#include <GL/gl.h>

#include "azimuth/gui/event.h" // for az_get_mouse_position

/*===========================================================================*/

void az_draw_cursor(void) {
  int x = 0, y = 0;
  if (!az_get_mouse_position(&x, &y)) return;
  glPushMatrix(); {
    glTranslated(0.5, 0.5, 0);
    glColor4f(0, 1, 0, 0.5); // green tint
    glBegin(GL_QUAD_STRIP); {
      glVertex2i(x, y);
      glVertex2i(x + 2, y + 5);
      glVertex2i(x + 20, y + 20);
      glVertex2i(x + 11, y + 14);
      glVertex2i(x + 6, y + 14);
      glVertex2i(x + 5, y + 11);
      glVertex2i(x, y + 20);
      glVertex2i(x + 2, y + 14);
      glVertex2i(x, y);
      glVertex2i(x + 2, y + 5);
    } glEnd();
    glColor3f(0, 1, 0); // green
    glBegin(GL_LINE_LOOP); {
      glVertex2i(x, y);
      glVertex2i(x + 20, y + 20);
      glVertex2i(x + 6, y + 14);
      glVertex2i(x, y + 20);
    } glEnd();
    glBegin(GL_LINE_LOOP); {
      glVertex2i(x + 2, y + 5);
      glVertex2i(x + 11, y + 14);
      glVertex2i(x + 5, y + 11);
      glVertex2i(x + 2, y + 14);
    } glEnd();
  } glPopMatrix();
}

/*===========================================================================*/
