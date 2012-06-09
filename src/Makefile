# Copyright (C) 2008 by Johannes Overmann <Johannes.Overmann@gmx.de>
# Please see COPYING for license.

# --- config ----------------------------------------------------------------
WARN = -Wall -W -g
#OPT = -O2
OPT =
CPPFLAGS += $(ADDITIONAL_CPPFLAGS)
CXXFLAGS += $(WARN) $(OPT)
CXX = g++
CC = $(CXX)

# --- default target
default: all

# --- target definition -----------------------------------------------------
-include Makefile.init # indirectly include TARGET
include TARGET
SRC := $(wildcard *.cc) $(ADDITIONAL_SOURCES)
VERSION := $(shell grep '\#define VERSION' $(TARGET).cc | sed 's/.*"\([^"]*\)".*/\1/g')
DISTFILES := $(SRC) $(wildcard *.h) TARGET INSTALL COPYING Makefile configure $(wildcard $(TARGET).1)


# --- common rules ----------------------------------------------------------
OBJ := $(SRC:.cc=.o)

all: $(TARGET)

usage:
	@echo "Targets: $(TARGET) strip install dist clean"

$(TARGET): $(OBJ)

strip:
	strip $(TARGET)

install: all strip
	cp $(TARGET) /usr/local/bin

PACKAGE = $(TARGET)-$(VERSION)
dist:
	rm -rf $(PACKAGE)
	mkdir $(PACKAGE)
	cp $(DISTFILES) $(PACKAGE)
	tar czvhf $(PACKAGE).tgz $(PACKAGE)

clean:
	rm -f $(OBJ) $(DEP) $(TARGET) *~ $(PACKAGE).tgz
	rm -rf $(PACKAGE) $(ADDITIONAL_CLEANFILES)

svnclean: clean

mantxt: 
	nroff -c -man mp3check.1 | sed 's/.'$(shell echo -e '\010')'//g' > mp3check-man.txt


.PHONY: default all clean strip dist svnclean install usage mantxt

# --- meta object compiler for qt -------------------------------------------
moc_%.cc: %.h
	moc -o $@ $<
	
	
# --- dependency generation -------------------------------------------------
.dep.%: %.cc
	$(CXX) $(CPPFLAGS) -MM -MT "$@ $(<:%.cc=%.o)" $< -o $@
DEP := $(SRC:%.cc=.dep.%)
ifeq ($(findstring $(MAKECMDGOALS),clean),)
ifeq ($(findstring $(MAKECMDGOALS),svnclean),)
-include $(DEP)
endif
endif
		
