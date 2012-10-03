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
OUTDIR = out
OBJDIR = $(OUTDIR)/obj
BINDIR = $(OUTDIR)/bin

CFLAGS = -Wall -O1
C99FLAGS = -std=c99 -pedantic $(CFLAGS)

C99FILES := $(shell find $(SRCDIR) -name '*.c')
HEADERS := $(shell find $(SRCDIR) -name '*.h')
OBJFILES := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(C99FILES))

#=============================================================================#

$(BINDIR)/azimuth: $(OBJFILES) $(OBJDIR)/SDLMain.o
	@echo "Linking $@"
	@mkdir -p $(@D)
	@gcc -o $@ $^ $(CFLAGS) \
	     -framework Cocoa -framework OpenGL -framework SDL

$(OBJDIR)/SDLMain.o: $(SRCDIR)/SDLMain.m $(SRCDIR)/SDLMain.h
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(CFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@echo "Compiling $@"
	@mkdir -p $(@D)
	@gcc -o $@ -c $< $(C99FLAGS) -Isrc/

#=============================================================================#

.PHONY: run
run: $(BINDIR)/azimuth
	$(BINDIR)/azimuth

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
