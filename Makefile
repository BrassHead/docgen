# Linux makefile for docgen
#
# Usage:
# make			Incrementally makes
# make clean		Cleans the source tree of *.o etc.
# make release		Makes a release version
# make autodoc		Generates documentation from source
# make check		Checks output against last (distributed)
# make install		Installs
#

PACKAGE = docgen
VERSION = 1.0

#RELOPTS = -g -O2 -DNDEBUG
#RELLIBS = -lbw
DBGOPTS = -g -D_DEBUG
DBGLIBS = ../bw/libbw.a

PREFIX = ~
BINDIR = $(PREFIX)/bin
CC = c++
AR = ar
RANLIB = ranlib
INSTALL = install -c

LDFLAGS = 
DEFS =
CFLAGS = $(DEFS) -I../bw/include -Wall -Werror
CCFLAGS = -std=c++11
CPPFLAGS = 

#%.o: %.cc
#	$(CC) -c $(RELOPTS) $(CCFLAGS) $(CFLAGS) $<

%.o: %.cc
	$(CC) -c $(DBGOPTS) $(CCFLAGS) $(CFLAGS) $<

SOURCES = docitem.cc main.cc docgen.cc lexstream.cc output.cc
OBJECTS = docitem.o main.o docgen.o lexstream.o output.o
BWOBJECTS = ../string.o ../exception.o

# targets


#docgen: $(OBJECTS)
#	$(CC) $(RELOPTS) $(CFLAGS) $(LDFLAGS) -o docgen $(OBJECTS) $(RELLIBS)

docgen: $(OBJECTS)
	$(CC) $(DBGOPTS) $(CFLAGS) $(LDFLAGS) -o docgen $(OBJECTS) $(DBGLIBS)


all: docgen

debug: docgen

#release: docgen

install: docgen
	$(INSTALL) docgen $(BINDIR)

docgen.o: docgen.h lexstream.h docitem.h
docitem.o: docgen.h lexstream.h docitem.h
lexstream.o: lexstream.h
main.o: docgen.h lexstream.h docitem.h
output.o: docitem.h

clean:
	rm -f *.o
	rm -f *~ doc/*~
	-rm -f docgen docgen.d
	-rm -f testout/* check.log

dist: clean
	( \
	  cd .. ; \
	  ln -s $(PACKAGE) $(PACKAGE)-$(VERSION) ; \
	  tar czf $(PACKAGE)-$(VERSION)-`date +%Y%m%d`.tar.gz \
	      $(PACKAGE)-$(VERSION) ; \
	  rm -f $(PACKAGE)-$(VERSION) ; \
	)


check: docgen
	-rm -f testout/*
	./docgen testout $(SOURCES)
	-diff doc/auto testout >check.log
	if [ -s check.log ] ; then  \
	  echo "" ; \
	  echo "" ; \
	  echo "======= Check failed -- see check.log for details" ; \
	else  \
	  echo "" ; \
	  echo "" ; \
	  echo "======= Check passed" ; \
	  rm -f check.log ; \
	fi 

# In other makefiles, I call this target docgen, but that conflicts here
autodoc: $(SOURCES)
	-rm doc/auto/*
	docgen doc/auto $(SOURCES)

restyle:
	astyle --recursive --style=stroustrup --indent=tab=4 "*.cc" "*.h"

