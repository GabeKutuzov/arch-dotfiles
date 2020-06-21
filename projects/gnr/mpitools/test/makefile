# PCLINUX MAKE file for mpitools test programs
# V1A, 03/12/00, GNR
# Rev, 07/01/16, GNR - Add isumtest, modify for 64-bit openmpi
# Rev, 08/12/16, GNR - Add rkgtest
# Rev, 08/31/16, GNR - Add copptest, 09/02/16, make it PAR0,PARn
# Rev, 01/29/17, GNR - Add gathtest
# Rev, 05/23/17, GNR - Remove rkgtest, now in src/oldplot
# Rev, 09/02/18, GNR - Use mpicc to avoid giving mpi.h path

# Define directories where include files are stored
INC := $(HOME)/include

# Define directory where ROCKS libraries are stored
LIB=$(HOME)/lib/pclux64

CC := mpicc
CFLAGS := -g
# Note -Wformat removed from our usual set for rkprintf in copptest
CFLGS := $(CFLAGS) -Wcomment -Wimplicit -Wsequence-point \
   -Wreturn-type -Wunused-variable -Wunused-label -Wuninitialized \
   -Wundef -Wcast-qual -Wunreachable-code -pthread -m64
SVNV := $(shell gnrversn)
DDEFS := PCLUX64 _ISOC99_SOURCE USE_MPI SVNVRS=$(patsubst %,'"%"',$(SVNV))

.SUFFIXES:
.SUFFIXES: .c .o
.PHONY: all mtest gtest ctest clean

# Default will be to make all tests
all:	isumtest mtest ctest gathtest
mtest:	Memtest memtestP0 memtestPn
ctest:	copptestP0 copptestPn

# Basic	compilation rules
.c.o:
	$(CC) -c -g -DPCLUX64 -DUSE_MPI $(INC:%=-I%) $<

DIRs = pclux64
DEFSs = $(DDEFS)
DIR0 = pc64P0
DEFS0 = $(DDEFS) PAR PAR0
DIRn = pc64Pn
DEFSn = $(DDEFS) PAR PARn

$(DIRs)/%.o:   %.c $(DIRs)/%.d makefile
	$(CC) $(CFLGS) $(DEFSs:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIRs)

$(DIR0)/%.o:   %.c $(DIR0)/%.d makefile
	$(CC) $(CFLGS) $(DEFS0:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIR0)

$(DIRn)/%.o:   %.c $(DIRn)/%.d makefile
	$(CC) $(CFLGS) $(DEFSn:%=-D%) $(INC:%=-I%) -c $<
	mv $*.o $(DIRn)

mtnxdr.c mtnxdr.h:	memthdrs.c memtest.h memtobjs $(INC)/sysdef.h \
		$(INC)/mpitools.h $(INC)/memshare.h $(INC)/mempools.h \
		$(INC)/swap.h $(HOME)/bin/nxdr2
	nxdr2 -m64 -b memtobjs -f memthdrs.c -o mtnxdr.c -h mtnxdr.h \
		-c "$(CC) -DPCLINUX -DUSE_MPI -I$(INC)"

ctnxdr.c ctnxdr.h:	copthdrs.c copptest.h coptobjs $(INC)/sysdef.h \
		$(INC)/mpitools.h $(INC)/memshare.h $(INC)/mempools.h \
		$(INC)/swap.h $(HOME)/bin/nxdr2
	nxdr2 -m64 -b coptobjs -f copthdrs.c -o ctnxdr.c -h ctnxdr.h \
		-c "$(CC) -DPCLINUX -DUSE_MPI -I$(INC)" -t CPT

# Generate dependencies via $(CC) mechanism
$(DIRs)/%.d:	%.c mtnxdr.h ctnxdr.h
	$(SHELL) -ec '$(CC) -MM $(DEFS:%=-D%) $(INC:%=-I%) $< \
   | cat dirfile - > $@'

$(DIR0)/%.d:	%.c mtnxdr.h
	$(SHELL) -ec '$(CC) -MM $(DEFS:%=-D%) $(INC:%=-I%) $< \
   | cat dirP0 - > $@'

$(DIRn)/%.d:	%.c mtnxdr.h
	$(SHELL) -ec '$(CC) -MM $(DEFS:%=-D%) $(INC:%=-I%) $< \
   | cat dirPn - > $@'

