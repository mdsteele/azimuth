#=============================================================================#
# Copyright 2012 Matthew D. Steele <mdsteele@alum.mit.edu>                    #
#                                                                             #
# This file is part of Azimuth.                                               #
#                                                                             #
# Azimuth is free software: you can redistribute it and/or modify it under    #
# the terms of the GNU General Public License as published by the Free        #
# Software Foundation, either version 3 of the License, or (at your option)   #
# any later version.                                                          #
#                                                                             #
# Azimuth is distributed in the hope that it will be useful, but WITHOUT      #
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       #
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   #
# more details.                                                               #
#                                                                             #
# You should have received a copy of the GNU General Public License along     #
# with Azimuth.  If not, see <http://www.gnu.org/licenses/>.                  #
#=============================================================================#

BUILDTYPE ?= debug

SRCDIR = src
DATADIR = data
OUTDIR = out/$(BUILDTYPE)
OBJDIR = $(OUTDIR)/obj
BINDIR = $(OUTDIR)/bin

#=============================================================================#
# Determine our build environment.

ALL_TARGETS = $(BINDIR)/azimuth $(BINDIR)/editor $(BINDIR)/unit_tests \
              $(BINDIR)/muse $(BINDIR)/zfxr

CFLAGS = -I$(SRCDIR) -Wall -Werror -Wempty-body -Winline \
         -Wmissing-field-initializers -Wold-style-definition -Wshadow \
         -Wsign-compare -Wstrict-prototypes -Wundef -Wno-unused-local-typedef

ifeq "$(BUILDTYPE)" "debug"
  CFLAGS += -O1 -g
else ifeq "$(BUILDTYPE)" "release"
  # For release builds, disable asserts, but don't warn about e.g. static
  # functions or local variables that are only used for asserts, and which
  # therefore become unused when asserts are disabled.
  CFLAGS += -O2 -DNDEBUG -Wno-unused-function -Wno-unused-variable
else
  $(error BUILDTYPE must be 'debug' or 'release')
endif

# Use clang if it's available, otherwise use gcc.
CC := $(shell which clang > /dev/null && echo clang || echo gcc)
ifeq "$(CC)" "clang"
  CFLAGS += -Winitializer-overrides -Wno-objc-protocol-method-implementation
else
  CFLAGS += -Woverride-init -Wno-unused-local-typedefs
endif

OS_NAME := $(shell uname)
ifeq "$(OS_NAME)" "Darwin"
  CFLAGS += -I$(SRCDIR)/macosx -mmacosx-version-min=10.6
  # Use the SDL framework if it's installed.  Otherwise, look to see if SDL has
  # been installed via MacPorts.  Otherwise, give up.
  ifeq "$(shell test -d /Library/Frameworks/SDL.framework && echo ok)" "ok"
    SDL_FRAMEWORK_PATH = /Library/Frameworks/SDL.framework
  else ifeq "$(shell test -d ~/Library/Frameworks/SDL.framework \
	             && echo ok)" "ok"
    SDL_FRAMEWORK_PATH = ~/Library/Frameworks/SDL.framework
  else ifeq "$(shell test -d /Network/Library/Frameworks/SDL.framework \
	             && echo ok)" "ok"
    SDL_FRAMEWORK_PATH = /Network/Library/Frameworks/SDL.framework
  endif
  ifdef SDL_FRAMEWORK_PATH
    SDL_LIBFLAGS = -framework SDL -rpath @executable_path/../Frameworks
  else ifeq "$(shell test -f /opt/local/lib/libSDL.a && echo ok)" "ok"
    SDL_LIBFLAGS = -I/opt/local/include -L/opt/local/lib -lSDL
  else
    $(error SDL does not seem to be installed)
  endif
  MAIN_LIBFLAGS = -framework Cocoa $(SDL_LIBFLAGS) -framework OpenGL
  TEST_LIBFLAGS =
  MUSE_LIBFLAGS = -framework Cocoa $(SDL_LIBFLAGS)
  SYSTEM_OBJFILES = $(OBJDIR)/macosx/SDLMain.o \
                    $(OBJDIR)/azimuth/system/resource_mac.o
  ALL_TARGETS += macosx_app
