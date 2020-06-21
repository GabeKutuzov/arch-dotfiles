#	Makefile for uKernel system support functions
#
#	Initial Date:	12/01/94, BV
#	Revision Date:	12/04/94, BV
#			01/04/95, BV
#			01/24/95, BV
#			02/21/95, BV
#
#	Notes:
#			-Transit to development of static-Kernel, etc.
#			-Changed to a flat structure (i.e. no more sys, sys/include)
#			-Transit to development of new communication model
#			-Added sim target for easier simulator compilation
#			-This makefile now makes MVP user applications
#
#			-Get "make depend" to work with this whole thing.
#			-Add "make install" once 0.3 is ready

# IMPORTANT NOTE:
# the simulator cannot simulate printfs. these are ifdefed out with
# #ifdef SIM in the code. When compiling use this command:
# make sim CDEBUGFLAGS = -DSIM    until the Makefile is fixed

# Initial Setups:
MVP_CC = mpcl
MVP_LD = mvplnk
MVP_PP = ppcl
INC = $(C80_HOME)/include
LIB = $(C80_HOME)/lib

CPPFLAGS =
# z is link, mz don't unroll loops, g debug
#CCFLAGS = -I$(INC) -mz -dSIM -g
CCFLAGS = -I$(INC) -mz -g
#CCFLAGS = -I$(INC) -o3 -pk -q
DEFINES +=  $(CDEBUGFLAGS)

#LDFLAGS = -x -c -heap 1048576 -pstack 1024 $(INC)/sip80.lnk 
LDFLAGS = -x -c  $(INC)/sip80.lnk 
LIBS = -l mp_int.lib -l mp_cio.lib -l mp_task.lib -l mp_rts.lib -l mp_ppcmd.lib -l mp_timer.lib -l ppcmd.lib

KERNOBJ = sip80knl.obj
KERNMAP = sip80knl.map
SIMOBJ = sim80knl.obj
SIMMAP = sim80knl.map

TEST = bserv_mp.obj  test_pp.o
TARGET = bserv_mp

all:	$(TEST)
	$(MVP_LD) -ar -o $(TARGET).out $(LIB)/$(KERNOBJ) $(TEST) -m  $(TARGET).map $(LDFLAGS) $(LIBS)
	sync

sim:	$(TEST)
	$(MVP_LD) -ar -o $(TARGET)S.out $(LIB)/$(SIMOBJ) $(TEST) -m $(TARGET)S.map $(LDFLAGS) $(LIBS)
	sync

clean:
	rm -f *.obj *.o *.map *.out *%
	sync

# Basic suffixes definitions for every makefile
.SUFFIXES: .c .asm .obj .s .o
.asm.obj:
	$(MVP_CC) -c $(CPPFLAGS) $(CCFLAGS) -DMVP -DANSI $<
.c.obj:
	$(MVP_CC) -c $(CPPFLAGS) $(CCFLAGS) $(DEFINES) -DMVP -DANSI -DTI_C_110 $<
.c.o:
	$(MVP_PP) -c $(CCFLAGS) -eoo -eas -DADSP -DANSI -DTI_C_110 $<
.s.o:
	$(MVP_PP) -c $(CCFLAGS) -eoo -eas -DADSP -DANSI $<
