#-----------------------------------------------------------------------------
# Makefile
#
# Simple makefile for building and installing L4-7 cfmask.
#-----------------------------------------------------------------------------
.PHONY: all install clean

all:
	echo "make all in src..."; \
        (cd src; $(MAKE))

install: all
	echo "make install in src..."; \
        (cd src; $(MAKE) install)

clean:
	echo "make clean in src..."; \
        (cd src; $(MAKE) clean)

