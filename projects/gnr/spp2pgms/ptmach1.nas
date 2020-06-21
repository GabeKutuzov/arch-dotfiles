;======================================================================;
;                             ptmach1.nas                              ;
;                                                                      ;
; This is the NAS program to run the "Rock Vision" region of the TW    ;
; game as a demo PT machine.  It also incorporates the code for the    ;
; basic Izhikevich neuron model.  It is intended to work in conjunc-   ;
; tion with versions of the NeuroSynthesizer and UserTools that have   ;
; been modified to work with these tests.  The Izhikevich neuron       ;
; test is not intended to represent any particular neuronal system.    ;
;                                                                      ;
; The "Rock Vision" region comprises 36 x 40 (total 1440) cells.       ;
; These will be divided evenly across the 3 SPP2 FPGA chips.  Each     ;
; cell will have 3 sensor inputs, corresponding to the red, green,     ;
; and blue values at one pixel on the virtual video camera in the      ;
; game, and one value input that will modulate amplification.          ;
; There are no recurrent connections.  Parameters supplied by          ;
; NeuroSynthesizer will be those needed to emulate the network         ;
; evaluation and amplification previously done in CNS.                 ;
;                                                                      ;
; For the Izhikevich model, the NeuroSyntheizer will place one         ;
; celltype comprising 1000 cells on each of the three SPP2 chips.      ;
; Each cell will have 4 inputs from other cells:  two from its own     ;
; chip and one from each of the other chips, plus one external         ;
; ("sensor") input representing the current probe.  Parameters a,b,    ;
; c,d will be variable across cells in order to enable checking that   ;
; different results come from different cells.  Variables v and u of   ;
; the model will be kept to 16-bit precision internally (range +/-     ;
; 128 mV S8).  Due to the fact that the SPP2 can exchange only 8-bit   ;
; activity values between cells, only the high-order 8 bits (integer   ;
; mV) of the activity will be exchanged.                               ;
;                                                                      ;
; Code assumes an assembler that inserts NOPs only where needed.  Code ;
; also assumes IMM = 0 throughout, i.e. if changed, set it back to 0.  ;
; Note that there is no unsigned arithmetic in the SPP2, therefore,    ;
; positive values (e.g. omega2) must still contain a zero sign bit.    ;
; Much of the multiprecision arithmetic has been written in order to   ;
; explore the capabilities and limitations of the Assembler language.  ;
; In a real program, less precise versions would probably be used to   ;
; get greater speed.                                                   ;
;======================================================================;
; V1A, 10/26/09, GNR - New program                                     ;
; V1B, 11/13/09, GNR - Add code for Rock Vision repertoire             ;
;======================================================================;

;----------------------------------------------------------------------;
;                            Memory layout                             ;
;                                                                      ;
; Each comment begins with the host (SDRAM) byte location of the item. ;
; An asterisk indicates that the item location is fixed by firmware.   ;
; Where binary scales are given as Si/r, the 'i' value refers to the   ;
; scale for the Izhikevich model and the 'r' value refers to the scale ;
; for the RV model (COMPAT=C in CNS).                                  ;
;----------------------------------------------------------------------;
;                    --        ;  0 *Record markers
;                    --        ;  8 *Record length (Host words
;                              ;     including padding to 8X)
;                    --        ; 12 *Pointer range start (hwords)
;                              ; 14 *Pointer range end (hwords)
;                   0,1        ; 16 *SDRAM writeback address (bytes)
;                     2        ; 20 *Writeback start (words - 4)
;                     3        ; 22 *Writeback end   (words - 4)
defNum   icell        4        ; 24 *Number of this cell
defNum   exportSi     5        ; 26 *Byte (S0/8)
defNum   pcnblk       6        ; 28  Ptr to first CONNTYPE block
defNum   pCij         7        ; 30  Ptr to first Cij
defNum   pMij         8        ; 32  Ptr to first Mij
defNum   pVk0         9        ; 34  Ptr to first Vk (Value)
defNum   pSbar       10        ; 36  Ptr to Sbar
defNum   pSj         11        ; 38  Ptr to first Sj
;                    12        ; 40 *Total number of inputs (<256)
;                    13        ; 42 *Number of sensor inputs (<256)
defNum   nct         14        ; 44  Number of connection types
defNum   itype       15        ; 46  Cell type (0-4 used by R.M. code)
defNum     CT_Izhi      5      ;        Izhikevich
defNum     CT_RV        6      ;        Rock Vision
defNum   pSi         16        ; 48  Ptr to Si (Writeback region)

