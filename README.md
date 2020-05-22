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
$ sudo apt-get install libgl-mesa-dev libsdl1.2-dev
```

### Building and running

To build and run a debug build of the game, run:

```shell
$ make       # Compiles everything.
$ make test  # Runs unit tests, to make sure things look okay on your system.
$ make run   # Starts the game.
```

To build and install a packaged app on Mac OS X, run:

```shell
$ BUILDTYPE=release make macosx_app
$ mv -i out/release/host/Azimuth.app /Applications/
$ open /Applications/Azimuth.app
```

To build and install a packaged app on Debian, run:

```shell
$ BUILDTYPE=release make linux_deb
$ sudo dpkg -i out/release/host/azimuth_*.deb
$ azimuth
```

## Troubleshooting

### SDL_SetVideoMode failed: Couldn't find matching GLX visual

This error seems to occur on some older Linux machines.  One thing you can try
is to edit `src/azimuth/gui/screen.c` and comment out the following lines
before recompiling:

```c
SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
```

This will disable antialiasing, so the graphics might not be 100% correct, but
the game should still be perfectly playable.

## License

This codebase is licensed under the GNU GPL, version 3.  This codebase is free
software: you can redistribute it and/or modify it under the terms of the GNU
General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.

Azimuth is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

The complete license can be found in the LICENSE file.

### Music

Additionally, the music for Azimuth (found in the `data/music` directory) may
optionally be used under a [Creative Commons Attribution
4.0](https://creativecommons.org/licenses/by/4.0/) (CC-BY 4.0) license.  When
attributing, I'd appreciate it if you would link back to
https://mdsteele.games/azimuth/ or to this repository.
