# Azimuth

Azimuth is a metroidvania game, with vector graphics.  You can build it from
source here, or download binaries at https://mdsteele.games/azimuth/download

Azimuth is inspired by such games as the Metroid series (particularly Super
Metroid and Metroid Fusion), SketchFighter 4000 Alpha, and Star Control II
(a.k.a. The Ur-Quan Masters) -- all fine games that are well worth your time.

## Build instructions

### Prerequisites

Azimuth depends on OpenGL and on [SDL
1.2](https://www.libsdl.org/download-1.2.php).  On Mac, you can install SDL 1.2
either in Framework form or via e.g. MacPorts, and the Makefile should be able
to sort it out.  On Debian, all it should take is:

```shell
$ sudo apt-get install libsdl1.2-dev
```

### Building and running

From the root of the repository, run:

```shell
$ make       # Compiles everything.
$ make test  # Runs unit tests, to make sure things look okay on your system.
$ make run   # Starts the game.
```

The final packaged game will be output to `out/debug/Azimuth.app` on Mac, or to
`out/debug/Azimuth` on Linux.  (If you try to run the executable found at
`out/debug/bin/azimuth`, it won't be able find the game resource files.)

You can also build a release version with:

```shell
$ BUILDTYPE=release make
$ BUILDTYPE=release make test
$ BUILDTYPE=release make run
```

in which case the outputs will be found in `out/release/` instead.

## License

This codebase is licensed under the GNU GPL, version 3.  This codebase is free
software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.

Azimuth is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

The complete license can be found in the LICENSE file.
