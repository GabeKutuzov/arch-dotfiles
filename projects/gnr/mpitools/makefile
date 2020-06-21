# (c) Copyright 1999-2016, The Rockefeller University *21224* 
# $Id: makefile 3 2017-01-16 19:29:37Z  $
########################################################################
#                  MAKE file for MPI tools utilities                   #
#                                                                      #
# These utilities support node-to-node messaging and shared memory on  #
# parallel computers.  They replace the earlier "nctools" and          #
# "nsitools" libraries.  This is the master make file for 64-bit MPI   #
# systems.  It runs maklux64, mak64P0, and mak64Pn to make serial,     #
# PAR0 ("host"), and PARn ("comp node") libraries.                     #
#                                                                      #
# Call with 'make CFLAGS=-O2 <target>' to get optimized compilations.  #
#                                                                      #
# N.B.  All libraries now contain the stattmr() function, but calls to #
# it in an application are nulled out by a macro in mpitools.h unless  #
# TIMERS is pound-defined before mpitools is included in the source.   #
#                                                                      #
# (Some obsolete history removed here)                                 #
# V1A, 12/15/92, ABP - Initial version                                 #
# Rev, 05/04/99, GNR - New directory layout                            #
# Rev, 08/26/99, GNR - Add allocp*, imax, isum, nnget*, nnput*         #
# Rev, 10/24/99, GNR - Add nn{get|put}[iu]8                            #
# Rev, 03/04/00, GNR - Remove allocs/frees routines, add libsertools   #
# Rev, 04/22/00, GNR - Add abexit                                      #
# Rev, 12/09/00, GNR - Add blkbcst                                     #
# V1F, 06/30/01, GNR - Add chngpool                                    #
# Rev, 12/27/08, GNR - Add 'clean' target, dependency on makefile      #
# Rev, 06/29/16, GNR - Update for mpi version                          #
# ==>, 06/29/16, GNR - Last mod before committing to svn repository    #
########################################################################

.PHONY: all SERTOOLS PAR0TOOLS PARnTOOLS

########################################################################
#    Here are the targets that can be entered from the command line    #
#############Lin########################################################

all:		SERTOOLS PAR0TOOLS PARnTOOLS

help Help HELP h H:
	@echo "To make full set of mpitools libraries for LINUX,"
	@echo "    use the command 'make' or 'make all'"
	@echo "To make just the serial, PAR0, or PARn library, use"
	@echo "    'make SERTOOLS', 'make PAR0TOOLS', or 'make PARnTOOLS'"

SERTOOLS:
	$(MAKE) -f maklux64

PAR0TOOLS:
	$(MAKE) -f mak64P0

PARnTOOLS:
	$(MAKE) -f mak64Pn

clean:
	$(MAKE) -f maklux64 clean
	$(MAKE) -f mak64P0  clean
	$(MAKE) -f mak64Pn  clean