; Work area--If space gets tight, this could also be placed in area
; at end of NPU address space that cannot be loaded from host due to
; Sj pointer compression, but those locations need a wload to set the
; stash address...
defNum   pVk          17       ; 50  Ptr to value for k conntype

; Cell type parameters.  Each record represents one cell, so this
; space can be different for different cell types.  Type codes
; 0-4 are used by R.Martin code and parameters are not shown here.

; Cell type parameters for Rock Vision model.
defNum   sdamp       18        ; 52  (S15)
defNum   tdamp       19        ; 54  1.0 - sdamp (S15)
defNum   omega2      20        ; 56  (S15)
defNum   pt          21        ; 58  (S8/15)
; Cell type parameters for Izhikevich model
defNum   a           18        ; 52  a     (S14)
defNum   b           19        ; 54  b/Cm  (S14)
defNum   c           20        ; 56  c     (S8)
defNum   d           21        ; 58  d/Cm  (S8)
defNum   k           22        ; 60  k/Cm  (S16)
defNum   vT          23        ; 62  vT    (S8)
defNum   vPeak       24        ; 64  vPeak (S8)

; Connection type parameters (nct blocks).  Will start on next
; multiple of 4 bytes after end of celltype parameters.  Offset
; to first block is in pcnblk (NPU word 6) and offsets given
; here are relative to pcnblk.  Summation params are followed
; by amplification params only for those connection-type blocks
; that have cnopt COP_ESV bit set.  Code at NEW_CONNECTION
; assumes order of these params.
defNum   cnopt        0        ; Option bits:
defNum     Cop_Knee     1      ;   Knee threshold
defNum     Cop_Probe    2      ;   This is a probe
defNum     Cop_ESV      4      ;   Use ESV amplification rule
defNum     Cop_Last    64      ;   This is last connection type
defNum     Cop_sgn8  0x80	;   Input Sj are signed 8 bit
				;   (This value MUST be 0x80!)
defNum   nc           1        ; Number of connections of this type
defNum   scl          2        ; (S12)
defNum   sjrev        3        ; (S0/8)
defNum   et           4        ; (S0/8)
defNum   cijmask      5        ; (bits)
defNum   lcnblk       6        ; Length of no-amp conntype block

defNum   upsm         6        ; Growth rate for Mij (S15)
defNum   zetam        7        ; Decay rate for Mij (S15)
defNum   mxmp         8        ; Max Mij increment (S8)
defNum   mxmij        9        ; Max Mij (S8)
defNum   mtj         10        ; Threshold on Sj (S8)
defNum   mtv         11        ; Value threshold (S8)
defNum   mti         12        ; Threshold on Si (S15)
defNum   delta       13        ; "Learning rate" (S12)
defNum   ampway      14        ; 8 way scales (S12)
defNum   lampblk     22        ; Length of conntype block w/amp

; Writeback area--information transmitted to next cycle.  Must
; start on a multiple of 8 host words (multiple of 16 minus 8
; NPU words for the "lost" header) after the last conntype block.
; Offsets of these areas are available in the named locations
; listed.  (Some code requires Cij to start on an even-numbered
; NPU word.)
;        Si           pSi      ; Si (L2S8/15)
;        Ui          (pSi+1)   ; Ui/Cm (L2S8)
;        Sbar         pSbar    ; Sbar (L2S8/15)
;        Cij          pCij     ; Cij (L2S12)
;        Mij          pMij     ; Mij (L2S8)

; Input area.  (If value is required for a connection type,
; NeuroSynth will treat it as two sensors so we can have a
; 16-bit signed value on S8 scale.)
;        Vk           pVk      ; Value (L2S8)
;        Sj           pSj      ; Sj (L1S0/8)

;----------------------------------------------------------------------;
;      Register Plan for input summation and amplification phase       ;
;----------------------------------------------------------------------;

; reg0-reg3  General scratch
; reg4-reg5  Scaled sum over connection types (S8/16)
; reg6   Connection option bits, way code in high byte
; reg7   Address of current Mij
; reg8   Address of current Cij
; reg9   Address of current Sj
; rega   Connection counter
; regb   Loop over per-conn part of conntype block, later Cij
; regc   Loop over per-cell part of conntype block
; regd   Address of current conntype block
; rege   delta * (s(i,t-1) - mti) * (v(t) - mtv) (S12), then
;        16 if amplif, then conntype scale factor
; regf   Fetch/stash addresses

