# (c) Copyright 2016, The Rockefeller University *11225* 
# $Id: makefile 2 2009-12-30 17:16:03Z  $
########################################################################
#          Make file for V9 ROCKS metafile plotting library            #
#                     Version for 64-bit PC Linux                      #
#                                                                      #
# This is the master make file for 64-bit MPI systems.  It runs        #
# maklux64, mak64P0, and mak64Pn to make serial, PAR0 ("host"), and    #
# PARn ("comp node") libraries.                                        #
#                                                                      #
# Call with 'make CFLAGS=-O2 <target>' to get optimized compilations.  #
#                                                                      #
# V1A, 07/21/16, GNR - New routine, derived from mpitools makefile     #
# ==>, 07/21/16, GNR - Last mod before committing to svn repository    #
########################################################################

.PHONY: all SERPLOT PLOTP0 PLOTPn

########################################################################
#    Here are the targets that can be entered from the command line    #
#############Lin########################################################

all:		SERPLOT PLOTP0 PLOTPn

help Help HELP h H:
	@echo "To make full set of V9 plot libraries for LINUX,"
	@echo "    use the command 'make' or 'make all'"
	@echo "To make just the serial, PAR0, or PARn library, use"
	@echo "    'make SERPLOT', 'make PLOTP0', or 'make PLOTPn'"

SERPLOT:
	$(MAKE) -f maklux64

PLOTP0:
	$(MAKE) -f mak64P0

PLOTPn:
	$(MAKE) -f mak64Pn

clean:
	$(MAKE) -f maklux64 clean
	$(MAKE) -f mak64P0  clean
	$(MAKE) -f mak64Pn  clean

