# (c) Copyright 1992, Neurosciences Research Foundation, Inc.
#################################################################
#                   makefile for mfprint
#
#  This makefile creates the program "mfprint". It uses
#  KEEP_STATE to store the information regarding header files 
#  and other dependencies relating to "mfprint".  The program 
#  assumes that the environment variable OPENWINHOME and XGLHOME
#  are defined. If this is not the case then before using this 
#  file you should define OPENWINHOME and XGLHOME with the right   
#  directories.(usually OPENWINHOME=/usr/openwin,XGLHOME=/usr/xgl)
#  
#  Version 1, 09/16/92, ROZ                                      
#################################################################

XGLHOME = /usr/xgl
OPENWINHOME = /usr/openwin

# Add programs and local libraries on next line
REQUIRED = 	mfconv.o \
		/home/nsi/lib/sparc/plotps.a \
		/home/nsi/lib/sparc/libxglut.a \
		/home/nsi/lib/sparc/orplotxgl.a  

# Shared directories
SD =		/home/nsi/include/sparc
GD =		/home/nsi/include
XG =		$(XGLHOME)/include
XW =		$(OPENWINHOME)/include

# Openwin libraries
OW_LIBS =	-lxview -lolgx -lX11  -lm 
LIBS =		-L$(XGLHOME)/lib -L$(OPENWINHOME)/lib -lxgl $(OW_LIBS)

# Compilation flags
CFLAGS = 	-c -g -DSUN4 -DPLOTXGL -DPLOTPS -DPLOT \
		-I$(SD) -I$(GD) -I$(XG) -I$(XW)

.c.o:
		acc $(CFLAGS) $< 

.KEEP_STATE:

mfprint:	mfprint.o $(REQUIRED)
		acc -o $@ $@.o $(REQUIRED) $(LIBS)  
		mv mfprint /home/nsi/bin/mfprint
