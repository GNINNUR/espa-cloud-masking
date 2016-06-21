#-----------------------------------------------------------------------------
# Makefile
#
# Project Name: cloud masking
#-----------------------------------------------------------------------------
.PHONY: check-environment all install clean all-script install-script clean-script all-cfmask install-cfmask clean-cfmask rpms cfmask-rpm

include make.config

DIR_CFMASK = cfmask

all: all-script all-cfmask

install: check-environment install-script install-cfmask

clean: clean-script clean-cfmask

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
all-cfmask:
	echo "make all in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) all);

install-cfmask:
	echo "make install in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) install);

clean-cfmask:
	echo "make clean in cfmask"; \
        (cd $(DIR_CFMASK); $(MAKE) clean);

#-----------------------------------------------------------------------------
rpms: cfmask-rpm

cfmask-rpm:
	rpmbuild -bb --clean RPM_spec_files/RPM-CFmask.spec

#-----------------------------------------------------------------------------
check-environment:
ifndef PREFIX
    $(error Environment variable PREFIX is not defined)
endif

