#-----------------------------------------------------------------------------
# Makefile
#
# Project Name: cloud masking
#-----------------------------------------------------------------------------
.PHONY: check-environment all install clean all-script install-script clean-script all-static-data install-static-data clean-static-data all-l4-7 install-l4-7 clean-l4-7 all-l8 install-l8 clean-l8

include make.config

DIR_L4-7 = l4-7_cfmask
DIR_L8 = not-validated-prototype-l8_cfmask

all: all-script all-static-data all-l4-7 all-l8

install: check-environment install-script install-static-data install-l4-7 install-l8

clean: clean-script clean-static-data clean-l4-7 clean-l8

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
all-l8: all-script all-static-data
	echo "make all in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) all);

install-l8: install-script install-static-data
	echo "make install in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) install);

clean-l8: clean-script clean-static-data
	echo "make clean in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) clean);

#-----------------------------------------------------------------------------
all-l4-7: all-script all-static-data
	echo "make all in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) all);

install-l4-7: install-script install-static-data
	echo "make install in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) install);

clean-l4-7: clean-script clean-static-data
	echo "make clean in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) clean);

#-----------------------------------------------------------------------------
check-environment:
ifndef PREFIX
    $(error Environment variable PREFIX is not defined)
endif

