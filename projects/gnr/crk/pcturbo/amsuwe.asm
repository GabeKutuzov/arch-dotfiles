           %TITLE "AMSUWE 64-BIT MULTIPLY, SHIFT, AND ADD ROUTINE"
;**********************************************************************
;                                                                     *
;     ROCKS CRYSTALLOGRAPHIC COMPUTING SYSTEM - C IMPLEMENTATION      *
;                                                                     *
;     Synopsis:  ui64 amsuwe(ui64 sum, unsigned long x,               *
;          unsigned long y, int s, int ec)                            *
;                                                                     *
;     This routine multiplies two unsigned long integers, shifts the  *
;          product by s bits (pos=left, neg=right), then adds the     *
;          shifted product into a 64-bit accumulator and returns      *
;          the total.  Errors result in calls to e64act().            *
;                                                                     *
;     Several of these routines require setting the coprocessor       *
;          to the "chop" rounding mode.  The existing state of the    *
;          control register is saved and restored, although a call    *
;          to Borland indicated that they "thought" this was not      *
;          strictly required.  In fact, their own library functions   *
;          (such as float to long) perform exactly this maneuver.     *
;          To minimize bp manipulation, we use one of the arguments   *
;          as a temp for saving the control word.                     *
;                                                                     *
;**********************************************************************                                                                     *
;
;        V1A, 07/10/99, G. N. Reeke
;
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL MDL,C
           EXTRN   e64act:PROC
           PUBLIC  amsuwe

ui64       STRUC                       ;Define 64-bit data item
hi         DD      ?
lo         DD      ?
ui64       ENDS

           .CODE
amsuwe     PROC
           ARG     sum:ui64,x:DWORD,y:DWORD,s:WORD,ec:WORD
           fild    s                   ;Get user's scale
           fild    x                   ;Pick up first multiplier
           ftst                        ;Convert neg to big pos
           fstsw   ax
           sahf
           jae     pos1
           fadd    CS:s32

pos1:      fild    y                   ;Pick up second multiplier
           ftst                        ;Convert neg to big pos
           fstsw   ax
           sahf
           jae     pos2
           fadd    CS:s32

pos2:      fmul
           fnstcw  s                   ;Save coprocessor control word
           mov     ax,SEG retval       ;Locate result struct
           mov     es,ax
           ASSUME  es:SEG retval

           fwait
           mov     ax,s                ;Enable round down mode
           or      ah,04h
           mov     WORD PTR x,ax
           fldcw   WORD PTR x

           fscale                      ;Apply user's scale
           call    NEAR PTR ovchk      ;Check for overflow

           frndint                     ;For compat with C version
           fild    QWORD PTR sum       ;Accumulate the sum
           ftst                        ;Convert neg to big pos
           fstsw   ax
           sahf
           jae     pos3
           fadd    CS:s64

pos3:      faddp
           call    NEAR PTR ovchk      ;Check for overflow
           fcom    CS:s63              ;Convert big pos to neg
           fstsw   ax
           sahf
           jb      pos4
           fsub    CS:s64
            
pos4:      fistp   QWORD PTR retval    ;Round and save the total
           fstp    st(0)               ;Pop off the scale
           fldcw   s                   ;Restore original rounding mode
           fwait

           mov     ax,OFFSET retval
           mov     dx,SEG retval
           ret                         ;Return

ovchk:     fcom    CS:s64              ;Check for overflow
           fstsw   ax
           sahf
           jb      ovchk2
           push    ec
           push    cs
           mov     ax,OFFSET amsnm
           push    ax
           call    e64act
           add     sp,6
           fstp    st(0)               ;Pop off bad data
           fld     CS:s64              ;Replace with max pos value
           fld1
           fsub
ovchk2:    retn

amsuwe     ENDP
           
           ALIGN   2
s64        DD      05f800000h          ;Exactly 2**64
s63        DD      05f000000h          ;Exactly 2**63
s32        DD      04f800000h          ;Exactly 2**32
amsnm      DB      "amsuwe"
           DB      0
           ENDS

           .DATA?
retval     ui64
           ENDS

           END

