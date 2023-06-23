# Release Process

## Debian

### Building Debian package using Docker

```shell
# Build the image.
docker build -t azimuth-debian-build -f ./data/deb/Dockerfile .

# Extract the built Debian package.
VERSION_NUMBER=$(sed -n 's/^\#define AZ_VERSION_[A-Z]* \([0-9]\{1,\}\)$/\1/p' src/azimuth/version.h | paste -s -d. -)
CONTAINER_ID=$(docker create azimuth-debian-build)
docker cp ${CONTAINER_ID}:/azimuth/out/release/host/azimuth_${VERSION_NUMBER}_i386.deb .
docker rm -v ${CONTAINER_ID}
```

### Building compressed binary on host

Install dependencies:

```shell
$ sudo apt-get install libgl-mesa-dev libsdl2-dev
```

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
BUILDTYPE=release IDENTITY='Developer ID Application' make signed_macosx_dmg
ls out/release/host/signed/Azimuth-*-Mac.dmg
```

### Notarizing

See
[these instructions](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow)
for details.

If you haven't already,
[create an app-specific password](https://support.apple.com/en-us/HT204397) for
the notary tool, then save it to the keychain with:

```shell
xcrun notarytool store-credentials "AC_PASSWORD" \
    --apple-id "<your Apple ID email>" \
    --team-id <the Team ID from the Apple Developer membership details page> \
    --password <the app-specific password you generated above>
```

With that password on the keychain, run:

```shell
xcrun notarytool submit out/release/host/signed/Azimuth-*-Mac.dmg \
    --keychain-profile "AC_PASSWORD" \
    --wait
```

That command will print a submission UUID (in the form
`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`).  If there are any problems, you can
download the error log with:

```shell
xcrun notarytool log <the submission UUID> \
    --keychain-profile "AC_PASSWORD" \
    developer_log.json
```

If notarization succeeds, staple the ticket to the app with:

```shell
xcrun stapler staple out/release/host/signed/Azimuth-*-Mac.dmg
xcrun stapler validate out/release/host/signed/Azimuth-*-Mac.dmg
```

## Windows

Azimuth can be cross-compiled for Windows using [MXE](https://mxe.cc/).

### Building using Docker

```shell
# Build the image.
docker build -t azimuth-windows-build -f ./data/win/Dockerfile .

# Extract the built Debian package.
VERSION_NUMBER=$(sed -n 's/^\#define AZ_VERSION_[A-Z]* \([0-9]\{1,\}\)$/\1/p' src/azimuth/version.h | paste -s -d. -)
CONTAINER_ID=$(docker create azimuth-windows-build)
docker cp ${CONTAINER_ID}:/azimuth/out/release/windows/Azimuth-v${VERSION_NUMBER}-Windows.zip .
docker rm -v ${CONTAINER_ID}
```

### Building on host machine

Follow the [MXE tutorial](https://mxe.cc/#tutorial) to set up the environment.

Install SDL2 for MXE:

```shell
cd path/to/mxe/repo && make sdl2  # If building MXE from source.
# ---OR---
sudo apt-get install mxe-i686-w64-mingw32.static-sdl2  # If using apt.
```

From the repository root, run:

```shell
# Build the Windows executable:
BUILDTYPE=release TARGET=windows make clean
BUILDTYPE=release TARGET=windows make windows_zip
ls out/release/windows/Azimuth-*-Windows.zip
```
