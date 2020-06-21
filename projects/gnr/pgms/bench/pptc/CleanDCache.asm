;Example 11-17 illustrates a program that writes back modified
;data cache subblocks to external memory to preserve the data
;cache coherency. Note that the cmnd instruction's DCR bit resets
;flags but does not execute a writeback of data.
;Refer to Figure 3-5, MP Cache LRU Register, and Figure 3-6,
;MP Cache Tag Registers, for additional details.
;Example 11-17.
;
;
;Data-Cache Clean Sample Code
;
	.global	_CleanDCache
crNum	.set	r2		; control register number for
				;  DTAGn, n=0,1, ... ,15
setNum	.set	r3		; set number 0, 1, 2, or 3
blkNum	.set	r4		; block number 0, 1, 2, or 3
tag	.set	r5		; tag register value for current
				;  block
dirty	.set	r6		; dirty bits for current block
_CleanDCache:
	or	0x400,r0,crNum	; CR number for DTAG0
	or	r0,r0,setNum	; initial set number = 0
SetLoop:
	or	r0,r0,blkNum	; initial block number = 0
BlockLoop:	
	rdcr	crNum,tag	; read next DTAG register
	and	0x154,tag,dirty	; isolate dirty bits
	bcnd	BlockClean,dirty,eq0.w	; jump if block is entirely
				;  clean
	addu	1,blkNum,blkNum	; increment block number
CleanSub0:
	bbz	CleanSub1,dirty,2	; jump if subblock 0 is clean
	sl.im	8,10,setNum,tag	; add set to tag address
	dcachec	0x00(tag)	; clean dirty subblock 0
CleanSub1:
	bbz.a	CleanSub2,dirty,4	; jump if subblock 1 is clean
	dcachec	0x40(tag)	; clean dirty subblock 1
CleanSub2:
	bbz.a	CleanSub3,dirty,6	; jump if subblock 2 is clean
	dcachec	0x80(tag)	; clean dirty subblock 2
CleanSub3:
	bbz.a	BlockClean,dirty,8	; jump if subblock 3 is clean
	dcachec	0xc0(tag)	; clean dirty subblock 3
BlockClean:
	bbz	BlockLoop,blkNum,2	; loop again if block < 4
	addu	1,crNum,crNum	; point to next DTAG register

	bbz	SetLoop,setNum,2	; loop again if set < 4
	addu	1,setNum,setNum	; increment set number

	jsr.a	r31(r0),r0	; return from subroutine
