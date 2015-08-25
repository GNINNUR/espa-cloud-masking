#------------------------------------------------------------------------------ # Makefile
#
# For building L4-7 cfmask.
#------------------------------------------------------------------------------
.PHONY: check-environment

STATIC_INSTALL_PATH = $(PREFIX)/$(ESPA_PROJECT)/static_data

all:

install: check-environment
	install -d $(STATIC_INSTALL_PATH)
	install -m 644 EarthSunDistance.txt $(STATIC_INSTALL_PATH)

clean:

check-environment:
ifndef PREFIX
    $(error PREFIX is undefined)
endif
ifndef ESPA_PROJECT
    $(error ESPA_PROJECT is undefined)
endif

