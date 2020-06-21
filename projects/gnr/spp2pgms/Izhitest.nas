;======================================================================;
;                            Izhitest.nas                              ;
;                                                                      ;
; This is a test program to run a basic Izhikevich neuron model on     ;
; the SPP2.  It is intended to work in conjunction with versions of    ;
; the NeuroSynthesizer and UserTools that have been modified to work   ;
; with this test.  This test is not intended to represent any          ;
; particular neuronal system.                                          ;
;                                                                      ;
; The NeuroSyntheizer will place one celltype comprising 1000 cells    ;
; on each of the three SPP2 chips.  Each cell will have 4 inputs from  ;
; other cells:  two from its own chip and one from each of the other   ;
; chips, plus one external ("sensor") input representing the current   ;
; probe.  Parameters a,b,c,d will be variable across cells in order to ;
; enable checking that different results come from different cells.    ;
;                                                                      ;
; Variables v and u of the model will be kept to 16-bit precision      ;
; internally (range +/- 128 mv S8).  Due to the fact that the SPP2     ;
; can exchange only 8-bit activity values between cells, only the      ;
; high-order 8 bits (integer mV) of the activity will be exchanged.    ;
;                                                                      ;
; Code assumes an assembler that inserts NOPs only where needed.       ;
; Much of the multiprecision arithmetic has been written in order to   ;
; explore the capabilities and limitations of the Assembler language.  ;
; In a real program, less precise versions would probably be used to   ;
; get greater speed.                                                   ;
;======================================================================;
; V1A, 10/26/09, GNR - New program                                     ;
;======================================================================;

;----------------------------------------------------------------------;
;                            Memory layout                             ;
;----------------------------------------------------------------------;

; Record header
defNum   icell        4        ; Number of this cell
defNum   exportSi     5        ; Byte (S0)
defNum   pCij         7
defNum   pSj         11

; Cell type parameters (just one set in this program)
defNum   nct         14        ; Number of connection types
defNum   a           15        ; (S14)
defNum   b           16        ; b/Cm (S14)
defNum   c           17        ; (S8)
defNum   d           18        ; d/Cm (S8)
defNum   k           19        ; k/Cm (S16)
defNum   vT          20        ; (S8)
defNum   vPeak       21        ; (S8)

; Connection type parameters (nct blocks)
; (Code at NEW_CONNECTION assumes order of these params)
defNum   acn1        24        ; Address of first conntype block
defNum   cnopt        0        ; Option bits:
defNum   Cop_Knee     1        ;   Knee threshold
defNum   Cop_Probe    2        ;   This is a probe
defNum   nc           1        ; Number of connections of this type
defNum   scl          2        ; (S12)
defNum   et           3        ; (S0)
defNum   sjrev        4        ; (S0)
defNum   cijmask      5        ; (logical)
defNum   lcnblk       6        ; Length of conntype block
;                    30-35     ; Data for CONNTYPE 2
;                    36-41     ; Data for CONNTYPE 3
;                    42-47     ; Data for CONNTYPE 4

; Work area--placed in space at end of control blocks that
; is otherwise unusable because writeback area must start on
; a (16*n+8) address.  Could also go at end of NPU address
; space that cannot be loaded from host due to Sj pointer
; compression, but those locations need a wload to set the
; stash address...
defNum   acne        55        ; End of conntype blocks

; Writeback area--information transmitted to next cycle
; Must start on a multiple of 8 host words (multiple of
; 16 minus 8 NPU words)
defNum   Si          56        ; (S8)
defNum   Ui          57        ; u/Cm (S8)
;        Cij                   ; After Ui (S12, 2 bytes/connection)
;        Sj                    ; After Cij (S0, 1 byte/connection)

;----------------------------------------------------------------------;
;                            Register Plan                             ;
;----------------------------------------------------------------------;

; Register Plan
;    (If two listings, first refers to input summation stage,
;     second to cell response stage)
; reg0-reg5  General scratch
; reg6   Si(t-1) (S8)
; reg7   Connection option bits, then Ui(t-1) (S8)
; reg8   Address of current Cij, then total input (S8)
; reg9   Address of current Sj
; rega   Connection counter, then -1
; regb   Loop over per-conn part of conntype block
; regc   Loop over per-cell part of conntype block
; regd   Address of current conntype block, then k
; rege   Conntype scale factor, then k, then b, then a
; regf   Fetch/stash addresses

