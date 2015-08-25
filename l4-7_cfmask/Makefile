#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for building and installing L4-7 cfmask.
#------------------------------------------------------------------------------

export ESPA_APPLICATION = cfmask

all:
	echo "make all in src..."; \
        (cd src; $(MAKE) -f Makefile)

install: all
	echo "make install in src..."; \
        (cd src; $(MAKE) -f Makefile install)

clean:
	echo "make clean in src..."; \
        (cd src; $(MAKE) -f Makefile clean)

