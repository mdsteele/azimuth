# Release Process

## Debian

### Prereqs

Install dependencies:

```shell
$ sudo apt-get install libgl-mesa-dev libsdl2-dev
```

### Building

From the repository root, run:

```shell
# Make sure the tests pass:
make test

# Build the Debian package:
BUILDTYPE=release make clean
BUILDTYPE=release make linux_zip
ls out/release/host/Azimuth-*-Linux.tar.bz2
```

## Mac OS X

### Prereqs

Install the [SDL2](https://www.libsdl.org/download-2.0.php) framework.

### Building

From the repository root, run:

```shell
# Make sure the tests pass:
make test

# Build and sign the Mac OS X app:
BUILDTYPE=release make clean
BUILDTYPE=release IDENTITY=TODO make signed_macosx_zip
ls out/release/host/signed/Azimuth-*-Mac.zip
```

## Windows

Azimuth can be cross-compiled for Windows using [MXE](https://mxe.cc/).

### Prereqs

Follow the [MXE tutorial](https://mxe.cc/#tutorial) to set up the environment.

Install SDL2 for MXE:

```shell
cd path/to/mxe/repo && make sdl2  # If building MXE from source.
# ---OR---
sudo apt-get install mxe-i686-w64-mingw32.static-sdl2  # If using apt.
```

### Building

From the repository root, run:

```shell
# Build the Windows executable:
BUILDTYPE=release TARGET=windows make clean
BUILDTYPE=release TARGET=windows make windows_zip
ls out/release/windows/Azimuth-*-Windows.zip
```
