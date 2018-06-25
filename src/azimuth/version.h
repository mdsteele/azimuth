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
#ifndef AZIMUTH_VERSION_H_
#define AZIMUTH_VERSION_H_

/*===========================================================================*/

#define AZ_VERSION_MAJOR 1
#define AZ_VERSION_MINOR 0
#define AZ_VERSION_PATCH 2

/*===========================================================================*/

#ifndef NDEBUG
#define AZ_VERSION_SUFFIX "/debug"
#else
#define AZ_VERSION_SUFFIX ""
#endif

#define AZ_MAKE_VERSION_STRING_(major, minor, patch, suffix)    \
  AZ_MAKE_VERSION_STRING2_(major, minor, patch, suffix)
#define AZ_MAKE_VERSION_STRING2_(major, minor, patch, suffix) \
  "v" #major "." #minor "." #patch suffix

#define AZ_VERSION_STRING \
  AZ_MAKE_VERSION_STRING_(AZ_VERSION_MAJOR, AZ_VERSION_MINOR, \
                          AZ_VERSION_PATCH, AZ_VERSION_SUFFIX)

/*===========================================================================*/

#endif // AZIMUTH_VERSION_H_
