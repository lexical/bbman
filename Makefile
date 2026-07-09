# Convenience wrapper for the maintained wxWidgets 3 / GTK3 build.

.PHONY: all clean install

all clean install:
	$(MAKE) -C unix -f Makefile.wx3 $@