;----------------------------------------------------------------------;
;                            Startup Code                              ;
;----------------------------------------------------------------------;

START_OF_PROGRAM:
         wfetch  reg0 itype    ; Fetch itype, set IMM = 0
         compare reg0 CT_Izhi  ; Skip fire test if not Izhi
         jumpnz  SETUP_INPUT
;
; If spike occurred in previous cycle, as determined by '1' bit
; set in Ui(t-1), there is no need to evaluate input sums, just
; go and perform Izhikevich reset.
; (At present, the Izhikevich test has no Cij amplification,
; therefore, that code is also skipped.  In a real program,
; that would not necessarily be the case.)
;
         fetch   regf pSi      ; Locate Si(t-1)
         inc     regf          ; Step to Ui(t-1)
         fetch   reg0 regf     ; Pick up Ui(t-1)
         and     reg0 1
         jumpnz  IZHIKEVICH_RESET
;
; Set up loop over connection types
;
SETUP_INPUT:
         fetch   regd pcnblk   ; Locate conntype blocks
         fetch   reg7 pMij     ; Locate Mij
         fetch   reg8 pCij     ; Locate Cij
         fetch   reg9 pSj      ; Locate Sj
         fetch   reg0 pVk0     ; Locate Vk
         load    regf pVk      ; Store in work area
         stash   reg0 regf
;
; Clear what will be scaled input total.  (Due to lack of a way to
; add P stack to P, it is better to use general registers for this.)
;
         load    reg4 0
         load    reg5 0

;----------------------------------------------------------------------;
;                     Process one connection type                      ;
;----------------------------------------------------------------------;

NEW_CONNTYPE:
         fetch   reg6 regd     ; Pick up cnopt bits

;----------------------------------------------------------------------;
;    Perform pre-amplification calculations that do not depend on      ;
;  individual connections, i.e. calculate delta*(Sbar-mti)*(Vk-mtv)    ;
; N.B.  For purposes of amplification, when s values are on mV scale,  ;
; we pretend the scale is 7 greater, i.e. 128 mV is a Hebb factor of   ;
; 1.0.  This requires doubling Sj Hebb term to match COMPAT=C S8.      ;
;----------------------------------------------------------------------;

         load    rege 0        ; This will be signal to skip amplif
         load    reg0 reg6     ; Check for amplification
         and     reg0 Cop_ESV
         jumpz   SUM_CONNECTIONS

         clrp                  ; Locate amplif params in regb
         load    reg0 mtv
         addacc  regd
         addacc  reg0
         fetch   reg2 pVk      ; Locate conntype value, 2 NOP ticks
         saveplo regb

         clrp                  ; Compute vk - mtv (S8)
         fetch   reg0 reg2     ; Fetch Vk
         fetch   reg1 regb     ; Fetch mtv
         inc     reg2          ; Advance pVk for next conntype
         load    regf pVk
         stash   reg2 regf
         addacc  reg0
         subtacc reg1
         testp                 ; If (vk-mtv)==0, no amplif this time
         jumpz   SUM_CONNECTIONS
         inc     regb          ; Advance regb to mti, 2 NOP ticks
         saveplo reg3          ; reg3 = vk - mtv

         fetch   regf pSi      ; Locate si(t-1)
         fetch   regc pSbar    ; Locate Sbar
         fetch   reg0 regf
         fetch   reg1 sdamp
         mult    reg0 reg1
         multacc reg0 reg1     ; Double it so product is in P-md
         fetch   reg0 regc
         fetch   reg1 tdamp    ; 2 NOP ticks
         savepmd reg2          ; reg2 = Si*sdamp (S8/15)
         mult    reg0 reg1
         multacc reg0 reg1
         multacc reg0 reg1
         multacc reg0 reg1     ; Four times to prepare for shftp
         shftp                 ; P-lo = Sbar*tdamp (S8/15)
         addacc  reg2          ; P = Si*sdamp + Sbar*(1-sdamp) (S8/15)
         fetch   reg1 regb     ; reg1 = mti, 2 NOP ticks
         saveplo reg0          ; Stash new Sbar for next cycle
         stash   reg0 regc
         subtacc reg1          ; P = Sbar - mti
         testp                 ; If (si-mti)==0, no amplif this time
         jumpz   SUM_CONNECTIONS
         saveplo reg2          ; reg2 = si - mti (S15)
         mult    reg2 reg3     ; P = (si-mti)*(vk-mtv) (S23)

         shftbr  reg2		; Store signs of vk,si terms
         shftbr  reg3		;   in high byte of reg6
         and     reg2 0x80
         and     reg3 0x80
         shftll  reg3
         or      reg3 reg2
         shftll  reg3
         shftll  reg3
         or      reg6 reg3
