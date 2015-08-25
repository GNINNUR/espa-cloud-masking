#------------------------------------------------------------------------------ # Makefile
#
# For building L4-7 cfmask.
#------------------------------------------------------------------------------
.PHONY: check-environment

all:

install: check-environment
	install -d $(PREFIX)/$(PROJECT_NAME)/static_data
	install -m 644 EarthSunDistance.txt $(PREFIX)/$(PROJECT_NAME)/static_data

clean:

check-environment:
ifndef PROJECT_NAME
    $(error PROJECT_NAME is undefined)
endif