else
  MAIN_LIBFLAGS = -lm -lSDL -lGL
  TEST_LIBFLAGS = -lm
  MUSE_LIBFLAGS = -lm -lSDL
  SYSTEM_OBJFILES = $(OBJDIR)/azimuth/system/resource_linux.o
  ALL_TARGETS += linux_app
endif

C99FLAGS = -std=c99 -pedantic $(CFLAGS)

define compile-sys
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ -c $< $(CFLAGS)
endef
define compile-c99
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ -c $< $(C99FLAGS)
endef
define copy-file
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@
endef

#=============================================================================#
# Find all of the source files:

AZ_CONTROL_HEADERS := $(shell find $(SRCDIR)/azimuth/control -name '*.h')
AZ_GUI_HEADERS := $(shell find $(SRCDIR)/azimuth/gui -name '*.h')
AZ_STATE_HEADERS := $(shell find $(SRCDIR)/azimuth/state -name '*.h')
AZ_SYSTEM_HEADERS := $(shell find $(SRCDIR)/azimuth/system -name '*.h')
AZ_TICK_HEADERS := $(shell find $(SRCDIR)/azimuth/tick -name '*.h')
AZ_UTIL_HEADERS := $(shell find $(SRCDIR)/azimuth/util -name '*.h') \
                   $(SRCDIR)/azimuth/constants.h
AZ_VIEW_HEADERS := $(shell find $(SRCDIR)/azimuth/view -name '*.h')
AZ_EDITOR_HEADERS := $(shell find $(SRCDIR)/editor -name '*.h')
AZ_TEST_HEADERS := $(shell find $(SRCDIR)/test -name '*.h')
AZ_MUSE_HEADERS := $(shell find $(SRCDIR)/muse -name '*.h')
AZ_ZFXR_HEADERS := $(shell find $(SRCDIR)/zfxr -name '*.h')

AZ_CONTROL_C99FILES := $(shell find $(SRCDIR)/azimuth/control -name '*.c')
AZ_GUI_C99FILES := $(shell find $(SRCDIR)/azimuth/gui -name '*.c')
AZ_STATE_C99FILES := $(shell find $(SRCDIR)/azimuth/state -name '*.c')
AZ_TICK_C99FILES := $(shell find $(SRCDIR)/azimuth/tick -name '*.c')
AZ_UTIL_C99FILES := $(shell find $(SRCDIR)/azimuth/util -name '*.c')
AZ_VIEW_C99FILES := $(shell find $(SRCDIR)/azimuth/view -name '*.c')

MAIN_C99FILES := $(AZ_UTIL_C99FILES) $(AZ_STATE_C99FILES) $(AZ_TICK_C99FILES) \
                 $(AZ_GUI_C99FILES) $(AZ_VIEW_C99FILES) \
                 $(AZ_CONTROL_C99FILES) $(SRCDIR)/azimuth/main.c
EDIT_C99FILES := $(shell find $(SRCDIR)/editor -name '*.c') \
                 $(AZ_UTIL_C99FILES) $(AZ_STATE_C99FILES) $(AZ_GUI_C99FILES) \
                 $(AZ_VIEW_C99FILES)
TEST_C99FILES := $(shell find $(SRCDIR)/test -name '*.c') \
                 $(AZ_UTIL_C99FILES) $(AZ_STATE_C99FILES)
MUSE_C99FILES := $(shell find $(SRCDIR)/muse -name '*.c') \
                 $(AZ_UTIL_C99FILES) $(AZ_STATE_C99FILES)
ZFXR_C99FILES := $(shell find $(SRCDIR)/zfxr -name '*.c') \
                 $(AZ_UTIL_C99FILES) $(AZ_STATE_C99FILES) $(AZ_GUI_C99FILES) \
                 $(AZ_VIEW_C99FILES)

MAIN_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_C99FILES)) \
                 $(SYSTEM_OBJFILES)
EDIT_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(EDIT_C99FILES)) \
                 $(SYSTEM_OBJFILES)
TEST_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(TEST_C99FILES))
MUSE_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MUSE_C99FILES)) \
                 $(SYSTEM_OBJFILES)