;
; Scale product times delta.  (Result can have 23 + 15 = 38
; signif bits + sign on scale S35, so we discard 23 bits
; to save 15 bits + sign on scale S12 in rege.)
;
         inc     regb          ; Advance regb to delta
         fetch   rege regb     ; Pick up delta (S12)
         call    MULP32	; delta*vk*si term (S23+12-16=19)
	wload   rege 512	; rege = 1 (S9)
	call    MULT32	; reg0,reg1 = del*vk*si (S19+9-16=12)
    	load    rege reg1     ; Save product in rege for later amplif

;----------------------------------------------------------------------;
;                   Sum connection and probe inputs                    ;
;----------------------------------------------------------------------;

SUM_CONNECTIONS:
         load    regc regd     ; Locate conntype parms
         inc     regc		; Advance to nc
         fetch   rega regc     ; Pick up nc
         inc     regc          ; Advance to scl
         inc     regc          ; Advance to sjrev
         clrp                  ; Initialize conntype sum

NEW_CONNECTION:
         load    regb regc     ; Start serial access to conntype block
         pushp                 ; Save current Ai total
;
; N.B.  The Cij storage area always begins on an even NPU word,
; therefore, we can use the low order bit of the Cij pointer to
; determine whether to use the left or right byte of the Sj hword.
; If this is a model with signed Sj, we must then expand 8 bit Sj
; value to a 16-bit signed value.
;
         fetch   reg0 reg9     ; Pick up Sj (S0/8)
         load    reg1 reg8     ; Test parity of Cij pointer
         and     reg1 1
         jumpz   NCA1
         shftbr  reg0          ; Odd connection, use MSB
         inc     reg9          ; Go to new hword next time
NCA1:    and     reg0 0xff
	load	reg1 reg6	; Test for signed Sj
	and	reg1 Cop_sgn8	; (Cop_sgn8 must be 0x80)
	and	reg1 reg0
	jumpz	NCA2		; Jump if not signed or not negative
	wor	reg0	0xff00	; Propagate sign
	imm	0		; Restore our normal IMM=0

NCA2:    clrp
         fetch   reg1 regb     ; Pick up sjrev
         addacc  reg0          ; Calc Sj-sjrev (S0/8)
         subtacc reg1
         inc     regb          ; Locate et, fill NOP slots
         fetch   reg2 regb     ; Pick up et
         saveplo reg3          ; Save Sj-sjrev in reg3 for amplif
         compare reg3 reg2     ; Test Sj-sjrev vs et
         jumpn   CHECK_FOR_AMPLIF
         inc     regb          ; Locate cijmask
         load    reg0 reg6
         and     reg0 Cop_Knee ; Test knee threshold bit
         jumpz   NCA3
         subtacc reg2

NCA3:    fetch   reg1 reg8     ; Pick up Cij, fill NOP slots
         fetch   reg2 regb     ; Pick up cijmask
         saveplo reg0          ; Modified Sj to reg0
         popp                  ; Restore Ai total
         and     reg1 reg2     ; Shorten Cij to specified precision
         multacc reg0 reg1     ; Ai = Sum(Cij*Sj) (S12/20)

;----------------------------------------------------------------------;
;              Perform connection strength amplification               ;
;              (Currently only rule ESV is implemented)                ;
;----------------------------------------------------------------------;
;
; On entry here, P is Ai, reg3 = Sj - sjrev (S0/8), reg6 has
; Vk|Si ampway signs, rege has the delta*(Si-mti)*(Vk-mtv) product.
; Following logic at SLOWAMP in d3go, we begin by updating Mij.
;
CHECK_FOR_AMPLIF:
         compare rege 0        ; Test amplification flag
         jumpz   NEXT_CONNECTION

         pushp                 ; Save current total
	load    reg0 reg6	; If using signed mV inputs,
	and     reg0 Cop_sgn8	;   double reg3 to imputed S8
	jumpz	CAM1
	shftll  reg3

