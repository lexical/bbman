SHELL = /bin/sh


subdirs = libssh/
top_srcdir = .
srcdir = .
prefix = /usr/local
exec_prefix = ${prefix}
bindir = $(exec_prefix)/bin
incldir= $(prefix)/include
infodir = $(prefix)/info
libdir = $(prefix)/lib/
mandir = $(prefix)/man/man1

CC = gcc
CFLAGS = -g -O2 -Iinclude/ -Wall -g
LDFLAGS =  
LIBS = -lssh -pthread -Llibssh/
INSTALL = /usr/bin/install -c
LN= ln -s
OBJECTS= client.o libssh/*.o
VERSION=0.1
DISTLIB=libssh-$(VERSION)
CONFIG=include/libssh/config.h
all: $(CONFIG) $(OBJECTS)
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) all) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
	$(CC) -o ssh $(OBJECTS) $(LDFLAGS) $(LIBS)
	$(LN) -sf ssh sftp
$(CONFIG):
	$(LN) -f ../../config.h $(CONFIG)
dist:
	rm -fr $(DISTLIB)
	mkdir $(DISTLIB)
	cp Makefile.in configure.in configure config.h.in install-sh \
	mkinstalldirs config.sub config.guess $(DISTLIB)
	mkdir $(DISTLIB)/libssh
	mkdir $(DISTLIB)/include
	mkdir $(DISTLIB)/include/libssh
#	mkdir $(DISTLIB)/doc
	cp libssh/Makefile.in $(DISTLIB)/libssh/
	cp libssh/*.c $(DISTLIB)/libssh/
	cp include/libssh/libssh.h include/libssh/sftp.h \
	include/libssh/crypto.h include/libssh/ssh2.h  $(DISTLIB)/include/libssh/
	cp *.c API.txt COPYING README AUTHORS CHANGELOG $(DISTLIB)/
#	cp doc/* $(DISTLIB)/doc/
	tar czf $(DISTLIB).tgz $(DISTLIB)/
install:
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) install) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
	$(top_srcdir)/mkinstalldirs $(incldir)/libssh
	$(INSTALL) include/libssh/libssh.h $(incldir)/libssh/
	$(INSTALL) include/libssh/config.h $(incldir)/libssh/
	$(INSTALL) include/libssh/sftp.h $(incldir)/libssh/
	$(INSTALL) include/libssh/crypto.h $(incldir)/libssh/
clean:
	/bin/rm -f *~ *.o ssh sftp
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) clean) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

distclean: clean
	/bin/rm -f Makefile config.h config.status config.cache config.log
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) distclean) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

