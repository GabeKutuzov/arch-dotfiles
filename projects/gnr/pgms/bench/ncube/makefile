#######################################################################
# MAKE file for d3bench on a single ncube node
#######################################################################

OBJ = d3bench.on  

#  Ncube system libraries
G   = /usr/lib/ncube/lib/vrtxc.lib

# some useful directories
#I   = /home/nsi/include/ncube

.SUFFIXES:
.SUFFIXES: .on .sn .c


#Compile for node
CFLAGS = -DNN -kNN -g -W2 

%.on: ../%.c
	xucc -c $(CFLAGS) -I$I $<

d3bench.n:	$(OBJ) 
		xucc  $(CFLAGS) $(OBJ) $(G)
		xlcn -t 64 -e d3bench.n -ns d3bench.n.sym cn.out

