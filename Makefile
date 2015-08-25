#------------------------------------------------------------------------------
# Makefile
#
# Project Name: cloud masking
# Makefile that will call 'make X' in subdirectories.
# Assumes the following Makefiles exist:
#	l4-7_cfmask/Makefile
#	not-validated-prototype-l8_cfmask/Makefile
#	scripts/Makefile
#------------------------------------------------------------------------------
DIR_L4-7 = l4-7_cfmask
DIR_L8 = not-validated-prototype-l8_cfmask

all: all-l4-7 all-l8

install: install-l4-7 install-l8

clean: clean-l4-7 clean-l8

#------------------------------------------------------------------------------
all-script:
	echo "make all in scripts"; \
        (cd scripts; $(MAKE) all -f Makefile);

install-script:
	echo "make install in scripts"; \
        (cd scripts; $(MAKE) install -f Makefile);

clean-script:
	echo "make clean in scripts"; \
        (cd scripts; $(MAKE) clean -f Makefile);

#------------------------------------------------------------------------------
all-l8: all-script
	echo "make all in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) all -f Makefile);

install-l8: install-script
	echo "make install in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) install -f Makefile);

clean-l8: clean-script
	echo "make clean in cfmask_l8"; \
        (cd $(DIR_L8); $(MAKE) clean -f Makefile);

#------------------------------------------------------------------------------
all-l4-7: install-script
	echo "make all in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) all -f Makefile);

install-l4-7: install-script
	echo "make install in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) install -f Makefile);

clean-l4-7: clean-script
	echo "make clean in cfmask_l4-7"; \
        (cd $(DIR_L4-7); $(MAKE) clean -f Makefile);