;----------------------------------------------------------------------;
;                            Startup Code                              ;
;----------------------------------------------------------------------;

START_OF_PROGRAM:
;
; If spike occurred in previous cycle, as determined by Si(t-1),
; returned to this cycle being above vPeak, there is no need to
; evaluate input sums, just go and perform Izhikevich reset
;
         wload   regf Si       ; Locate Si(t-1), set IMM = 0
         fetch   reg0 regf
         load    regf vPeak
         fetch   reg1 regf
         compare reg1 reg0
         jumpn   IZHIKEVICH_RESET
;
; Set up loop over connection types
;
         load    regd acn1     ; Locate conntype blocks
         fetch   reg0 nct      ; Locate end of conntype blocks
         load    reg1 lcnblk
         mult    reg0 reg1
         addacc  regd
         fetch   reg8 pCij     ; Locate Cij, fill NOP slots
         fetch   reg9 pSj      ; Locate Sj
         saveplo reg0
         load    regf acne
         stash   reg0 regf     ; acne = acn1 + nct*lcnblk
;
; Clear what will be input total.  (Due to peculiarities of how P
; stack works, this is stored as 16 bits in each of 3 stack registers
;
         clrp
         pushp
         pushp
         pushp

;----------------------------------------------------------------------;
;                   Sum connection and probe inputs                    ;
;----------------------------------------------------------------------;

NEW_CONNTYPE:
         load    regc regd     ; Locate conntype parms
         fetch   reg7 regc     ; Pick up cnopt bits
         inc     regc
         fetch   rega regc     ; Pick up nc
         inc     regc          ; Skip over scl
         inc     regc          ; Locate et
         clrp                  ; Initialize conntype sum

NEW_CONNECTION:
         load    regb regc     ; Restart serial access to conntype block
;
; N.B.  The Cij storage area always begins on a multiple of 8 host
; words, therefore, we can use the low order bit of the Cij pointer
; to determine whether to use the left or right byte of the Sj hword.
;
         fetch   reg0 reg9     ; Pick up Sj
         load    reg1 reg8     ; Test parity of Cij pointer
         and     reg1 1
         jumpz   NCA1
         shftbr  reg0          ; Odd connection, use MSB
         inc     reg9          ; Go to new hword next time
NCA1:    and     reg0 0xff

         fetch   reg1 regb     ; Pick up et
         compare reg0 reg1     ; Test Sj-et
         jumpn   NEXT_CONNECTION
         pushp                 ; Save current total
         inc     regb          ; Locate sjrev
         clrp                  ; Calc Sj (- et if Cop_Knee) - sjrev
         fetch   reg2 regb     ; Pick up sjrev
         inc     regb          ; Locate cijmask
         addacc  reg0          ; Sj to P
         load    reg0 reg7
         and     reg0 Cop_Knee ; Test knee threshold bit
         jumpz   NCA2
         subtacc reg1
NCA2:    subtacc reg2
         fetch   reg1 reg8     ; Pick up Cij, fill NOP slots
         fetch   reg2 regb     ; Pick up cijmask
         saveplo reg0          ; Modified Sj to Reg0
         and     reg1 reg2     ; Shorten Cij to specified precision
         popp                  ; Current total back to P
         multacc reg0 reg1     ; Ai = Sum(Cij*Sj) (S12)
;
; Advance to next connection
; Note:  Do not step Reg9 (Sj pointer) here--that is done
; above when the choice is made to use the LSB or MSB Sj.
;
NEXT_CONNECTION:
         inc     reg8          ; Advance to next Cij
         dec     rega          ; Decrease connection counter
         jumpnz  NEW_CONNECTION

;----------------------------------------------------------------------;
;   Have connection sum in P (S12), scale it and add into SummAffs     ;
;----------------------------------------------------------------------;

         dec     regc          ; Point back at scale factor
         fetch   rege regc     ; Pick up scale factor

         saveplo reg1          ; 48-bit + 16 bit x 48-bit
         savepmd reg2
         savephi reg3
         popp                  ; AffSumLo to P
         multacc reg1 rege     ; P = AffSumLo + CTSumLo * scl
         shftll  reg1          ; Sign(CTSumLo) to CY
         saveplo reg1          ; Reg1 = new AffSumLo
         savepmd reg4          ; Reg4,5 = carry from AffSumLo
         savephi reg5
         popp                  ; AffSumMd to P
         addacc  reg4          ; + (CTSumLo*scl)Md
         multacc reg2 rege     ; + (CTSumMd*scl)
         addacc  rege          ; (Because no jumpncy)
         jumpcy  NCB1
         subtacc rege          ; (Undo addacc if CTSumLo < 2**15)
NCB1:    shftll  reg2          ; Sign(CTSumMd) to CY
         saveplo reg2          ; Reg2 = new AffSumMd
         savepmd reg4
         popp                  ; AffSumHi to P
         addacc  reg5          ; + (CTSumLo*scl)Hi
         addacc  reg4          ; + (CTSumMd*scl)Md
         multacc reg3 rege     ; + (CTSumHi*scl)
         addacc  rege
         jumpcy  NCB2
         subtacc rege
NCB2:    pushp                 ; new AffSumHi to stack
         clrp                  ; new AffSumMd to stack
         addacc  reg2
         pushp
         clrp                  ; new AffSumLo to stack
         addacc  reg1
         pushp
;
; Advance to next connection type
;
         clrp
         load    reg0 lcnblk
         addacc  regd
         addacc  reg0
         load    regf acne
         fetch   rege regf
         saveplo regd
         compare regd rege
         jumpnz  NEW_CONNTYPE

;----------------------------------------------------------------------;
;             Have SumAffs, now evaluate Izhikevich model              ;
;----------------------------------------------------------------------;
;
; Retrieve input sum from P stack.
; Reduce from S24 to S8 with rounding, i.e. discard low hword.
;
         wload   rega -1       ; Useful constant
         popp                  ; AffSumLo to Reg2 (for rounding)
         saveplo reg2
         popp                  ; AffSumMd to Reg8 (the desired sum)
         saveplo reg8
         popp                  ; AffSumHi to P (for overflow test)
         compare rega reg8
         jumpn   IMA1          ; If AffSumMd is negative,
         subtacc rega          ;   only valid AffSumHi value is -1
IMA1:    testp
         jumpz   NotOver       ; No overflow
;
; There is no way to signal the host in case of an overflow.
; Best option seems to be to set the result to the largest
; available positive or negative number.
;
SumOver: compare rega reg8     ; Got overflow
         jumpn   IMC1
         wload   reg8 0x7fff   ; Positive result, make huge
         jump    K_PRODUCT
IMC1:    wload   reg8 0x8000   ; Negative result, make huge
         jump    K_PRODUCT

NotOver: compare rega reg2     ; (Reverse test gives one less jump)
         jumpn   K_PRODUCT     ; No round if AffSumLo >= 0
         inc     reg8
         jumpcy  SumOver
;
; Finished retrieving afferent sum and checking for overflow.
; Now can proceed to evaluate dV = k*v*(v-vt) - u + I
; (Caution: IMM may be nonzero on entry here)
;
K_PRODUCT:
         wload   regf Si       ; Locate Si(t-1), restore IMM = 0
         fetch   reg6 regf     ; Reg6 = Si(t-1)
         load    regf vT       ; Locate vT
         fetch   reg0 regf     ; Reg0 = vT
         clrp
         addacc  reg6
         subtacc reg0          ; P = (v-vT) (S8)
         load    regf k        ; Temporarily park k in Regd
         fetch   regd regf     ; Thus filling NOP slots
         saveplo reg1
         load    regf Ui       ; And fetch u(t-1) into Reg7
         fetch   reg7 regf     ;    for use later
         mult    reg6 reg1     ; P = v*(v-vT) (S16)
         load    reg0 0x0080   ; Rounding test
         saveplo reg1
         and     reg0 reg1
         addacc  reg0          ; Round
         ; Multiply product by 0x0100 so when low order is
         ; discarded net result is right shift of 8 bits,
         ; leaving 24-bit v*(v-vT) product (S8)
         wload   rege 0x0100   ; Preload multiplier, fill NOP slots
         savepmd reg0          ; Rounded v*(v-vT) to Regs 0,1
         saveplo reg1
         call    MULT32        ; (IMM = 0 guaranteed on return)
         ; Now can multiply by k.  Because k is (S16), the result
         ; is (S24), so we can round and discard low order,
         ; leaving (S8) result in P for addition of other terms.
         load    rege regd     ; Use k stored in Regd earlier
         call    MULT32
         compare rega reg2     ; Need to round?
         jumpn   KPA2
         subtacc rega          ; -(-1) = +1
KPA2:    addacc  reg6          ; Si(t+1) = Si(t) + k*v*(v-vT)
         subtacc reg7          ;   - u
         addacc  reg8          ;   + I
         load    regf b        ; Locate b, fill NOP slots
         fetch   reg1 regf     ; Pick up b
         saveplo reg0          ; (Leave old Si in Reg6 in case
                               ;    needed for alt u calc.)
;
; Update u.  u(t) = u(t-1) + (du = a*(b*u - u) = a*(b-1)*u)
;
         mult    reg7 reg1     ; b*u (S22)
         load    regf Si       ; Save new Si, fill NOP slots
         stash   reg0 regf
         saveplo reg1          ; b*u to Reg 0,1 for scaling
         savepmd reg0
         load    rege 4        ; Scale b*u to (S24)
         call    MULT32        ; (Leaves 32 high bits (S8) in P)
         ; Recalling that TESTP does not correctly check for
         ; overflow if result is negative, check now for
         ; overflow by a slower but correct method (see above)
         compare rega reg1
         jumpn   UPU1
         inc     reg0
UPU1:    and     reg0 rega
         jumpnz  UiOver        ; Got overflow
         subtacc reg7          ; b*u - u (S8)
         load    regf a        ; Pick up a, fill NOP slots
         fetch   rege regf
         saveplo reg1          ; Prepare to multiply by a
         mult    reg1 rege     ; a*(b*u - u) (S22)
         load    rege 4        ; Again scale to (S24), 1 NOP slot
         saveplo reg1
         savepmd reg0
         call    MULT32
         ; Another test for overflow--see comment above
         compare rega reg1
         jumpn   UPU3
         inc     reg0
UPU3:    and     reg0 rega
         jumpnz  UiOver        ; Got overflow
         addacc  reg7          ; Ui(t) = Ui(t-1) + a*(b*u - u)
         load    regf Ui       ; Locate Ui, fill 1 NOP slot
         saveplo reg0
         stash   reg0 regf     ; Save updated Ui(t)
         jump    ALL_DONE
;
; Here when calculation of Ui(t) overflows
; Sign of result should be same as current sign(P)
;
UiOver:  load    regf Ui
         testp
         jumpn   UiNeg
         load    rega 0
UiNeg:   stash   rega regf
         jump    ALL_DONE

;----------------------------------------------------------------------;
;                         Vm(t-1) was > vPeak                          ;
; No need to evaluate afferent sums, just do Izhikevich reset:         ;
;    Vm <- c, u <- u+d                                                 ;
;----------------------------------------------------------------------;

IZHIKEVICH_RESET:
         load    regf c        ; Set Si(t) to c
         fetch   reg6 regf
         load    regf Si
         stash   reg6 regf

         load    regf Ui       ; Set Ui(t) to Ui(t-1) + d
         fetch   reg7 regf
         clrp
         addacc  reg7
         load    rege d
         load    reg1 rege
         addacc  reg1
         saveplo reg7          ; (No avoiding 2 NOPs here)
         stash   reg7 regf

;----------------------------------------------------------------------;
;                            All finished                              ;
;----------------------------------------------------------------------;
;
; Save high byte of new Si (left in Reg6 above) in exportSi
;
ALL_DONE:
         shftbr  reg6
         load    regf exportSi
         stash   reg6 regf
         jump    START_OF_PROGRAM  ; Standard end-of-program

;----------------------------------------------------------------------;
;                                MULT32                                ;
;    Signed multiply of 32 bits in Reg0(Hi)-Reg1(Lo) by 16 bits in     ;
;    Reg14.  Return result in Reg0-Reg2 (Hi 32 bits left in P also).   ;
;    No scratch regs are used.                                         ;
;----------------------------------------------------------------------;

MULT32:  mult    reg1 rege         ; Low-order multiply
         wand    reg1 0x8000       ; Test for sign adjustment, 2 slots
         saveplo reg2              ; Reg2 = Product(Lo)
         multacc reg1 rege         ; Repeat for 2*product
         shftp                     ; P(Lo) = Carry from Product(Lo)
         jumpz   M32A1
         addacc  rege
M32A1:   multacc reg0 rege         ; Hi-order multiply
         imm     0                 ; Restore IMM = 0, use 1 NOP slot
         saveplo reg1              ; Retrieve results
         savepmd reg0
         ret
