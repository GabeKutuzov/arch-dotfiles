;	init.cmd -	C80 simulator command file for the S/IP80 SBus TMX320C80 board
;
;   Initial Date:   02/04/95    BV
;	Revised:		02/21/95	BV
;
;	Notes:

; *** Internal Hardware defininitions ***
; Lower block of RAM0,1 for PP0-3
ma 0x00000000,0x4000,ram
;
; Individual RAM2 blocks for PP0-3
ma 0x00008000,0x800,ram
ma 0x00009000,0x800,ram
ma 0x0000a000,0x800,ram
ma 0x0000b000,0x800,ram
;
; Individual PRAM blocks for PP0-3
ma 0x01000000,0x800,ram
ma 0x01001000,0x800,ram
ma 0x01002000,0x800,ram
ma 0x01003000,0x800,ram
;
; PRAM block for the MP (depending on which simulator is initializing)
if $$MVP_MP$$
ma 0x01010000,0x800,ram
endif
if $$MVP_PP$$
ma 0x01010000,0x800,rom
endif
;
; TC and VC Registers respectively
; Note:  These were causing problems in the simulator startup
;if $$MVP_MP$$
;ma 0x01820000,0x200,ram
;ma 0x01820000,0x200,ram
;endif
;
; *** External Hardware defininitions ***
; SBus and LCA Registers
ma 0x21200000,0x4,sram3,0,8
ma 0x21400000,0xD,sram3,0,8
;
; DRAM Memory Array
ma 0x21600000,0x200000,dram2,2,64
