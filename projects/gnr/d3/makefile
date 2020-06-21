# (c) Copyright 1999-2018, The Rockefeller University *11225* 
# $Id: makefile 79 2018-08-21 19:26:36Z  $
########################################################################
#         MAKE file for CNS (DARWIN III) running on PC Linux           #
#               64-bit version, serial and MPI parallel                #
#                                                                      #
# This makefile makes the following targets:                           #
#    Cns    The serial version compiled with the mpitools library      #
#    cnsP0  Parallel host (PAR0) version                               #
#    cnsPn  Parallel comp node (PARn) version                          #
#                                                                      #
# The following options are always selected by this makefile:          #
# include BBDS, no timers.                                             #
#                                                                      #
# V1A, 08/08/99, GNR - Derived from IRIX makefile                      #
# V8D, 02/14/07, GNR - Remove PNS, add BBDS                            #
# ==>, 12/29/07, GNR - Last mod before committing to svn repository    #
# Rev, 12/28/08, GNR - Add clean target, dependency on makefile itself #
# Rev, 02/16/16, GNR - Derive this 64-bit version from maklux32        #
# Rev, 02/18/16, GNR - Make separate nxdr outputs for 32- and 64-bits  #
# Rev, 02/24/16, GNR - Simpler dependency mechanism, no sed needed     #
# Rev, 03/21/16, GNR - Now just make one Cns with BBDS turned on       #
#                      Add selection of warning flags, compdate        #
# Rev, 06/25/16, GNR - Add Cnsm target, mpitools lib                   #
# Rev, 09/18/16, GNR - Separate nxdr outputs for serial, parallel      #
# R67, 10/22/16, GNR - If parallel, run d3bxvp on PAR0 node            #
# Rev, 08/03/18, GNR - Incorporate LCLHOME                             #
########################################################################

# Define directory where this stuff is stored
LCLHOME ?= $(HOME)
# Define directory where include files are stored
INC := $(LCLHOME)/include
# Define directory where ROCKS libraries are stored
LIB := $(LCLHOME)/lib/pclux64

# Object files used to build all versions of CNS
CNSOBJ := \
darwin3 \
d3autsc \
d3decay \
d3elij \
d3exit \
d3genr \
d3gfsv \
d3go   \
d3grex \
d3gvin \
d3i3vr \
d3inhb \
d3inhi \
d3lani \
d3lkni \
d3lplt \
d3mset \
d3ngph \
d3nset \
d3oafs \
d3oflw \
d3plot \
d3regenr \
d3resetn \
d3rplt \
d3rpsi \
d3rstr \
d3save \
d3stat \
d3term \
d3tsvp \
d3valu \
d3woff \
d3zsta \
stubs

# Additional files used to build serial version of CNS
SEROBJ := \
d3ajw  \
d3allo \
d3bgis \
d3bxvp \
d3cchk \
d3chng \
d3cij  \
d3clst \
d3cond \
d3cnmd \
d3cpli \
d3ctbl \
d3dflt \
d3dij  \
d3dprt \
d3echo \
d3epsi \
d3evtm \
d3fchk \
d3g1g3 \
d3g1imin \
d3g2cell \
d3g2cond \
d3g2conn \
d3g3prob \
d3g3rset \
d3g3timr \
d3gfcl \
d3gfhd \
d3gfrl \
d3gfsh \
d3grp0 \
d3grp1 \
d3grp2 \
d3grp3 \
d3ijpi \
d3ijpl \
d3imgt \
d3kij  \
d3lij  \
d3lime \
d3mark \
d3master \
d3mchk \
d3news \
d3nxt64s \
d3parm \
d3perr \
d3phase \
d3resetz \
d3rlnm \
d3salf \
d3schk \
d3sfhm \
d3sj   \
d3snsa \
d3start \
d3tchk \
d3uprg \
d3vchk \
d3vitp \
d3vjnt \
d3vplt \
d3vtch \
findln \
findmdlt \
findpcf \
findvc \
getthr \
rfdmtx \
tvplot
 
