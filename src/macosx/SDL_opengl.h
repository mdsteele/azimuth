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
#ifndef MACOSX_SDL_OPENGL_H_
#define MACOSX_SDL_OPENGL_H_

// On other systems, the SDL2 OpenGL headers can be found at "SDL_opengl.h",
// but on Mac OS X they're at <SDL2/SDL_opengl.h>.  So we use #include
// "SDL_opengl.h" in our source files, and then for building on Mac we arrange
// for that to resolve to this file.

#ifndef __APPLE__
#error This file is only intended for building on Mac OS X.
#endif

#include <SDL2/SDL_opengl.h>

#endif // MACOSX_SDL_OPENGL_H_
