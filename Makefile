# jpnevulator - serial reader/writer
# Copyright (C) 2006-2020 Freddy Spierenburg
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# Name of the binary.
NAME=jpnevulator

# Destination where to put our stuff 'make install'.
ifeq ($(origin DESTDIR), undefined)
	bindir=/usr/local/bin
	mandir=/usr/local/man/man1
else
	bindir=$(DESTDIR)/usr/bin
	mandir=$(DESTDIR)/usr/share/man/man1
endif

# List of the objects to built.
OBJECTS=main.o
OBJECTS+=options.o
OBJECTS+=jpnevulator.o
OBJECTS+=byte.o
OBJECTS+=interface.o
OBJECTS+=tty.o
OBJECTS+=pty.o
OBJECTS+=io.o
OBJECTS+=checksum.o
OBJECTS+=crc16.o
OBJECTS+=crc8.o
OBJECTS+=list.o
OBJECTS+=misc.o

# List of the manual pages.
MANPAGES=jpnevulator.1.gz

# Tools 
CLIBS?=
CFLAGS+=-Wall
LDFLAGS?=
CC?=gcc
GZIP=gzip
INSTALL=install

.PHONY: all FORCE clean install

all: $(NAME) $(MANPAGES)

$(NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(NAME) $(CLIBS) $(OBJECTS)

$(MANPAGES):
	$(GZIP) --best -c `echo $@|sed 's/\.gz$$//'` > $@

clean:
	rm -f $(NAME) $(OBJECTS) $(MANPAGES)

install: $(NAME) $(MANPAGES)
	$(INSTALL) -D -m 0755 $(NAME) $(bindir)/$(NAME)
	for manual in $(MANPAGES); do \
		$(INSTALL) -D -m 0644 $$manual $(mandir)/$$manual; \
	done

# Dummy target as dependecy if something has to be build everytime
FORCE:

# Automatic collection of dependencies in the source files.
# It's only updated the first time, after that it must be done maually 
# with "make depend"

# Target to update the file, it's removed first
dependall: dependrm depend

#remove the file
dependrm:
	rm -f dependencies.in
	
# Build the file that contains the dependencies. No deps in this rule.
# If there were deps it would be rebuilt every change, which is unneeded:
depend: dependencies.in 
	@echo "depend"

dependencies.in:
	$(CC) -MM $(CFLAGS) $(patsubst %.o,%.c,$(OBJECTS)) >$@

# The dependecies are included from a separate file:
-include dependencies.in