ZFXR_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(ZFXR_C99FILES)) \
                 $(SYSTEM_OBJFILES)

RESOURCE_FILES := $(shell find $(DATADIR)/music -name '*.txt') \
                  $(shell find $(DATADIR)/rooms -name '*.txt')

#=============================================================================#
# Default build target:

.PHONY: all
all: $(ALL_TARGETS)

#=============================================================================#
# Build rules for linking the executables:

$(BINDIR)/azimuth: $(MAIN_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(CFLAGS) $(MAIN_LIBFLAGS)

$(BINDIR)/editor: $(EDIT_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(CFLAGS) $(MAIN_LIBFLAGS)

$(BINDIR)/unit_tests: $(TEST_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(CFLAGS) $(TEST_LIBFLAGS)

$(BINDIR)/muse: $(MUSE_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(CFLAGS) $(MUSE_LIBFLAGS)

$(BINDIR)/zfxr: $(ZFXR_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@$(CC) -o $@ $^ $(CFLAGS) $(MAIN_LIBFLAGS)

#=============================================================================#
# Build rules for compiling system-specific code:

$(OBJDIR)/macosx/SDLMain.o: $(SRCDIR)/macosx/SDLMain.m \
                            $(SRCDIR)/macosx/SDLMain.h
	$(compile-sys)

$(OBJDIR)/azimuth/system/resource_mac.o: \
    $(SRCDIR)/azimuth/system/resource_mac.m $(AZ_SYSTEM_HEADERS)
	$(compile-sys)

$(OBJDIR)/azimuth/system/%.o: $(SRCDIR)/azimuth/system/%.c $(AZ_SYSTEM_HEADERS)
	$(compile-sys)

#=============================================================================#
# Build rules for compiling non-system-specific code:

$(OBJDIR)/azimuth/util/%.o: $(SRCDIR)/azimuth/util/%.c $(AZ_UTIL_HEADERS)
	$(compile-c99)

$(OBJDIR)/azimuth/state/%.o: $(SRCDIR)/azimuth/state/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_STATE_HEADERS)
	$(compile-c99)

$(OBJDIR)/azimuth/tick/%.o: $(SRCDIR)/azimuth/tick/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_STATE_HEADERS) $(AZ_TICK_HEADERS)
	$(compile-c99)

$(OBJDIR)/azimuth/gui/%.o: $(SRCDIR)/azimuth/gui/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_SYSTEM_HEADERS) $(AZ_GUI_HEADERS)
	$(compile-c99)

$(OBJDIR)/azimuth/view/%.o: $(SRCDIR)/azimuth/view/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_STATE_HEADERS) $(AZ_GUI_HEADERS) \
    $(AZ_VIEW_HEADERS) $(SRCDIR)/azimuth/version.h
	$(compile-c99)

$(OBJDIR)/azimuth/control/%.o: $(SRCDIR)/azimuth/control/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_SYSTEM_HEADERS) $(AZ_STATE_HEADERS) \
    $(AZ_TICK_HEADERS) $(AZ_GUI_HEADERS) $(AZ_VIEW_HEADERS) \
    $(AZ_CONTROL_HEADERS)
	$(compile-c99)

$(OBJDIR)/azimuth/main.o: $(SRCDIR)/azimuth/main.c \
    $(AZ_UTIL_HEADERS) $(AZ_SYSTEM_HEADERS) $(AZ_STATE_HEADERS) \
    $(AZ_TICK_HEADERS) $(AZ_GUI_HEADERS) $(AZ_VIEW_HEADERS) \
    $(AZ_CONTROL_HEADERS)
	$(compile-c99)

$(OBJDIR)/editor/%.o: $(SRCDIR)/editor/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_SYSTEM_HEADERS) $(AZ_STATE_HEADERS) \
    $(AZ_GUI_HEADERS) $(AZ_VIEW_HEADERS) $(AZ_EDITOR_HEADERS)
	$(compile-c99)

$(OBJDIR)/test/%.o: $(SRCDIR)/test/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_STATE_HEADERS) $(AZ_TEST_HEADERS)
	$(compile-c99)

$(OBJDIR)/muse/%.o: $(SRCDIR)/muse/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_STATE_HEADERS) $(AZ_MUSE_HEADERS)
	$(compile-c99)

