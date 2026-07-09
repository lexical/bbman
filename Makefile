# Convenience wrapper for the maintained wxWidgets 3 / GTK3 build.

.PHONY: all clean install uninstall

all clean install uninstall:
	$(MAKE) -C unix -f Makefile.wx3 $@
