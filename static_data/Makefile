#-----------------------------------------------------------------------------
# Makefile
#
# For building L4-7 cfmask.
#-----------------------------------------------------------------------------
.PHONY: all install clean

# Inherit from upper-level make.config
TOP = ..
include $(TOP)/make.config

# Define the script installation path
static_install_path = $(espa_project_dir)/static_data

all:

install:
	install -d $(static_install_path)
	install -m 644 EarthSunDistance.txt $(static_install_path)

clean:

