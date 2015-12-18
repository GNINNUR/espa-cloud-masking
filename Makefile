#-----------------------------------------------------------------------------
# Makefile
#
# Project Name: cloud masking
#-----------------------------------------------------------------------------
.PHONY: check-environment all install clean all-script install-script clean-script all-static-data install-static-data clean-static-data all-cfmask install-cfmask clean-cfmask

include make.config

DIR_CFMASK = cfmask

all: all-script all-static-data all-cfmask

install: check-environment install-script install-static-data install-cfmask

clean: clean-script clean-static-data clean-cfmask

#-----------------------------------------------------------------------------
all-script:
	echo "make all in scripts"; \
        (cd scripts; $(MAKE) all);

install-script:
	echo "make install in scripts"; \
        (cd scripts; $(MAKE) install);

clean-script:
	echo "make clean in scripts"; \
        (cd scripts; $(MAKE) clean);

#-----------------------------------------------------------------------------
all-static-data:
	echo "make all in static_data"; \
        (cd static_data; $(MAKE) all);

install-static-data:
	echo "make install in static_data"; \
        (cd static_data; $(MAKE) install);

clean-static-data:
	echo "make clean in static_data"; \
        (cd static_data; $(MAKE) clean);

#-----------------------------------------------------------------------------
all-cfmask: all-script all-static-data
	echo "make all in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) all);

install-cfmask: install-script install-static-data
	echo "make install in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) install);

clean-cfmask: clean-script clean-static-data
	echo "make clean in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) clean);

#-----------------------------------------------------------------------------
check-environment:
ifndef PREFIX
    $(error Environment variable PREFIX is not defined)
endif

