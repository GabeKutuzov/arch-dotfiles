# $Id: makefile 8 2013-04-19 18:34:31Z  $
########################################################################
#            Make file for BBD mex-function client library             #
#                        Linux 32-bit version                          #
#                                                                      #
# V1A, 10/29/10, GNR - New makefile                                    #
# ==>, 11/04/10, GNR - Last mod before committing to svn repository    #
# Rev, 11/09/10, GNR - Generate from maklux64 version                  #
# Rev, 09/24/12, GNR - Change MatlabR2010b to MatlabR2012b             #
# Rev, 04/18/13, GNR - Make bbdcquit a separate object file            #
########################################################################

BINDIR	:= $(HOME)/bin
MEXDIR	:= $(HOME)/matlab
LIBDIR	:= $(HOME)/lib
INC1	:= $(HOME)/include
INC2	:= /opt/MatlabR2012b/extern/include
INC     := $(INC1) $(INC2)

# Linux common pathnames
DIR := pclinux
LL  := $(LIBDIR)/$(DIR)

CFILES = \
abexit   \
bbdcminp \
bbdcmlog \
bbdcquit

MEXFILES = \
bbdcrege \
bbdcregs \
bbdcrsid \
bbdcregt \
bbdcrtid \
bbdcregv \
bbdcchk  \
bbdcevnt \
bbdcexsi \
bbdcgete \
bbdcgetm \
bbdcputs \
bbdcputt \
bbdcputv

.SUFFIXES:
.SUFFIXES: .d .c .s .o .h .mexglx
.PHONY : bbdmex bbdxinit bbdxclse bbdxminp bbdxmlog depend clean

# Target dependent macro definitions:
CC := gcc
MEXC := mex
CFLAGS := -O2 -Wunused -m32
SVNV := $(shell gnrversn)
DEFS := PCLINUX _ISOC99_SOURCE SVNVRS=$(patsubst %,'"%"',$(SVNV))

# Generate dependencies via gcc mechanism
$(DIR)/%.d:	%.c
	$(SHELL) -ec '$(CC) -MM $(DEFS:%=-D%) $(CPPFLAGS) $(INC:%=-I%) $< \
	| sed '\''s/^\(.*\)\.o[ :]*/$(DIR)\/\1.o $(DIR)\/\1.d : /g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

#Compile for Linux
$(DIR)/%.o:	%.c $(DIR)/%.d makefile
	$(CC) $(CFLAGS) $(DEFS:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIR)

#Compile a mex file
$(MEXDIR)/%.mexglx:	%.c $(DIR)/abexit.o $(LL)/libcrkpic.a $(INC1)/bbd.h makefile
	$(MEXC) $(DEFS:%=-D%) -I$(INC1) -g -outdir $(MEXDIR) $< $(DIR)/abexit.o \
		$(LL)/libcrkpic.a -lrt

#Make all this stuff
# N.B.  bbbcminp and bbdcmlog have same names in 32 and 64-bit versions
all: depend bbdxinit bbdxclse bbdmex bbdxminp bbdxmlog

bbdmex:		$(MEXFILES:%=$(MEXDIR)/%.mexglx)
bbdxinit:	$(MEXDIR)/bbdcinit.mexglx
bbdxclse:	$(MEXDIR)/bbdcclse.mexglx
bbdxminp:	$(BINDIR)/bbdcminp
bbdxmlog:	$(BINDIR)/bbdcmlog
depend:		$(CFILES:%=$(DIR)/%.d)

$(BINDIR)/bbdcminp:	$(DIR)/bbdcminp.o makefile
		$(CC) $(CFLAGS) $(DIR)/bbdcminp.o $(LL)/libcrk.a -lm -lrt -o $@

$(BINDIR)/bbdcmlog:	$(DIR)/bbdcmlog.o makefile
		$(CC) $(CFLAGS) $(DIR)/bbdcmlog.o $(LL)/libcrk.a -lm -lrt -o $@

$(MEXDIR)/bbdcinit.mexglx: bbdcinit.c $(DIR)/abexit.o $(DIR)/bbdcquit.o \
		$(LL)/libcrkpic.a $(INC1)/bbd.h makefile
	mex $(DEFS:%=-D%) -I$(INC1) -g -outdir $(MEXDIR) $< $(DIR)/abexit.o \
		$(DIR)/bbdcquit.o $(LL)/libcrkpic.a -lrt

$(MEXDIR)/bbdcclse.mexglx: bbdcclse.c $(DIR)/abexit.o $(DIR)/bbdcquit.o \
		$(LL)/libcrkpic.a $(INC1)/bbd.h makefile
	mex $(DEFS:%=-D%) -I$(INC1) -g -outdir $(MEXDIR) $< $(DIR)/abexit.o \
		$(DIR)/bbdcquit.o $(LL)/libcrkpic.a -lrt

#Compile abexit.o and bbdcquit.o for inclusion in mexa64 files
$(DIR)/abexit.o:	abexit.c $(DIR)/abexit.d  makefile
	$(CC) $(CFLAGS) -fPIC $(DEFS:%=-D%) -DMATLAB_MEX_FILE \
		$(INC:%=-I%) -c $<
	mv abexit.o $(DIR)

$(DIR)/bbdcquit.o:	bbdcquit.c $(DIR)/bbdcquit.d  makefile
	$(CC) $(CFLAGS) -fPIC $(DEFS:%=-D%) -DMATLAB_MEX_FILE \
		$(INC:%=-I%) -c $<
	mv bbdcquit.o $(DIR)

#Remove all the products of this make
clean:
	rm -f $(DIR)/*.o $(BINDIR)/bbcdmlog $(BINDIR)/bbdcminp \
		$(MEXDIR)/$(MEXFILES:%=%.mexglx)

-include $(CFILES:%=$(DIR)/%.d)
