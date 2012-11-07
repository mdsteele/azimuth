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

SRCDIR = src
DATADIR = data
OUTDIR = out
OBJDIR = $(OUTDIR)/obj
BINDIR = $(OUTDIR)/bin

CFLAGS = -Wall -Werror -O1 -I$(SRCDIR)
C99FLAGS = -std=c99 -pedantic $(CFLAGS)

define compile-sys
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(CFLAGS)
endef
define compile-c99
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(C99FLAGS)
endef

#=============================================================================#
# Determine what OS we're on and what targets we're building.

ALL_TARGETS := $(BINDIR)/azimuth $(BINDIR)/editor $(BINDIR)/unit_tests

OS_NAME := $(shell uname)
ifeq "$(OS_NAME)" "Darwin"
  CFLAGS += -I$(SRCDIR)/macosx
  MAIN_LIBFLAGS = -framework Cocoa -framework OpenGL -framework SDL
  TEST_LIBFLAGS =
  SYSTEM_OBJFILES = $(OBJDIR)/macosx/SDLMain.o \
                    $(OBJDIR)/azimuth/system/misc_posix.o \
                    $(OBJDIR)/azimuth/system/resource_mac.o
  ALL_TARGETS += macosx_app
else
  MAIN_LIBFLAGS = -lGL -lSDL
  TEST_LIBFLAGS = -lm
  SYSTEM_OBJFILES = $(OBJDIR)/azimuth/system/misc_posix.o \
                    $(OBJDIR)/azimuth/system/resource_linux.o
  ALL_TARGETS += linux_app
endif

#=============================================================================#
# Find all of the source files:

AZ_CONTROL_HEADERS := $(shell find $(SRCDIR)/azimuth/control -name '*.h')
AZ_GUI_HEADERS := $(shell find $(SRCDIR)/azimuth/gui -name '*.h')
AZ_STATE_HEADERS := $(shell find $(SRCDIR)/azimuth/state -name '*.h')
AZ_SYSTEM_HEADERS := $(shell find $(SRCDIR)/azimuth/system -name '*.h')
AZ_TICK_HEADERS := $(shell find $(SRCDIR)/azimuth/tick -name '*.h')
AZ_VIEW_HEADERS := $(shell find $(SRCDIR)/azimuth/view -name '*.h')
AZ_EDITOR_HEADERS := $(shell find $(SRCDIR)/editor -name '*.h')
AZ_TEST_HEADERS := $(shell find $(SRCDIR)/test -name '*.h')

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

MAIN_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_C99FILES)) \
                 $(SYSTEM_OBJFILES)
EDIT_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(EDIT_C99FILES)) \
                 $(SYSTEM_OBJFILES)
TEST_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(TEST_C99FILES))

RESOURCE_FILES := $(shell find $(DATADIR)/rooms -name '*.txt') \

#=============================================================================#
# Default build target:

.PHONY: all
all: $(ALL_TARGETS)

#=============================================================================#
# Build rules for linking the executables:

$(BINDIR)/azimuth: $(MAIN_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@gcc -o $@ $^ $(CFLAGS) $(MAIN_LIBFLAGS)

$(BINDIR)/editor: $(EDIT_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@gcc -o $@ $^ $(CFLAGS) $(MAIN_LIBFLAGS)

$(BINDIR)/unit_tests: $(TEST_OBJFILES)
	@echo "Linking $@"
	@mkdir -p $(@D)
	@gcc -o $@ $^ $(CFLAGS) $(TEST_LIBFLAGS)

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
    $(AZ_VIEW_HEADERS)
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

#=============================================================================#
# Build rules for bundling Mac OS X application:

MACOSX_APPDIR = $(OUTDIR)/Azimuth.app/Contents
MACOSX_APP_FILES := $(MACOSX_APPDIR)/Info.plist \
    $(MACOSX_APPDIR)/MacOS/azimuth \
    $(MACOSX_APPDIR)/Resources/application.icns \
    $(patsubst $(DATADIR)/%,$(MACOSX_APPDIR)/Resources/%,$(RESOURCE_FILES))

.PHONY: macosx_app
macosx_app: $(MACOSX_APP_FILES)

$(MACOSX_APPDIR)/Info.plist: $(DATADIR)/Info.plist
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(MACOSX_APPDIR)/MacOS/azimuth: $(BINDIR)/azimuth
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(MACOSX_APPDIR)/Resources/application.icns: $(DATADIR)/application.icns
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(MACOSX_APPDIR)/Resources/rooms/%: $(DATADIR)/rooms/%
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

#=============================================================================#
# Build rules for bundling Linux application:

LINUX_APPDIR = $(OUTDIR)/Azimuth
LINUX_APP_FILES := $(LINUX_APPDIR)/Azimuth \
    $(patsubst $(DATADIR)/%,$(LINUX_APPDIR)/%,$(RESOURCE_FILES))

.PHONY: linux_app
linux_app: $(LINUX_APP_FILES)

$(LINUX_APPDIR)/Azimuth: $(BINDIR)/azimuth
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(LINUX_APPDIR)/rooms/%: $(DATADIR)/rooms/%
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

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
