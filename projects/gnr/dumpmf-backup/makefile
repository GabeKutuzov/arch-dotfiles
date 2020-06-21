#/* (c) Copyright 20xx, The Rockefeller University *nnnnn* */
########################################################################
#                Make file for dumpmf program, Version 1               #
#                     Version for 64-bit PC Linux                      #
#                                                                      #
# Version 1, 12/26/18, GMK                                             #
########################################################################

# Directory where this stuff is stored
LCLHOME ?= $(HOME)
# Define directory where include files are stored
INC = $(LCLHOME)/include
# Define directory where libraries are stored
RL = $(LCLHOME)/lib/pclux64
#Define name of program to be created
PROG = dumpmf
# Object files to be compiled
OBJFILES = \
dumpmf    \
readbits  \
dumpbits

#compiler flags
CC = gcc
CFLAGS := -m64 -g3 -Wall
DEFS := PCLUX64 _ISOC99_SOURCE USE_MPI SVNVRS=$(patsubst %,'"%"',$(SVNV))

.PHONY: all clean
all: $(PROG)

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) $(DEFS:%=-D%) -I$(INC) -o $(PROG) \
		$(OBJFILES:%=%.c) -L$(RL) -lcrk -lm

clean:
	$(RM) $(PROG)

-include $(OBJFILES:%=$(DIR)/%.d)
