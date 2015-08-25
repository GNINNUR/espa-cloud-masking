#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: check-environment

all:

install: check-environment
	install -d $(PREFIX)/$(PROJECT_NAME)/bin
	install -m 755 run_fillminima.py $(PREFIX)/$(PROJECT_NAME)/bin
	install -m 664 fillminima.py $(PREFIX)/$(PROJECT_NAME)/bin

clean:

check-environment:
ifndef PROJECT_NAME
    $(error PROJECT_NAME is undefined)
endif

