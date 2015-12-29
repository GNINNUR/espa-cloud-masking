#-----------------------------------------------------------------------------
# Makefile
#
# For building cfmask.
#-----------------------------------------------------------------------------
.PHONY: all install clean

# Inherit from upper-level make.config
TOP = ../..
include $(TOP)/make.config

#-----------------------------------------------------------------------------
# Set up compile options
CC    = gcc
RM    = rm
EXTRA = -Wall $(EXTRA_OPTIONS)

# Define the include files
INC = cfmask.h const.h error.h fill_local_minima_in_image.h \
      identify_clouds.h input.h misc.h output.h

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
      convert_and_generate_statistics.c  \
      cfmask.c
OBJ = $(SRC:.c=.o)

# Define include paths
INCDIR  = -I. -I$(ESPAINC) -I$(XML2INC)
NCFLAGS = $(EXTRA) $(INCDIR)

# Define the object libraries and paths
EXLIB = -L$(ESPALIB) -l_espa_raw_binary -l_espa_common \
        -L$(XML2LIB) -lxml2 \
        -L$(LZMALIB) -llzma \
        -L$(ZLIBLIB) -lz
MATHLIB = -lm
LOADLIB = $(EXLIB) $(MATHLIB)

# Define C executables
EXE = cfmask

#-----------------------------------------------------------------------------
all: $(EXE)

$(EXE): $(OBJ) $(INC)
	$(CC) $(EXTRA) -o $(EXE) $(OBJ) $(LOADLIB)

#-----------------------------------------------------------------------------
install: $(EXE)
	install -d $(link_path)
	install -d $(cfmask_bin_install_path)
	install -m 755 $(EXE) $(cfmask_bin_install_path)
	ln -sf $(cfmask_link_source_path)/$(EXE) $(link_path)/$(EXE)

#-----------------------------------------------------------------------------
clean:
	$(RM) -f *.o $(EXE)

#-----------------------------------------------------------------------------
$(OBJ): $(INC)

.c.o:
	$(CC) $(NCFLAGS) -c $<