CAM1:    clrp                  ; Locate amplif params in regb
         load    reg0 upsm
         addacc  regd
         addacc  reg0
         fetch   reg2 reg7     ; Pick up Mij(t-1), 2 NOP ticks
         saveplo regb
         fetch   reg1 regb     ; reg1 = upsm
         mult    reg1 reg3     ; P = upsm * (Sj - sjrev) (S23)
         multacc reg1 reg3     ; Double it to S24
         inc     regb          ; Advance to zetam, 2 NOP ticks
         savepmd reg3          ; reg3 = upsm*(Sj - sjrev) (S24-16=8)
         fetch   reg1 regb     ; reg1 = zetam
         mult    reg1 reg2     ; P = zetam * Mij (S23)
         multacc reg1 reg2     ; Double it to S24
         inc     regb          ; Advance to mxmp, 2 NOP ticks
         savepmd reg1          ; reg1 = zetam * Mij (S24-16=8)
         fetch   reg0 regb     ; reg0 = mxmp
         testp                 ; If abs(zetam*Mij) > mxmp,
         jumpn   CAM2          ;   use mxmp with correct sign
         compare reg1 reg0
         jumpn   CAM4          ; All is well
         jump    CAM3
CAM2:    clrp                  ; Mij < 0, do reverse test
         subtacc reg0
         saveplo reg0          ; reg0 = -mxmp, 2 unavoidable NOPs
         compare reg0 reg1
         jumpn   CAM4          ; All is well
CAM3:    load    reg1 reg0     ; Replace zetam*Mij with signed mxmp
CAM4:    clrp                  ; Mij = Mij + upsm*(Sj - sjrev) -
         addacc  reg2          ;   Min(zetam*Mij, mxmp) (S8)
         addacc  reg3
         subtacc reg1
         inc     regb          ; Advance to mxmij
         saveplo reg1          ; reg1 = Updated Mij
         fetch   reg0 regb     ; reg0 = mxmij
         compare reg1 reg0
         jumpn   CBM2          ; All is well
	load 	reg1 reg0	; Replace Mij with mxmij in reg1
         clrp			;   and in P
         addacc  reg0
;
; Form quint product wayscale*delta*(Vk-mtv)*(Si-mti)*(Mij-mtj)
;
CBM2:    stash   reg1 reg7	; Save updated Mij

	inc     regb          ; Advance to mtj
         fetch   reg1 regb     ; reg1 = mtj
         subtacc reg1          ; P = Mij - mtj (S8)
         load    regf ampway   ; Locate ampway, 1 NOP slot
         saveplo reg2          ; reg2 = Mij - mtj, 1 unavoidable NOP
         mult    reg2 rege     ; P = delta*(Vk-mtv)*(Si-mti)*(Mij-mtj)
				;   (24 signif bits on scale S20)
         shftbr  reg2          ; Sign(Mij - mtj) to way code
         and     reg2 0x80
         shftll  reg2
         or      reg2 reg6
         shftbr  reg2
         savepmd reg0          ; Quad product to reg0,reg1 (S20)
         saveplo reg1
         clrp                  ; Locate ampway multiplier
         addacc  regd
         addacc  regf
         addacc  reg2
         fetch   regb reg8     ; regb = Cij (S12), 2 NOP ticks
         saveplo regf
         fetch   rege regf     ; rege = way scale (S12)
         call    MULT32        ; reg0,1 = quint product (S32-16=16)
;
; If sign of the change in Cij is same as sign of Cij,
; apply phi (saturation) factor = 1.0 - 2Cij**2 + Cij**4
; (regb no longer needed for addressing, use to hold Cij)
;
         pushp                 ; Save quint product in stack
         mult    regb reg0     ; Get sign of Cij * Delta(Cij)
         testp
         jumpn   PHI1
         mult    regb regb     ; P = Cij**2 (S24)
         load    rege 16       ; rege = 1 (S4)
         call    MULP32        ; P = reg0,reg1 = Cij**2 (S28-16=12)
         load    regb reg1     ; regb = Cij**2 (S12)
         mult    reg1 reg1     ; P = Cij**4 (S24)
	wload	reg3 4096	; reg3 = 1S12, IMM!=0 fixed by MULP32
         call    MULP32        ; P = reg0,reg1 = Cij**4 (S28-16=12)
         addacc  reg3     	; P = (1 + Cij**4) (S12)
         subtacc regb
         subtacc regb          ; P = phi(Cij) (S12)
         fetch   regb reg8     ; Get Cij again, 2 NOP ticks
	saveplo rege          ; rege = Phi(Cij) or 1.0 (S12)
         jump    PHI2		; 2 NOP ticks
