#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: check-environment

BIN_INSTALL_PATH = $(PREFIX)/$(ESPA_PROJECT)/$(ESPA_APPLICATION)/bin

all:

install: check-environment
	install -d $(BIN_INSTALL_PATH)
	install -m 755 run_fillminima.py $(BIN_INSTALL_PATH)
	install -m 664 fillminima.py $(BIN_INSTALL_PATH)

clean:

check-environment:
ifndef PREFIX
    $(error PREFIX is undefined)
endif
ifndef ESPA_PROJECT
    $(error ESPA_PROJECT is undefined)
endif
ifndef ESPA_APPLICATION
    $(error ESPA_APPLICATION is undefined)
endif

