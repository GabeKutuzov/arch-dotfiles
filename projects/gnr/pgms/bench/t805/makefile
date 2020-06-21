#######################################################################
# MAKE file for d3bench on a single T8 transputer
#######################################################################

.SUFFIXES: .c .tco .btl .btr .cfb .cfs .lku .lbb .rsc

#compile .c file into .tco (object) file

TYPE = 805

FILES = \
d3bench

.KEEP_STATE:

%.tco:    ../%.c
	$(D4314A)/optimize/tools/icc -O2 -QT -T$(TYPE) -DT$(TYPE) $< 

%.btl:    %.lku
	icollect -t $<

d3bench.btl: d3bench.lku 

d3bench.lku: $(FILES:%=%.tco)
	ilink -T$(TYPE) $(FILES:%=%.tco) -f cnonconf.lnk 
