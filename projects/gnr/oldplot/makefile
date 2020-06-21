# (c) Copyright 2017, The Rockefeller University *11225* 
# $Id: makefile 33 2017-05-03 16:14:57Z  $
########################################################################
#          Make file for old ROCKS metafile plotting library           #
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
# Rev, 05/02/17, GNR - Add rkgtest                                     #
########################################################################

.PHONY: all SERPLOT PLOTP0 PLOTPn rkgtest RKGTSER RKGTP0 RKGTPn

########################################################################
#    Here are the targets that can be entered from the command line    #
#############Lin########################################################

all:		SERPLOT PLOTP0 PLOTPn
rkgtest:	RKGTSER RKGTP0 RKGTPn

help Help HELP h H:
	@echo "To make full set of old plot libraries for LINUX,"
	@echo "    use the command 'make' or 'make all'"
	@echo "To make just the serial, PAR0, or PARn library, use"
	@echo "    'make SERPLOT', 'make PLOTP0', or 'make PLOTPn'"
	@echo "To make rkgtest test program (Rkgtest ser, rkgtP0, rkgtPn),"
	@echo "    first make the libraries, then 'make rkgtest'"

SERPLOT:
	$(MAKE) -f maklux64

PLOTP0:
	$(MAKE) -f mak64P0

PLOTPn:
	$(MAKE) -f mak64Pn

RKGTSER:
	$(MAKE) -f maklux64 Rkgtest

RKGTP0:
	$(MAKE) -f mak64P0 rkgtP0

RKGTPn:
	$(MAKE) -f mak64Pn rkgtPn

clean:
	$(MAKE) -f maklux64 clean
	$(MAKE) -f mak64P0  clean
	$(MAKE) -f mak64Pn  clean