# Build isumtest-------------------------------------------
isumtest: isumtest.o $(LIB)/libmpitP0.a $(LIB)/libcrk.a ../pc64P0/abexit.o
	mpicc -g -o isumtest isumtest.o ../pc64P0/abexit.o $(LIB)/libmpitP0.a \
   	$(LIB)/libcrk.a

# Build gathtest-------------------------------------------
gathtest: gathtest.o $(LIB)/libmpitP0.a $(LIB)/libcrk.a ../pc64P0/abexit.o
	mpicc -g -o gathtest gathtest.o ../pc64P0/abexit.o $(LIB)/libmpitP0.a \
   	$(LIB)/libcrk.a

# Built Memtest--------------------------------------------
Memtest:	$(DIRs)/memtest.o $(DIRs)/mtnxdr.o mtnxdr.h \
		$(LIB)/libsermpit.a $(LIB)/libcrk.a
	$(CC) -g -o Memtest $(DIRs)/memtest.o $(DIRs)/mtnxdr.o \
   	$(LIB)/libsermpit.a $(LIB)/libcrk.a -lm

# Built memtestP0------------------------------------------
memtestP0:	$(DIR0)/memtest.o $(DIR0)/mtnxdr.o mtnxdr.h \
		$(LIB)/libmpitP0.a $(LIB)/libcrk.a
	mpicc -g -o memtestP0 $(DIR0)/memtest.o $(DIR0)/mtnxdr.o \
   	$(LIB)/libmpitP0.a $(LIB)/libcrk.a -lm

# Built memtestPn------------------------------------------
memtestPn:	$(DIRn)/memtest.o $(DIRn)/mtnxdr.o mtnxdr.h \
		$(LIB)/libmpitPn.a $(LIB)/libcrk.a
	mpicc -g -o memtestPn $(DIRn)/memtest.o $(DIRn)/mtnxdr.o \
   	$(LIB)/libmpitPn.a $(LIB)/libcrk.a -lm

# Built Rkgtest--------------------------------------------
Rkgtest:	$(DIRs)/rkgtest.o $(LIB)/libserplot.a \
		$(LIB)/libsermpit.a $(LIB)/libcrk.a
	$(CC) -g -o Rkgtest $(DIRs)/rkgtest.o $(LIB)/libserplot.a \
   	$(LIB)/libsermpit.a $(LIB)/libcrk.a -lm

# Built rkgtestP0------------------------------------------
rkgtestP0:	$(DIR0)/rkgtest.o $(LIB)/libplotP0.a \
		$(LIB)/libmpitP0.a $(LIB)/libcrk.a
	mpicc -g -o rkgtestP0 $(DIR0)/rkgtest.o $(LIB)/libplotP0.a \
   	$(LIB)/libmpitP0.a $(LIB)/libcrk.a -lm

# Built rkgtestPn------------------------------------------
rkgtestPn:	$(DIRn)/rkgtest.o $(LIB)/libplotPn.a \
		$(LIB)/libmpitPn.a $(LIB)/libcrk.a
	mpicc -g -o rkgtestPn $(DIRn)/rkgtest.o $(LIB)/libplotPn.a \
   	$(LIB)/libmpitPn.a $(LIB)/libcrk.a -lm

# Build copptestP0-----------------------------------------
copptestP0: $(DIR0)/copptest.o ctnxdr.o $(LIB)/libmpitP0.a \
		$(LIB)/librkpf.a $(LIB)/libcrk.a
	mpicc -g -o copptestP0 $(DIR0)/copptest.o ctnxdr.o \
		$(LIB)/libmpitP0.a $(LIB)/librkpf.a $(LIB)/libcrk.a

# Build copptestPn-----------------------------------------
copptestPn: $(DIRn)/copptest.o ctnxdr.o $(LIB)/libmpitPn.a \
		$(LIB)/libcrk.a
	mpicc -g -o copptestPn $(DIRn)/copptest.o ctnxdr.o \
		$(LIB)/libmpitPn.a $(LIB)/libcrk.a

# The usual cleanup target
clean:
	rm Memtest memtestP0 memtestPn
	rm Rkgtest rkgtestP0 rkgtestPn
	rm copptestP0 copptestPn
	rm pclux64/*.[od] pc64P0/*.[od] pc64Pn/*.[od]

