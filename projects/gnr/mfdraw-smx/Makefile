#################################################################
#                   Makefile for mfdraw	                        #
#								#
# 								#
#  								#
# Version: initial mfdraw version  2/21/2006        Steven Marx # 
# Rev, 11/6/07, GNR - Change /home/smarx to $(HOME) etc.        #
#################################################################

#includes
INCLUDES  = -I/$(HOME)/include -I/usr/X11R6/include -I/usr/X11R6/include/X11 -I/usr/include/GL

# libraries
LDIRS  = -L/$(HOME)/lib/pclinux -L/usr/lib -L/usr/X11R6/lib 

#before adding  libXmu and libXi and libglut to support glut stroke fonts
#these libs are sufficient to handle bit mapped glut fonts (note no need for lglut)
#LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lm -lXext -lX11 -lsupc++ \
#       -lGL -lGLU -lGLw

#added libXmu and libXi and libglut to support glut stroke fonts
#LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lm -lXext -lX11 -lXmu -lXi -lsupc++ \
#       -lglut -lGL -lGLU -lGLw
LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lm -lXext -lX11 -lXmu -lXi -lsupc++ \
       -lglut -lGL -lGLU

#following LIBS which includes inappropriate fully qualified libglut.so filename permits us to link on NEW release of Linux
#LIBS = -lfltk -lfltk_forms -lfltk_gl -lfltk_images -lm -lXext -lX11 -lXmu -lXi -lsupc++ \
#       /usr/lib/libglut.so.3.8.0  -lGL -lGLU -lGLw

# Compilation flags
CXXFLAGS    = -g -c  -DMESA -Wall

#MFLOG when defined enables logging of all drawing commands

# Compiler (note: -pg is for profiler)
CC = gcc

EXECUT = mfdraw
OBJS   = mfdraw.o mfdrawmain.o mfdrawUI.o mfdrawsockserver.o

# experiment with forcing linker to search given dir
# use  "--rpath /home/smarx/mfdraw" in next command and
# add /home/smarx/mfdraw to /etc/ld.so.conf and
# refresh cache with ldconfig as root

$(EXECUT) : $(OBJS)
	$(CC) $(OBJS) -o $(EXECUT) $(LDIRS) $(LIBS)

$(OBJS):%.o:%.cxx
	$(CC) $(CXXFLAGS) $(INCLUDES) $< -o $@

$(EXECUT)UI.h  $(EXECUT)UI.cxx : $(EXECUT)UI.fl 
	fluid -c $(EXECUT)UI.fl

mfdrawUI.o mfdrawmain.o : mfdrawUI.h

mfdrawmain.o mfdrawsockserver.o mfdraw.o : mfdrawsockserver.h mfdraw.h

#if the Makefile has changed then force compile and link
#on all source and resulting objects
$(OBJS) : Makefile

.PHONY : cleanall	
cleanall :
	rm -f  core* *~  *.o $(EXECUT)
	@echo 'removed all core, *~, object, and executable files'
