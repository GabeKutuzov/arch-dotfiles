# (c) Copyright 1992, Neurosciences Research Foundation, Inc.
#################################################################
#      Makefile for building nsi_cpp 
#
#  This makefile uses uses KEEP_STATE to store the information 
#  regarding header files.  
#  
#  Version 1, 10/26/92, ROZ                                      
#################################################################

OBJS = nsi_cpp.o 

# Directory for installation of executable
XD = /home/nsi/bin
# Shared directories
GI = /home/nsi/include

# Compilation flags
CFLAGS = 	-c -g -I$(GI)

.KEEP_STATE:

# Link libraries 
nsi_cpp:	 	$(OBJS)
		acc -o $@ $(OBJS)
		mv nsi_cpp $(XD)

# Compile the program 
nsi_cpp.o: 	nsi_cpp.c
		acc $(CFLAGS) $< 
