#------------------------------------------------------------------------------
# Makefile
#
# Simple makefile for installing the scripts.
#------------------------------------------------------------------------------
.PHONY: all install clean

# Inherit from upper-level make.config
TOP = ..
include $(TOP)/make.config

# Define the script installation path
link_path = $(PREFIX)/bin
script_install_path = $(espa_project_dir)/bin

SCRIPT_EXECUTABLES = \
    cloud_masking.py \
    run_fillminima.py

SCRIPT_IMPORTS = \
    fillminima.py

all:

install:
	install -d $(link_path)
	install -d $(script_install_path)
	@for file in $(SCRIPT_EXECUTABLES); do \
            echo "  installing $$file"; \
            install -m 755 $$file $(script_install_path); \
            ln -sf ../$(project_name)/bin/$$file $(link_path)/$$file; \
        done;
	@for file in $(SCRIPT_IMPORTS); do \
            echo "  installing $$file"; \
            install -m 644 $$file $(script_install_path); \
            ln -sf ../$(project_name)/bin/$$file $(link_path)/$$file; \
        done;

clean:

