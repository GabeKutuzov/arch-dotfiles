           %TITLE "AMSSWE 64-BIT MULTIPLY, SHIFT, AND ADD ROUTINE"
;**********************************************************************
;                                                                     *
;     ROCKS CRYSTALLOGRAPHIC COMPUTING SYSTEM - C IMPLEMENTATION      *
;                                                                     *
;     Synopsis:  si64 amsswe(si64 sum, long x, long y, int s, int ec) *
;                                                                     *
;     This routine multiplies two signed long integers, shifts the    *
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
           PUBLIC  amsswe

si64       STRUC                       ;Define 64-bit data item
hi         DD      ?
lo         DD      ?
si64       ENDS

           .CODE
amsswe     PROC
           ARG     sum:si64,x:DWORD,y:DWORD,s:WORD,ec:WORD
           fild    s                   ;Get user's scale
           fild    x                   ;Pick up first multiplier
           fnstcw  s                   ;Save coprocessor control word
           fimul   y                   ;Multiply--overflow not possible
           fwait
           mov     ax,s                ;Enable round down mode
           or      ah,04h
           mov     WORD PTR x,ax
           fldcw   WORD PTR x

           fscale                      ;Apply user's scale
           call    NEAR PTR ovchk      ;Check for overflow

           frndint                     ;For compat with C version
           fild    QWORD PTR sum       ;Accumulate the sum
           faddp
           call    NEAR PTR ovchk      ;Check for overflow
            
           mov     ax,SEG retval       ;Locate result struct
           mov     es,ax
           ASSUME  es:SEG retval
           fistp   QWORD PTR retval    ;Round and save the total
           fstp    st(0)               ;Pop off the scale
           fldcw   s                   ;Restore original rounding mode
           fwait

           mov     ax,OFFSET retval
           mov     dx,SEG retval
           ret                         ;Return

ovchk:     fld     CS:s64mx            ;Check for overflow
           fcom
           fstsw   ax
           sahf
           ja      ovchk2
           push    ec
           push    cs
           mov     ax,OFFSET amsnm
           push    ax
           call    e64act
           add     sp,6
           fstp    st(0)               ;Pop off overflow test data
           fstp    st(0)
           fld     CS:s64mx
           fld1
           fsub
           retn
ovchk2:    fchs
           fcomp
           fstsw   ax
           sahf
           jbe     ovchk3
           push    ec
           push    cs
           mov     ax,OFFSET amsnm
           push    ax
           call    e64act
           add     sp,6
           fstp    st(0)               ;Pop off overflow test data
           fld     CS:s64mx
           fchs
ovchk3:    retn

amsswe     ENDP
           
           ALIGN   2
s64mx      DD      05f000000h          ;Exactly 2^63
amsnm      DB      "amsswe"
           DB      0
           ENDS

           .DATA?
retval     si64
           ENDS

           END

