# (c) Copyright 2007-2014, The Rockefeller University *21223* 
########################################################################
#                Makefile for mfdraw (32-bit version)                  #
#                                                                      #
# Version: initial mfdraw version  2/21/2006        Steven Marx        #
# Rev, 11/06/07, GNR - Change /home/smarx to $(HOME) etc.              #
# Rev, 01/15/09, GNR - Pass svn revision number to code via -DSVNVRS   #
# Rev, 02/09/12, GNR - Store object files in pclinux subdirectory      #
# Rev, 02/12/14, GNR - Modified for fltk 1.3.2 w/glut emulation        #
########################################################################

# Define architecture-specific subdirectories
DIR	:= pclinux
LDIR	:= $(HOME)/lib/$(DIR)

# Includes
INCLUDES  = -I$(HOME)/include

# Libraries
LDIRS  = -L$(LDIR) -L/usr/local/lib 

# For unknown reasons, -lXcursor is needed at CoveRd, not at Lab
LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lXext -lXft \
	-lfontconfig -lXinerama -lXcursor -lpthread -ldl -lm -lGL -lGLU -lX11

#Following LIBS which includes inappropriate fully qualified libglut.so
#filename permits us to link on NEW release of Linux
#LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lm -lXext -lX11\
#	-lXmu -lXi -lsupc++ /usr/lib/libglut.so.3.8.0  -lGL -lGLU -lGLw

CC := gcc
# Compilation flags--add -pg for profiler
CXXFLAGS := -g -Wall
SVNV := $(shell gnrversn)
# Compilation defs--See mfdraw.h for useful values of -DDEBUG
DEFS := PCLINUX MESA _ISOC99_SOURCE SVNVRS=$(patsubst %,'"%"',$(SVNV))

EXECUT = mfdraw
OBJS   = mfdraw mfdrawmain mfdrawUI mfdrawsockserver
COBJS =

# Build the executable
$(EXECUT) : $(OBJS:%=$(DIR)/%.o) $(COBJS:%=$(DIR)/%.o) 
	$(CC) $(OBJS:%=$(DIR)/%.o) $(COBJS:%=$(DIR)/%.o) -o $@ \
		$(LDIRS) $(LIBS)

$(DIR)/%.o:	%.cxx 
	$(CC) `fltk-config --cxxflags` $(CXXFLAGS) $(DEFS:%=-D%) \
		$(INCLUDES) -c $< -o $@

$(DIR)/%.o:	%.c 
	$(CC) $(CXXFLAGS) $(DEFS:%=-D%) $(INCLUDES) -c $< -o $@

$(EXECUT)UI.h  $(EXECUT)UI.cxx : $(EXECUT)UI.fl 
	fluid -c $(EXECUT)UI.fl

$(DIR)/mfdrawUI.o : mfdrawUI.cxx mfdrawUI.h

$(DIR)/mfdraw.o : mfdraw.cxx $(HOME)/include/sysdef.h \
	$(HOME)/include/plotdefs.h \
	mfdrawsockserver.h mfdraw.h mfdrawUI.h

$(DIR)/mfdrawmain.o $(DIR)/mfdrawsockserver.o : \
	mfdrawsockserver.h mfdraw.h mfdrawUI.h

.PHONY : cleanall	
cleanall :
	rm -f  core* *~  $(DIR)/*.o mfdrawUI.[hct]* $(EXECUT)
	@echo 'Removed all core, *~, object, and executable files'
