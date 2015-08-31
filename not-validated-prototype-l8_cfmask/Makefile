#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for building and installing L8 cfmask.
#------------------------------------------------------------------------------
.PHONY: all install clean

all:
	echo "make all in src..."; \
        (cd src; $(MAKE) -f Makefile)

install: all
	echo "make install in src..."; \
        (cd src; $(MAKE) -f Makefile install)

clean:
	echo "make clean in src..."; \
        (cd src; $(MAKE) -f Makefile clean)

