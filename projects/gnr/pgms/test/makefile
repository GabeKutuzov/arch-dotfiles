########################################################################
#                 Makefile for various test programs                   #
#                                                                      #
#  Rev, 08/11/97, GNR - Merge a bunch of little makefiles into one     #
########################################################################

# WARNING:  The various targets have not all been tested since all
# the little individual makefiles were consolidated.  -GNR

.SUFFIXES: .o .c .s .on .sn

#Some useful directories
I   = /home/cdr/nsitest/include
SL  = /home/cdr/nsitest/lib/sparc
D3  = /home/cdr/d3

#Assemble for node from Sun (xasn is a shell script)

CC = acc

CFLAGS = -Xc -g -DSUN4 -I$I

#################### colorbar ####################

colorbar: colorbar.on 
	xucc -DNN -kNN -g colorbar.on \
		/home/nsi/ncube/lib/crk.lib \
		/usr/lib/ncube/lib/graphcn.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -e colorbar.n -ns colorbar.n.sym cn.out
	@rm -f cn.out
	@cp /usr/lib/ncube/usr/bin/nrun colorbar

colorbar.on: colorbar.c ncplots.h

#################### datetest ####################

datetest: datetest.on
	xucc -DNN -kNN -g datetest.on \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e datetest.n -ns datetest.n.sym cn.out
	@rm -f cn.out datetest
	@ln -s /usr/local/bin/nrun datetest

datetest.on:	datetest.c

#################### fgtest ####################

fg:     fgmain.on
	xucc -kNN -g fgmain.on \
		/usr/lib/ncube/lib/vrtxc.lib 
	xlcn -t 32 -e fg.n -ns fg.n.sym cn.out
	@rm -f cn.out
	@cp /usr/lib/ncube/usr/bin/nrun fg

fgmain.on: fgmain.c

#################### gsynchtest ####################

gsynchtest:	gsynchtest.on
	xucc -DNN -kNN -g gsynchtest.on \
		$(NL)/nctools.lib $(NL)/crk.lib \
		/usr/lib/ncube/lib/graphcn.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e gsynchtest.n -ns gsynchtest.n.sym cn.out
	@rm -f cn.out
	@cp /usr/lib/ncube/usr/bin/nrun gsynchtest

gsynchtest.on:	gsynchtest.c $I/sysdef.h $I/glu.h

#################### iotest ####################

iotest:	iotest.on ioget.on ioput.on
	xucc -DNN -kNN -g iotest.on ioget.on ioput.on $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e iotest.n -ns iotest.n.sym cn.out
	@rm -f cn.out iotest
	@ln -s /usr/local/bin/nrun iotest

iotest.on:	iotest.c iostr.h
ioget.on:	ioget.c  iostr.h
ioput.on:	ioput.c  iostr.h

#################### nntest ####################

nntest:	nntest.on nnget.on nnput.on
	xucc -DNN -kNN -g nntest.on nnget.on nnput.on $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e nntest.n -ns nntest.n.sym cn.out
	@rm -f cn.out nntest
	@ln -s /usr/local/bin/nrun nntest

nntest.on:	nntest.c
nnget.on:	nnget.c
nnput.on:	nnput.c

#################### para ####################

para:	para.o 
	acc para.o $(SL)/libserplot.a $(SL)/libcrk.a -o para 

para.o:		para.c $I/sysdef.h $I/plots.h

#################### rftest ####################

RFOBJ = rftest.on rfallo.on rfopen.on rfwrite.on rfclose.on rfread.on \
	rfseek.on

rftest:	$(RFOBJ)
	xucc -DNN -kNN -g $(RFOBJ) $(NL)/crk.lib $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e rftest.n -ns rftest.n.sym cn.out
	@rm -f cn.out rftest
	@ln -s /usr/local/bin/nrun rftest

rftest.on:	rftest.c rfdef.h
rfallo.on:	rfallo.c rfdef.h
rfopen.on:	rfopen.c rfdef.h
rfwrite.on:	rfwrite.c rfdef.h
rfclose.on:	rfclose.c rfdef.h
rfread.on:	rfread.c rfdef.h
rfseek.on:	rfseek.c rfdef.h

#################### rstcnvrt ####################

rstcnvrt: rstcnvrt.on
	xucc -DNN -kNN -g rstcnvrt.on $(NL)/crk.lib $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e rstcnvrt.n -ns rstcnvrt.n.sym cn.out
	@rm -f cn.out rstcnvrt
	@ln -s /usr/local/bin/nrun rstcnvrt

rstcnvrt.on: rstcnvrt.c $I/sysdef.h $I/rocks.h \
	$(D3)/d3global.h savblk.h newsvblk.h

#################### rstdump ####################

rstdump: rstdump.on
	xucc -DNN -kNN -g rstdump.on $(NL)/crk.lib $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e rstdump.n -ns rstdump.n.sym cn.out
	@rm -f cn.out rstdump
	@ln -s /usr/local/bin/nrun rstdump

rstdump.on: rstdump.c $I/sysdef.h $I/rocks.h $I/rfdef.h \
	$(D3)/savblk.h

#################### rstoldmp ####################

rstoldmp: rstoldmp.on
	xucc -DNN -kNN -g rstoldmp.on $(NL)/crk.lib $(NL)/nctools.lib \
		/usr/lib/ncube/lib/vrtxc.lib
	xlcn -t 32 -e rstoldmp.n -ns rstoldmp.n.sym cn.out
	@rm -f cn.out rstoldmp
	@ln -s /usr/local/bin/nrun rstoldmp

rstoldmp.on: rstoldmp.c $I/sysdef.h $I/rocks.h $I/rfdef.h

#################### suniotest ####################

SIOBJ = suniotest.o sunioget.o sunioput.o second.o

suniotest:	
	acc $(SIOBJ) -o suniotest

suniotest.o:	suniotest.c	$I/sysdef.h suniostr.h
sunioget.o:	sunioget.c	suniostr.h
sunioput.o:	sunioput.c	suniostr.h
second.o:	second.c	$I/sysdef.h

#################### swaptest ####################

swaptest:	swaptest.o _swapr4.o _swapr8.o
	acc -g _swapr4.o _swapr8.o swaptest.o -o swaptest
