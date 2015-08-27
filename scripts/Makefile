#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: check-environment

SCRIPT_INSTALL_PATH = $(PREFIX)/$(ESPA_PROJECT)/bin

all:

install: check-environment
	install -d $(SCRIPT_INSTALL_PATH)
	install -m 775 cloud_masking.py $(SCRIPT_INSTALL_PATH)
	install -m 775 run_fillminima.py $(SCRIPT_INSTALL_PATH)
	install -m 644 fillminima.py $(SCRIPT_INSTALL_PATH)

clean:

check-environment:
ifndef PREFIX
    $(error PREFIX is undefined)
endif
ifndef ESPA_PROJECT
    $(error ESPA_PROJECT is undefined)
endif

