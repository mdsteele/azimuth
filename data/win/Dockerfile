FROM debian:stable-slim

# Install MXE prereqs (see https://mxe.cc/#requirements-debian)
RUN apt-get update && apt-get install -y \
  autoconf \
  automake \
  autopoint \
  bash \
  bison \
  bzip2 \
  flex \
  g++ \
  gettext \
  git \
  gperf \
  intltool \
  libgdk-pixbuf2.0-dev \
  libltdl-dev \
  libgl-dev \
  libpcre3-dev \
  libssl-dev \
  libtool-bin \
  libxml-parser-perl \
  lzip \
  make \
  openssl \
  p7zip-full \
  patch \
  perl \
  python3 \
  python3-distutils \
  python3-mako \
  python3-pkg-resources \
  python-is-python3 \
  ruby \
  sed \
  unzip \
  wget \
  xz-utils
# Build MXE (see https://mxe.cc/#tutorial)
RUN git clone https://github.com/mxe/mxe.git /opt/mxe && \
  cd /opt/mxe && \
  make cc sdl2 MXE_TARGETS='i686-w64-mingw32.static'
ENV PATH="/opt/mxe/usr/bin:${PATH}"

WORKDIR /azimuth
COPY Makefile ./Makefile
COPY src ./src
COPY data ./data
RUN BUILDTYPE=release TARGET=windows make clean
RUN BUILDTYPE=release TARGET=windows make windows_zip