# Additional files used to build PAR0 version of CNS
PAR0OBJ := \
d3ajw  \
d3allo \
d3bgis \
d3bxvp \
d3cchk \
d3chng \
d3clst \
d3cnmd \
d3cpli \
d3ctbl \
d3dflt \
d3echo \
d3epsi \
d3evtm \
d3exch \
d3fchk \
d3g1g3 \
d3g1imin \
d3g2cell \
d3g2cond \
d3g2conn \
d3g3prob \
d3g3rset \
d3g3timr \
d3gfcl \
d3gfhd \
d3gfrl \
d3gfsh \
d3grp0 \
d3grp1 \
d3grp2 \
d3grp3 \
d3gsta \
d3ijpi \
d3ijpz \
d3imgt \
d3lime \
d3mark \
d3master \
d3mchk \
d3news \
d3nxt64p \
d3parm \
d3pdpz \
d3perr \
d3resetz \
d3rlnm \
d3salf \
d3schk \
d3sfhm \
d3snsa \
d3start \
d3tchk \
d3ucvt \
d3uprg \
d3vchk \
d3vitp \
d3vjnt \
d3vplt \
d3vtch \
findcell \
findln \
findmdlt \
findpcf \
findvc \
getthr \
rfdmtx \
tvplot

# Additional files used to build PARn version of CNS
PARnOBJ := \
d3cij  \
d3cond \
d3dij  \
d3exch \
d3gsta \
d3ijpn \
d3kij  \
d3lij  \
d3nxt64p \
d3pdpn \
d3phase \
d3sj   \
d3ucvt \
findcell

# Files currently under development -- not used in any version
# d3pxq.c

# Compile-time and target-dependent definitions
CC := gcc
MPICC := mpicc
CFLAGS := -g
CFLGS := $(CFLAGS) -Wcomment -Wformat -Wimplicit -Wsequence-point \
   -Wreturn-type -Wunused-variable -Wunused-label -Wuninitialized \
   -Wundef -Wcast-qual -Wunreachable-code -pthread -m64
SVNV := $(shell gnrversn)
DDEFS := PCLUX64 _ISOC99_SOURCE BBDS USE_MPI \
	SVNVRS=$(patsubst %,'"%"',$(SVNV))

.SUFFIXES:
.SUFFIXES: .o .c .s .d
.PHONY: all cdate clean
all:	cdate Cns cnsP0 cnsPn

# No .s files currently in use, but this is here for when needed
.s.o:
	as $(CFLAGS) $(DDEFS:%=-D%) $(INC:%=-I%) $*.s; \
	mv $(*F).o $(DIR)

# N.B.  In following LIBS defs, order is NOT arbitrary
# Macros for serial version
DIRs := pclux64
DEFSs = $(DDEFS)
OBJs = $(CNSOBJ) $(SEROBJ)
LIBSs = $(LIB)/libbbds.a $(LIB)/libenv.a $(LIB)/libserplot.a \
$(LIB)/libsermpit.a $(LIB)/libcrk.a
# Macros for PAR0 version
DIR0 := pc64P0
DEFS0 = $(DDEFS) PAR PAR0
OBJ0 = $(CNSOBJ) $(PAR0OBJ)
LIBS0 = $(LIB)/libbbds.a $(LIB)/libenv.a $(LIB)/libplotP0.a \
$(LIB)/libmpitP0.a $(LIB)/libcrk.a
# Macros for PARn version
DIRn := pc64Pn
DEFSn = $(DDEFS) PAR PARn
OBJn = $(CNSOBJ) $(PARnOBJ)
LIBSn = $(LIB)/libplotPn.a $(LIB)/libmpitPn.a $(LIB)/libcrk.a

$(DIRs)/%.o:   %.c $(DIRs)/%.d makefile
	$(CC) $(CFLGS) $(DEFSs:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIRs)

$(DIR0)/%.o:   %.c $(DIR0)/%.d makefile
	$(MPICC) $(CFLGS) $(DEFS0:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIR0)

$(DIRn)/%.o:   %.c $(DIRn)/%.d makefile
	$(MPICC) $(CFLGS) $(DEFSn:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIRn)

# Build the nxdr tables and headers
OHD := d3global celldata clblk ijpldef rpdef repblock celltype \
	conntype inhibblk modby modality misc conduct vbdef tvdef utvdef

