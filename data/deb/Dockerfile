FROM debian:stable-slim
RUN apt-get update && apt-get install -y \
  clang \
  libgl1-mesa-dev \
  libsdl2-dev \
  make
WORKDIR /azimuth
COPY Makefile ./Makefile
COPY src ./src
COPY data ./data
RUN make test
RUN BUILDTYPE=release make clean
RUN BUILDTYPE=release make linux_deb
