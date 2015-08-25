#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: check-environment

export PROJECT_NAME = espa-scripts

all:

install: check-environment
	install -d $(PREFIX)/$(PROJECT_NAME)/bin
	install -m 775 cfmask.py $(PREFIX)/$(PROJECT_NAME)/bin

clean:

check-environment:
ifndef PROJECT_NAME
    $(error PROJECT_NAME is undefined)
endif