PHI1:    wload	reg3 4096	; reg3 = 1S12, IMM!=0 fixed by MULT32
         load	rege reg3	; N.B. regb still has Cij here
PHI2:    popp                  ; Get back the quint product
         call    MULP32        ; P = reg0,reg1 = Delta phi (S12)
         addacc  regb          ; P = updated Cij
;
; Now check for overflow, constrain Cij to allowed range
;
         testp                 ; If new Cij < 0
         jumpn   COV2          ;   use 1S12 with correct sign
	saveplo reg1
         compare reg1 reg3
         jumpn   COV4          ; All is well
         jump    COV3
COV2:    wload	reg3 0xf000   ; 0xf000 = -1 (S12)
	imm	0
         compare reg3 reg1
         jumpn   CAM4          ; All is well
COV3:    load    reg1 reg3     ; Replace Cij with signed 1S12
COV4:    stash   reg1 reg8	; Save modified Cij
         popp                  ; Restore Ai sum
;
; Advance to next connection
; Note:  Do not step reg9 (Sj pointer) here--that is done
;   above when the choice is made to use the LSB or MSB Sj.
;
NEXT_CONNECTION:
	inc	reg7		; Advance to next Mij
				;  (even if amplif skipped)
         inc     reg8          ; Advance to next Cij
         dec     rega          ; Decrease connection counter
         jumpnz  NEW_CONNECTION

;----------------------------------------------------------------------;
;   Have connection sum in P (S12), scale it and add into SummAffs     ;
;----------------------------------------------------------------------;
;
; N.B.  Sj have no more than 8 bits, Cij 12 bits, and nc 8 bits,
; therefore the Sum(Cij*Sj) currently in P has no more than 28
; bits on scale S12/20 and P-hi can be ignored.  After scaling,
; we are on S24/32 and after rounding and discarding 16 P-lo bits,
; this is S8/16 and < 28 bits.  Allowing no more than 8 connection
; types (per NSI docs), the scaled sum fits in 31 bits and this
; is stored in regs 4 and 5.
;
         dec     regc          ; Point back at scale factor
         fetch   rege regc     ; Pick up scale factor
         call    MULP32
         wand    reg2 0x8000   ; Test rounding bit
         jumpz   ANR1
         load    reg2 1
         addacc  reg2          ; Round the product
ANR1:    addacc  reg5          ; Add in low grand total
         imm     0             ; Restore IMM = 0, 2 NOP ticks
         load    reg2 reg6     ; Prepare for conntype test below
         saveplo reg5          ; Save new lo grand total
         savepmd reg0          ; Hi total with carry to reg0
         clrp
         addacc  reg0          ; Effective right shift 16 bits
         addacc  reg4          ; Add in high grand total
         and     reg2 Cop_ESV  ; Test connection type, 2 NOP ticks
         savepmd reg4          ; Save new hi grand total
;
; Advance to next connection type
;
         load    reg0 lcnblk   ; Get proper increment to reg0
         jumpz   ANR2
         load    reg0 lampblk
ANR2:    clrp                  ; Increment regd by length
         addacc  regd          ;    of CONNTYPE block left
         addacc  reg0          ;    in reg0 by above code.
         load    reg1 reg6     ; Test last conntype bit, 2 NOP slots
         and     reg1 Cop_Last
         saveplo regd
         jumpz   NEW_CONNTYPE
;
; Finished all connection inputs, compute cell response
;
         fetch   reg0 itype
         compare reg0 CT_Izhi
         jumpz   EVAL_IZHI_MODEL

