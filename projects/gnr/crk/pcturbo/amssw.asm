           %TITLE "AMSSW 64-BIT MULTIPLY, SHIFT, AND ADD ROUTINE"
;**********************************************************************
;                                                                     *
;     ROCKS CRYSTALLOGRAPHIC COMPUTING SYSTEM - C IMPLEMENTATION      *
;                                                                     *
;     Synopsis:  si64 amssw(si64 sum, long x, long y, int s)          *
;                                                                     *
;     This routine multiplies two signed long integers, shifts the    *
;          product by s bits (pos=left, neg=right), then adds the     *
;          shifted product into a 64-bit accumulator and returns      *
;          the total.  There is no error checking.                    *
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
;        V1A, 07/03/99, G. N. Reeke
;
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL MDL,C
           PUBLIC  amssw

si64       STRUC                       ;Define 64-bit data item
hi         DD      ?
lo         DD      ?
si64       ENDS

           .CODE
amssw      PROC
           ARG     sum:si64,x:DWORD,y:DWORD,s:WORD
           fild    s                   ;Get user's scale
           fild    x                   ;Pick up first multiplier
           fnstcw  s                   ;Save coprocessor control word
           fimul   y                   ;Multiply

           mov     ax,SEG retval       ;Keep busy during fimul
           mov     es,ax
           ASSUME  es:SEG retval

           fwait
           mov     ax,s                ;Enable round down mode
           or      ah,04h
           mov     WORD PTR x,ax
           fldcw   WORD PTR x

           fscale                      ;Apply user's scale
           frndint                     ;For compat with C version
           fild    QWORD PTR sum       ;Accumulate the sum
           faddp                       
           fistp   QWORD PTR retval    ;Round and save the total
           fstp    st(0)               ;Pop off the scale
           fldcw   s                   ;Restore original rounding mode
           fwait

           mov     ax,OFFSET retval
           mov     dx,SEG retval
           ret                         ;Return
amssw      ENDP
           ENDS

           .DATA?
retval     si64
           ENDS

           END

