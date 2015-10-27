#-----------------------------------------------------------------------------
# Makefile
#
# For building L4-7 cfmask.
#-----------------------------------------------------------------------------
.PHONY: all install clean

# Inherit from upper-level make.config
TOP = ../..
include $(TOP)/make.config

bin_install_path = $(l47_algorithm_dir)/bin

# Set up compile options
CC    = gcc
RM    = rm -f
EXTRA_OPTIONS = $(static_option) $(threading_options) $(profiling_options)
EXTRA = -Wall -g -O2 $(EXTRA_OPTIONS)

# Define the include files
INC = cfmask.h const.h error.h fill_local_minima_in_image.h \
      identify_clouds.h input.h misc.h output.h
INCDIR  = -I. -I$(ESPAINC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the source code and object files
SRC = \
      misc.c                             \
      error.c                            \
      input.c                            \
      output.c                           \
      identify_clouds.c                  \
      fill_local_minima_in_image.c       \
      potential_cloud_shadow_snow_mask.c \
      object_cloud_shadow_match.c        \
      cfmask.c
OBJ = $(SRC:.c=.o)

# Define the object libraries
EXLIB = -L$(ESPALIB) -l_espa_raw_binary -l_espa_common \
        -l_espa_format_conversion \
        -L$(XML2LIB) -lxml2 \
        -L$(LZMALIB) -llzma \
        -L$(ZLIBLIB) -lz
MATHLIB = -lm
LOADLIB = $(EXLIB) $(MATHLIB)

# Define the executable
EXE = cfmask

# Target for the executable
all: $(EXE)

$(EXE): $(OBJ) $(INC)
	$(CC) $(EXTRA) -o $(EXE) $(OBJ) $(LOADLIB)

install: $(EXE)
	install -d $(link_path)
	install -d $(bin_install_path)
	install -m 755 $(EXE) $(bin_install_path)
	ln -sf ../$(project_name)/$(l47_algorithm)/bin/$(EXE) $(link_path)/$(EXE)

clean:
	$(RM) *.o $(EXE)

$(OBJ): $(INC)

.c.o:
	$(CC) $(NCFLAGS) -c $<