$(OBJDIR)/zfxr/%.o: $(SRCDIR)/zfxr/%.c \
    $(AZ_UTIL_HEADERS) $(AZ_SYSTEM_HEADERS) $(AZ_STATE_HEADERS) \
    $(AZ_GUI_HEADERS) $(AZ_VIEW_HEADERS) $(AZ_ZFXR_HEADERS)
	$(compile-c99)

#=============================================================================#
# Build rules for bundling Mac OS X application:

MACOSX_APPDIR = $(OUTDIR)/Azimuth.app/Contents
MACOSX_APP_FILES := $(MACOSX_APPDIR)/Info.plist \
    $(MACOSX_APPDIR)/MacOS/azimuth \
    $(MACOSX_APPDIR)/Resources/application.icns \
    $(patsubst $(DATADIR)/%,$(MACOSX_APPDIR)/Resources/%,$(RESOURCE_FILES))

ifdef SDL_FRAMEWORK_PATH
MACOSX_APP_FILES += $(MACOSX_APPDIR)/Frameworks/SDL.framework
$(MACOSX_APPDIR)/Frameworks/SDL.framework: $(SDL_FRAMEWORK_PATH)
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp -R $< $@
endif

.PHONY: macosx_app
macosx_app: $(MACOSX_APP_FILES)

$(MACOSX_APPDIR)/Info.plist: $(DATADIR)/Info.plist $(SRCDIR)/azimuth/version.h
	@echo "Generating $@"
	@mkdir -p $(@D)
	@sed "s/%AZ_VERSION_NUMBER/$(shell \
	      sed -n 's/^\#define AZ_VERSION_[A-Z]* \([0-9]\{1,\}\)$$/\1/p' \
	          $(SRCDIR)/azimuth/version.h | \
	      paste -s -d. -)/g" < \
	    $(DATADIR)/Info.plist > $@

$(MACOSX_APPDIR)/MacOS/azimuth: $(BINDIR)/azimuth
	$(copy-file)

$(MACOSX_APPDIR)/Resources/application.icns: $(DATADIR)/application.icns
	$(copy-file)

$(MACOSX_APPDIR)/Resources/music/%: $(DATADIR)/music/%
	$(copy-file)

$(MACOSX_APPDIR)/Resources/rooms/%: $(DATADIR)/rooms/%
	$(copy-file)

#=============================================================================#
# Build rules for bundling Linux application:

LINUX_APPDIR = $(OUTDIR)/Azimuth
LINUX_APP_FILES := $(LINUX_APPDIR)/Azimuth \
    $(patsubst $(DATADIR)/%,$(LINUX_APPDIR)/%,$(RESOURCE_FILES))

.PHONY: linux_app
linux_app: $(LINUX_APP_FILES)

$(LINUX_APPDIR)/Azimuth: $(BINDIR)/azimuth
	$(copy-file)

$(LINUX_APPDIR)/music/%: $(DATADIR)/music/%
	$(copy-file)

$(LINUX_APPDIR)/rooms/%: $(DATADIR)/rooms/%
	$(copy-file)

#=============================================================================#
# Convenience build targets:

.PHONY: run
ifeq "$(OS_NAME)" "Darwin"
run: macosx_app
	$(MACOSX_APPDIR)/MacOS/azimuth
else
run: linux_app
	$(LINUX_APPDIR)/Azimuth
endif

.PHONY: edit
edit: $(BINDIR)/editor
	$(BINDIR)/editor

.PHONY: test
test: $(BINDIR)/unit_tests
	$(BINDIR)/unit_tests

.PHONY: zfxr
zfxr: $(BINDIR)/zfxr
	$(BINDIR)/zfxr

.PHONY: clean
clean:
	rm -rf $(OUTDIR)

.PHONY: tidy
tidy:
	find $(SRCDIR) -name '*~' -print0 | xargs -0 rm

.PHONY: wc
wc:
	find $(SRCDIR) \( -name '*.c' -or -name '*.h' \) -print0 | \
	    xargs -0 wc -l

.PHONY: todo
todo:
	ack "FIXME|TODO" $(SRCDIR)

#=============================================================================#
