#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: check-environment

all:

install: check-environment
	install -d $(PREFIX)/$(ESPA_PROJECT)/bin
	install -m 775 cfmask.py $(PREFIX)/$(ESPA_PROJECT)/bin
	install -m 775 run_fillminima.py $(PREFIX)/$(ESPA_PROJECT)/bin
	install -m 644 fillminima.py $(PREFIX)/$(ESPA_PROJECT)/bin

clean:

check-environment:
ifndef ESPA_PROJECT
    $(error ESPA_PROJECT is undefined)
endif

