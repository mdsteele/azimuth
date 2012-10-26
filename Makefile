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

CFLAGS = -Wall -Werror -O1
C99FLAGS = -std=c99 -pedantic $(CFLAGS) -I$(SRCDIR)

HEADERS := $(shell find $(SRCDIR) -name '*.h')
MAIN_C99FILES := $(shell find $(SRCDIR)/azimuth -name '*.c')
EDIT_C99FILES := $(shell find $(SRCDIR)/editor -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/gui -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/state -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/system -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/util -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/view -name '*.c')
TEST_C99FILES := $(shell find $(SRCDIR)/test -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/state -name '*.c') \
	         $(shell find $(SRCDIR)/azimuth/util -name '*.c')
MAIN_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MAIN_C99FILES))
EDIT_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(EDIT_C99FILES))
TEST_OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(TEST_C99FILES))
RESOURCE_FILES := $(shell find $(DATADIR)/rooms -name '*.txt') \

ALL_TARGETS := $(BINDIR)/azimuth $(BINDIR)/editor $(BINDIR)/unit_tests

OS_NAME := $(shell uname)
ifeq "$(OS_NAME)" "Darwin"
  CFLAGS += -I$(SRCDIR)/macosx
  MAIN_OBJFILES += $(OBJDIR)/macosx/SDLMain.o
  EDIT_OBJFILES += $(OBJDIR)/macosx/SDLMain.o
  MAIN_LIBFLAGS := -framework Cocoa -framework OpenGL -framework SDL
  TEST_LIBFLAGS :=
  ALL_TARGETS += app
else
  MAIN_LIBFLAGS := -lGL -lSDL
  TEST_LIBFLAGS := -lm
endif

#=============================================================================#
# Default build target:

.PHONY: all
all: $(ALL_TARGETS)

#=============================================================================#
# Build rules for compiling code:

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

$(OBJDIR)/macosx/SDLMain.o: $(SRCDIR)/macosx/SDLMain.m \
                            $(SRCDIR)/macosx/SDLMain.h
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(C99FLAGS)

#=============================================================================#
# Build rules for bundling Mac OS X application:

APPDIR = $(OUTDIR)/Azimuth.app/Contents
APP_FILES := $(APPDIR)/Info.plist $(APPDIR)/MacOS/azimuth \
             $(APPDIR)/Resources/application.icns \
             $(patsubst $(DATADIR)/%,$(APPDIR)/Resources/%,$(RESOURCE_FILES))

.PHONY: app
app: $(APP_FILES)

$(APPDIR)/Info.plist: $(DATADIR)/Info.plist
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(APPDIR)/MacOS/azimuth: $(BINDIR)/azimuth
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(APPDIR)/Resources/application.icns: $(DATADIR)/application.icns
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

$(APPDIR)/Resources/rooms/%: $(DATADIR)/rooms/%
	@echo "Copying $<"
	@mkdir -p $(@D)
	@cp $< $@

#=============================================================================#
# Convenience build targets:

.PHONY: run
ifeq "$(OS_NAME)" "Darwin"
run: app
	$(APPDIR)/MacOS/azimuth
else
run: $(BINDIR)/azimuth
	$(BINDIR)/azimuth
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