;----------------------------------------------------------------------;
;            Evaluate standard piecewise continuous model              ;
;  N.B.  Afferent sum in reg4,5 is S8/16 because we know on old scale  ;
;  that all Sj are >= 0, so sign bit can be used as MSB of Sj.  The    ;
;  high order in reg4 allows partial sums to overflow as long as final ;
;  total is in range.  However, once shortened to 16 bits, values on   ;
;  old (COMPAT=C) scale must be shifted to S15 so multiplications by   ;
;  sdamp, omega2, etc. will not incorrectly interpret MSB as negative. ;
;  The present code assumes all non-Izhikevich models are COMPAT=C     ;
;  scale, but in a more general code, that would need to be an option. ;
;----------------------------------------------------------------------;
;
; Modifications to general register plan:
; reg6   Si(t)
; regc   pt
; rege   omega2
;
         fetch   regf pSi      ; Locate Si(t-1)
         fetch   rege omega2
         fetch   regc pt

         fetch   reg1 regf     ; reg1 = Si(t-1) (S15)
         mult    rege reg1     ; P = omega2*Si(t-1) (S30)
         multacc rege reg1     ; To (S31)
;
; This code scales sum in reg4,5 to S15 while adding it to
; persistence term, which has to be doubled to use shftp.
; AffSum in reg4,5 would need to be added twice if on S16.
;
         multacc rege reg1     ; Persistence to (S32)
         multacc rege reg1
         shftp                 ; Persistence to (S15)
         wload   reg3 0x7fff   ; N.B.  IMM != 0 for a while
         multacc reg4 reg3
         addacc  reg4
         shftlr  reg5
         addacc  reg5          ; AffSum + Persistence
         subtacc regc          ;   - pt (KNEE calc assumed)
         wload   reg6 0        ; Result if sum < pt, restore IMM=0
         testp                 ; Test for negative result
         jumpn   SPM3
         load    reg6 reg3     ; Result if overflow
         jumpcy  SPM3
         savepmd reg0          ; Hi total with carry to reg0
         compare reg0 0        ; This test required because
         jumpnz  SPM3          ; testp does not detect all carries
         saveplo reg6          ; Si(t) to reg6
SPM3:    stash   reg6 regf     ; Store Si(t)
         jump    ALL_DONE

;----------------------------------------------------------------------;
;                      Evaluate Izhikevich model                       ;
;----------------------------------------------------------------------;
;
; Modifications to general register plan:
; reg6   Si(t-1) then Si(t) (S8)
; reg7   Ui(t-1) then Ui(t) (S8)
; rega   Constant -1
; regc   Address of Si(t)
; regd   Address of Ui(t)
; rege   Izhikevich k, then b, then a
;
; N.B.   IMM != 0 during much of this code
;
; There is no way to signal the host in case of an overflow.
; Best option seems to be to set the result to the largest
; available positive or negative number.
;
EVAL_IZHI_MODEL:
         wload   rega 0xffff   ; Useful constant
         compare rega reg5
         jumpn   IMA1          ; If AffSum is negative,
         inc     reg4          ;   only valid AffSumHi value is -1
IMA1:    and     reg4 reg4     ; Test w/o using IMM
         jumpz   IMA4          ; AffSumLo in reg5 is OK
         jumpn   IMA2          ; Jump if negative overflow
         wload   reg5 0x7fff   ; Positive overflow, make huge
         jump    IMA4
IMA2:    wload   reg5 0x8000   ; Negative overflow, make huge
;
; Finished checking afferent sum for overflow.
; Now can proceed to evaluate dV = k*v*(v-vt) - u + I
; (Caution: IMM may be nonzero on entry here)
;
IMA4:    wfetch  regc pSi      ; Locate Si(t-1), restore IMM = 0
         fetch   reg6 regc     ; reg6 = Si(t-1) (S8)
         fetch   reg0 vT
         clrp
         addacc  reg6
         subtacc reg0          ; P = (v-vT) (S8)
         load    regd regc     ; Locate Ui(t-1), 2 NOP slots
         inc     regd
         saveplo reg1
         mult    reg6 reg1     ; P = v*(v-vT) (S16)
         fetch   reg7 regd     ; Ui to reg7 for use later
         load    reg0 0x80     ; Rounding test
         saveplo reg1
         and     reg0 reg1
         addacc  reg0          ; Round
         ; Multiply product by 0x0100 so when low order is
         ; discarded net result is right shift of 8 bits,
         ; leaving 24-bit v*(v-vT) product (S8)
         wload   rege 0x0100   ; Preload multiplier, fill NOP slots
         call    MULP32        ; (IMM = 0 guaranteed on return)
         ; Now can multiply by k.  Because k is (S16), the result
         ; is (S24), so we can round and discard low order,
         ; leaving (S8) result in P for addition of other terms.
         fetch   rege k
         call    MULP32
         compare rega reg2     ; Need to round?
         jumpn   KPA2
         subtacc rega          ; -(-1) = +1
