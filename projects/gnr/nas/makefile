########################################################################
#                          Makefile for nas                            #
#                                                                      #
# V1A, 10/30/09, GNR - Configure to compile Jack LLoyd nas w/gcc       #
########################################################################

# Path for object files
DIR := pclinux
CC := gcc
CXXC := g++
CFLAGS := -g -Wall
CXXFLAGS := -g -Wall
DEFS := PCLINUX

COBJS := parser.tab
CPPOBJS := nas

all:	nas

# Generate dependencies via gcc mechanism
$(DIR)/%.d:	%.c
	$(SHELL) -ec '$(CC) -MM $(DEFS:%=-D%) $(CPPFLAGS) -I. $*.c \
	| sed '\''s/^\(.*\)\.o[ :]*/$(DIR)\/\1.o $(DIR)\/\1.d : /g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

$(DIR)/%.d:	%.cpp
	$(SHELL) -ec '$(CXXC) -MM $(DEFS:%=-D%) $(CPPFLAGS) -I. $*.cpp \
	| sed '\''s/^\(.*\)\.o[ :]*/$(DIR)\/\1.o $(DIR)\/\1.d : /g'\'' > $@; \
	[ -s $@ ] || rm -f $@'

$(DIR)/%.o:	%.c $(DIR)/%.d makefile
	$(CC) $(CFLAGS) $(DEFS:%=-D%) -I. -c $*.c
	mv $*.o $(DIR)
	
$(DIR)/%.o:	%.cpp $(DIR)/%.d makefile
	$(CXXC) $(CFLAGS) $(DEFS:%=-D%) -I. -c $*.cpp
	mv $*.o $(DIR)

parser.tab.c:	parser.y
	bison -d -k -l parser.y

nas: 	$(DIR)/$(COBJS:%=%.o) $(DIR)/$(CPPOBJS:%=%.o)
	$(CXXC) $(DIR)/$(COBJS:%=%.o) $(DIR)/$(CPPOBJS:%=%.o) -o nas
	cp nas ~/bin

# Clean target
.PHONY : clean
clean:
	rm -f  core* *~ *.o $(DIR)/*.o nas
	@echo 'removed all core, *~, object, and executable files'
