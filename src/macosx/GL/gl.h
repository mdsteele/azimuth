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

#pragma once
#ifndef MACOSX_GL_GL_H_
#define MACOSX_GL_GL_H_

// On other systems, the OpenGL headers are at <GL/gl.h>, but on Mac OS X
// they're at <OpenGL/gl.h>.  So we use #include <GL/gl.h> in our source files,
// and then for building on Mac we arrange for that to resolve to this file.

#ifndef __APPLE__
#error This file is only intended for building on Mac OS X.
#endif

#include <OpenGL/gl.h>

#endif // MACOSX_GL_GL_H_
