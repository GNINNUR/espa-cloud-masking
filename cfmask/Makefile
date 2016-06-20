#-----------------------------------------------------------------------------
# Makefile
#
# Simple makefile for building and installing cfmask.
#-----------------------------------------------------------------------------
.PHONY: all install clean

all:
	echo "make all in src..."; \
        (cd src; $(MAKE))

install: all
	echo "make install in src..."; \
        (cd src; $(MAKE) install)
	echo "make install in static_data..."; \
        (cd static_data; $(MAKE) install)

clean:
	echo "make clean in src..."; \
        (cd src; $(MAKE) clean)