KPA2:    addacc  reg6          ; Si(t) = Si(t-1) + k*v*(v-vT)
         subtacc reg7          ;   - u
         addacc  reg5          ;   + I
         fetch   reg1 b        ; Pick up b (S14), 2 NOP ticks
         saveplo reg0          ; reg0 = Si(t); leave Si(t-1) in reg6
;
; Update u.  u(t) = u(t-1) + a*(b*v - u)
;
         mult    reg6 reg1     ; b*v (S22)
         load    reg6 reg0     ; New Si for code at ALL_DONE
         load    rege 4        ; Scale b*v to (S24)
         call    MULP32        ; (Leaves 32 high bits (S8) in P)
         subtacc reg7          ; b*v - u (S8)
         fetch   rege a        ; Pick up a (S14)
         call    MULP32        ; a*(b*v - u) (S22)
         load    rege 4        ; Again scale to (S24)
         call    MULP32
         addacc  reg7          ; u(t) = u(t-1) + du
         stash   reg6 regc     ; Save new Si, fill NOP slots
         ; Recalling that TESTP does not correctly check for
         ; overflow if result is negative, check now for
         ; overflow by a slower but correct method (see above)
         savepmd reg0
         saveplo reg7          ; u(t)
         compare rega reg1
         jumpn   UPU1
         inc     reg0
UPU1:    compare reg0 0
         jumpz   IRS2          ; No overflow, go save Ui(t)
         testp                 ; Check sign of result
         jumpn   UPU3
         wload   reg7 0x7fff
         imm     0
         jump    IRS2
UPU3:    wload   reg7 0x8000
         imm     0
         jump    IRS2

;----------------------------------------------------------------------;
;                         Vm(t-1) was > vPeak                          ;
; No need to evaluate afferent sums, just do Izhikevich reset:         ;
;    Vm <- c, u <- u+d                                                 ;
;----------------------------------------------------------------------;

IZHIKEVICH_RESET:
         fetch   reg6 c        ; Set Si(t) to c
         load    regc pSi

         load    regd regc     ; Locate Ui(t-1)
         inc     regd
         fetch   reg7 regd     ; Set Ui(t) to Ui(t-1) + d
         clrp
         addacc  reg7
         fetch   reg1 d
         addacc  reg1
         stash   reg6 regc     ; Save Si(t), 2 NOP ticks
         saveplo reg7
IRS2:    stash   reg7 regd

;----------------------------------------------------------------------;
;                            All finished                              ;
;----------------------------------------------------------------------;
;
; Save high byte of new Si (left in reg6 above) in exportSi
;
ALL_DONE:
         shftbr  reg6
         load    regf exportSi
         stash   reg6 regf
         jump    START_OF_PROGRAM  ; Standard end-of-program

;----------------------------------------------------------------------;
;                            MULT32, MULP32                            ;
;                                                                      ;
;    MULT32 performs a signed multiply of 32 bits in reg0(Hi),reg1(Lo) ;
;    by 16 bits in rege and returns the result in reg0(Hi),reg1(Md),   ;
;    reg2(Lo).  The Hi 32 bits are left in P also.                     ;
;                                                                      ;
;    MULP32 performs the same operation except the multiplicand is     ;
;    initially in P(Md),P(Lo).  The call to MULP32 can provide the     ;
;    needed delay times following some earlier operation on P.         ;
;                                                                      ;
;    No scratch regs are used.                                         ;
;----------------------------------------------------------------------;

MULP32:  savepmd reg0		    ; Move multiplicand to reg0,reg1
	saveplo reg1
MULT32:  mult    reg1 rege         ; Low-order multiply
         load    reg2 reg1         ; Sign test for carry correction
         wand    reg2 0x8000
         saveplo reg2              ; reg2 = Product(Lo)
         multacc reg1 rege         ; Repeat for 2*product
         shftp                     ; P(Lo) = Carry from Product(Lo)
         jumpz   M32A1
         addacc  rege
M32A1:   multacc reg0 rege         ; Hi-order multiply
         imm     0                 ; Restore IMM = 0, use 1 NOP slot
         saveplo reg1              ; Retrieve results
         savepmd reg0
         ret