d3nxt64s.c d3nxd64s.h:	d3hdrs.c d3objs $(INC)/sysdef.h \
		$(INC)/mpitools.h $(INC)/memshare.h $(INC)/mempools.h \
		$(INC)/collect.h $(INC)/swap.h $(OHD:%=%.h) \
		$(LCLHOME)/bin/nxdr2
	nxdr2 -m64 -b d3objs -f d3hdrs.c -o d3nxt64s.c -h d3nxd64s.h \
		-c "$(CC) -DPCLINUX -DUSE_MPI -DBBDS -I$(INC)"

d3nxt64p.c d3nxd64p.h:	d3hdrs.c d3objs $(INC)/sysdef.h \
		$(INC)/mpitools.h $(INC)/memshare.h $(INC)/mempools.h \
		$(INC)/collect.h $(INC)/swap.h $(OHD:%=%.h) \
		$(LCLHOME)/bin/nxdr2
	nxdr2 -m64 -b d3objs -f d3hdrs.c -o d3nxt64p.c -h d3nxd64p.h \
		-c "$(MPICC) -DPCLINUX -DUSE_MPI -DBBDS -DPAR -I$(INC)"

# Generate dependencies via gcc mechanism
$(DIRs)/%.d:	%.c d3nxd64s.h
	$(SHELL) -ec '$(CC) -MM $(DEFSs:%=-D%) $(INC:%=-I%) $< \
   | cat dirfile - > $@'

$(DIR0)/%.d:	%.c d3nxd64p.h
	$(SHELL) -ec '$(MPICC) -MM $(DEFS0:%=-D%) $(INC:%=-I%) $< \
   | cat dirP0 - > $@'

$(DIRn)/%.d:	%.c d3nxd64p.h
	$(SHELL) -ec '$(MPICC) -MM $(DEFSn:%=-D%) $(INC:%=-I%) $< \
   | cat dirPn - > $@'

# Put it all together

# Compile cdate every time the make file is run
cdate:
	$(CC) $(CFLGS) -DPCLUX64 $(INC:%=-I%) -o $(DIRs)/compdate.o -c compdate.c

# Build Cns------------------------------------------------
Cns:	$(OBJs:%=$(DIRs)/%.o) $(DIRs)/d3nxt64s.o d3nxd64s.h \
		$(LIBSs) makefile
	@echo ===Building Cns
	$(CC) $(CFLGS) $(OBJs:%=$(DIRs)/%.o) $(DIRs)/compdate.o \
		$(LIBSs) -lm -ldl -o $@; \
	cp $@ $(LCLHOME)/bin/Cns;

# Build cnsP0----------------------------------------------
cnsP0:	$(OBJ0:%=$(DIR0)/%.o) $(DIR0)/d3nxt64p.o d3nxd64p.h \
		$(LIBS0) makefile
	@echo ===Building cnsP0
	$(MPICC) $(CFLGS) $(OBJ0:%=$(DIR0)/%.o) $(DIRs)/compdate.o \
		$(LIBS0) -lm -ldl -o $@; \
	cp $@ $(LCLHOME)/bin/cnsP0;

# Build cnsPn----------------------------------------------
cnsPn:	$(OBJn:%=$(DIRn)/%.o) $(DIRn)/d3nxt64p.o d3nxd64p.h \
		$(LIBSn) makefile
	@echo ===Building cnsPn
	$(MPICC) $(CFLGS) $(OBJn:%=$(DIRn)/%.o) $(DIRs)/compdate.o \
		$(LIBSn) -lm -ldl -o $@; \
	cp $@ $(LCLHOME)/bin/cnsPn;

# The usual cleanup target
clean:
	rm -f Cns cnsP0 cnsPn
	rm -f $(LCLHOME)/bin/Cns $(LCLHOME)/bin/cnsP0 $(LCLHOME)/bin/cnsPn
	rm -f $(DIRs)/*.[od] $(DIR0)/*.[od] $(DIRn)/*.[od]

# I am using the serial dependency files for all three targets,
# as otherwise it is necessary to have separate makefile for
# each of them.  Hopefully the only consequence will be
# incorrect inclusion of some rocks headers in PARn builds.
-include $(OBJs:%=$(DIRs)/%.d)
-include $(OBJ0:%=$(DIR0)/%.d)
-include $(OBJn:%=$(DIRn)/%.d)
