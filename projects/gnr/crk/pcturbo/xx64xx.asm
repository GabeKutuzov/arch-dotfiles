           %TITLE "XX64XX 64-BIT FIXED POINT ARITHMETIC PACKAGE"
;**********************************************************************
;                                                                     *
;     ROCKS CRYSTALLOGRAPHIC COMPUTING SYSTEM - C IMPLEMENTATION      *
;                                                                     *
;     This package contains the 64-bit fixed-point arithmetic         *
;          routines documented in 'The Rocks Routines - C             *
;          Implementation'.  This is the TURBO C version.             *
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
;        V1A, 05/03/84, G. N. Reeke
;
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL MDL,C
           .CODE

;---------------------------------------------------------------------*
;                                                                     *
;        long dm64nb(long mul1, long mul2, long div, long *rem)       *
;                                                                     *
;        Multiplies mul1 by mul2, divides the 64-bit result by div,   *
;              stores the remainder in the fourth argument,           *
;              and returns the quotient with no shifting              *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  dm64nb 
dm64nb     PROC
           ARG     dmnbm1:DWORD,dmnbm2:DWORD,dmnbdiv:DWORD,dmnbrem:PTR
           les     bx,dmnbrem          ;Remainder pointer to es:bx
           fild    dmnbm1              ;Pick up first multiplier
           fimul   dmnbm2              ;Multiply
           fidiv   dmnbdiv             ;Divide
           fld     st(0)               ;Copy result to st(1)
           fnstcw  WORD PTR dmnbm1     ;Save coprocessor control word
           fwait
           mov     ax,WORD PTR dmnbm1  ;Prepare our own
           or      ah,0ch              ;Enable chop mode
           mov     WORD PTR dmnbm1+2,ax
           fldcw   WORD PTR dmnbm1+2
           frndint                     ;Truncate to integer
           fist    dmnbm2              ;Save integer part of quotient
           fsub                        ;Calculate remainder in st(1) 
           fimul   dmnbdiv             ;Multiply fraction by divisor
           fldcw   WORD PTR dmnbm1     ;Back to caller's mode
           fistp   DWORD PTR es:bx     ;Save remainder
           fwait
           mov     ax,WORD PTR dmnbm2  ;Pick up quotient for return
           mov     dx,WORD PTR dmnbm2+2
           ret
dm64nb     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long dm64nq(long mul1, long mul2, long div)                  *
;                                                                     *
;        Multiplies mul1 by mul2, divides the 64-bit result by div,   *
;              and returns the quotient with no shifting              *
;                                                                     *
;        This routine was formerly known as mdiv                      *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  dm64nq 
dm64nq     PROC
           ARG     dmnqm1:DWORD,dmnqm2:DWORD,dmnqdiv:DWORD
           fild    dmnqm1              ;Pick up first multiplier
           fimul   dmnqm2              ;Multiply
           fidiv   dmnqdiv             ;Divide
           fnstcw  WORD PTR dmnqm1     ;Save coprocessor control word
           fwait
           mov     ax,WORD PTR dmnqm1  ;Prepare our own
           or      ah,0ch              ;Enable chop mode
           mov     WORD PTR dmnqm1+2,ax
           fldcw   WORD PTR dmnqm1+2
           fistp   dmnqm2              ;Save integer part of quotient
           fldcw   WORD PTR dmnqm1     ;Back to caller's mode
           fwait
           mov     ax,WORD PTR dmnqm2  ;Pick up quotient for return
           mov     dx,WORD PTR dmnqm2+2
           ret
dm64nq     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long dm64nr(long mul1, long mul2, long div)                  *
;                                                                     *
;        Multiplies mul1 by mul2, divides the 64-bit result by div,   *
;              and returns the remainder with no shifting             *
;                                                                     *
;        This routine is critical to the UDEV and RANNUM functions    *
;---------------------------------------------------------------------*
           PUBLIC  dm64nr 
dm64nr     PROC
           ARG     dmnrm1:DWORD,dmnrm2:DWORD,dmnrdiv:DWORD
           fild    dmnrdiv             ;Pick up divisor
           fild    dmnrm1              ;Pick up first multiplier
           fimul   dmnrm2              ;Multiply
dmnrrep:   fprem                       ;Calculate remainder exactly
           fstsw   WORD PTR dmnrm1     ;Check for completion
           test    WORD PTR dmnrm1,0400h
           jnz     dmnrrep             ;Repeat until done
           fistp   dmnrm1              ;Save remainder
           fstp    st(0)               ;Pop the divisor off
           fwait
           mov     ax,WORD PTR dmnrm1  ;Pick up remainder for return
           mov     dx,WORD PTR dmnrm1+2
           ret
dm64nr     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long ds64nq(long hi32, long lo32, int ishft, long div)       *
;                                                                     *
;        Concatenates hi32-lo32, shifts ishft bits (where ishft > 0   *
;              is a left shift), then divides by div and returns      *
;              the quotient                                           *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  ds64nq 
ds64nq     PROC
           ARG     dsnqhi:DWORD,dsnqlo:DWORD,dsnqs:WORD,dsnqdiv:DWORD
           mov     ax,WORD PTR dsnqhi  ;Interchange hi/lo args
           mov     bx,WORD PTR dsnqhi+2
           mov     cx,WORD PTR dsnqlo
           mov     dx,WORD PTR dsnqlo+2
           mov     WORD PTR dsnqhi,cx
           mov     WORD PTR dsnqhi+2,dx
           mov     WORD PTR dsnqlo,ax
           mov     WORD PTR dsnqlo+2,bx
           fild    dsnqs               ;Load the scale factor
           fild    QWORD PTR dsnqhi    ;Load the quad argument
           fscale                      ;Perform 64-bit shift
           fnstcw  dsnqs               ;Save coprocessor control word
           fwait
           mov     ax,dsnqs            ;Prepare our own
           or      ah,04h              ;Control for round down
           mov     WORD PTR dsnqlo,ax
           or      ah,0ch              ;Control word for chop
           mov     WORD PTR dsnqlo+2,ax
           fldcw   WORD PTR dsnqlo     ;Enable round down mode
           frndint                     ;Yes, we do need this!
           fidiv   dsnqdiv             ;Perform division
           fldcw   WORD PTR dsnqlo+2   ;Now to chop mode
           fistp   dsnqhi              ;Save integer part of quotient
           fstp    st(0)               ;Pop the scale factor
           fldcw   WORD PTR dsnqs      ;Back to caller's mode
           fwait
           mov     ax,WORD PTR dsnqhi  ;Pick up quotient for return
           mov     dx,WORD PTR dsnqhi+2
           ret
ds64nq     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long jm64nb(long mul1, long mul2, unsigned long *lowbits)    *
;                                                                     *
;        Multiplies mul1 by mul2 and returns both the high and low    *
;              order product                                          *
;                                                                     *
;        Implementation remark: You can't just do an                  *
;              fist  DWORD PTR es:bx  to store the low order          *
;              because it cleverly stores 2**31 to indicate overflow  *
;---------------------------------------------------------------------*
           PUBLIC  jm64nb
jm64nb     PROC
           ARG     jmnbm1:DWORD,jmnbm2:DWORD,jmnblow:PTR
           les     bx,jmnblow          ;Low pointer to es:bx
           fild    jmnbm1              ;Pick up first multiplier
           fimul   jmnbm2              ;Multiply
           fistp   QWORD PTR jmnbm1    ;Save full product
           fwait
           mov     ax,WORD PTR jmnbm1  ;Pick up low order for return
           mov     dx,WORD PTR jmnbm1+2
           mov     WORD PTR es:bx,ax
           mov     WORD PTR es:bx+2,dx
           mov     ax,WORD PTR jmnbm2  ;Pick up high order for return
           mov     dx,WORD PTR jmnbm2+2
           ret                         ;Return
jm64nb     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long jm64nh(long mul1, long mul2)                            *
;                                                                     *
;        Multiplies mul1 by mul2 and returns the high order product   *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  jm64nh 
jm64nh     PROC
           ARG     jmnhm1:DWORD,jmnhm2:DWORD
           fild    jmnhm1              ;Pick up first multiplier
           fimul   jmnhm2              ;Multiply
           fistp   QWORD PTR jmnhm1    ;Save full product
           fwait
           mov     ax,WORD PTR jmnhm2  ;Pick up high order for return
           mov     dx,WORD PTR jmnhm2+2
           ret                         ;Return
jm64nh     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long jm64sb(long mul1, long mul2, int ishft,                 *
;              unsigned long *lowbits)                                *
;                                                                     *
;        Multiplies mul1 by mul2, shifts the result by ishft,         *
;              and returns both high and low order product            *
;                                                                     *
;        Note that the 3 jm64sx routines must go to round down mod    *
;              because a right shift might produce a fraction that    *
;              would otherwise incorrectly round up the result.       *
;---------------------------------------------------------------------*
           PUBLIC  jm64sb 
jm64sb     PROC
           ARG     jmsbm1:DWORD,jmsbm2:DWORD,jmsbs:WORD,jmsblow:PTR
           les     bx,jmsblow          ;Low pointer to es:bx
           fild    jmsbs               ;Get user's scale
           fild    jmsbm1              ;Pick up first multiplier
           fimul   jmsbm2              ;Multiply
           fscale                      ;Apply user's scale
           fnstcw  jmsbs               ;Save coprocessor control word
           fwait
           mov     ax,jmsbs            ;Prepare our own
           or      ah,04h              ;Enable round down mode
           mov     WORD PTR dmnbm1,ax
           fldcw   WORD PTR dmnbm1
           fistp   QWORD PTR jmsbm1    ;Save entire product
           fstp    st(0)               ;Pop off the scale
           fldcw   jmsbs
           fwait
           mov     ax,WORD PTR jmsbm1  ;Pick up low order for return
           mov     dx,WORD PTR jmsbm1+2
           mov     WORD PTR es:bx,ax
           mov     WORD PTR es:bx+2,dx
           mov     ax,WORD PTR jmsbm2  ;Pick up high order for return
           mov     dx,WORD PTR jmsbm2+2
           ret                         ;Return
jm64sb     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long jm64sh(long mul1, long mul2, int ishft)                 *
;                                                                     *
;        Multiplies mul1 by mul2, shifts the result by ishft,         *
;              and returns the high order product                     *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  jm64sh 
jm64sh     PROC
           ARG     jmshm1:DWORD,jmshm2:DWORD,jmshs:WORD
           fild    jmshs               ;Get user's scale
           fild    jmshm1              ;Pick up first multiplier
           fimul   jmshm2              ;Multiply
           fscale                      ;Apply scale
           fnstcw  jmshs               ;Save coprocessor control word
           fwait
           mov     ax,jmshs            ;Prepare our own
           or      ah,04h              ;Enable round down mode
           mov     WORD PTR jmshm1,ax
           fldcw   WORD PTR jmshm1
           fistp   QWORD PTR jmshm1    ;Save entire product
           fldcw   jmshs               ;Restore caller's rounding mode
           fstp    st(0)               ;Pop the scale
           fwait
           mov     ax,WORD PTR jmshm2  ;Pick up high order for return
           mov     dx,WORD PTR jmshm2+2
           ret                         ;Return
jm64sh     ENDP

           %NEWPAGE
;---------------------------------------------------------------------*
;                                                                     *
;        long jm64sl(long mul1, long mul2, int ishft)                 *
;                                                                     *
;        Multiplies mul1 by mul2, shifts the result by ishft,         *
;              and returns the low order part of the product          *
;                                                                     *
;        This routine replaces mshft, but differs in that the         *
;              shift argument has been rationalized to agree          *
;              with the other routines in this set                    *
;                                                                     *
;---------------------------------------------------------------------*
           PUBLIC  jm64sl 
jm64sl     PROC
           ARG     jmslm1:DWORD,jmslm2:DWORD,jmsls:WORD
           fild    jmsls               ;Get user's scale
           fild    jmslm1              ;Pick up first multiplier
           fimul   jmslm2              ;Multiply
           fscale                      ;Apply user's scale
           fnstcw  jmsls               ;Save coprocessor control word
           fwait
           mov     ax,jmsls            ;Prepare our own
           or      ah,04h              ;Enable round down mode
           mov     WORD PTR jmslm1,ax
           fldcw   WORD PTR jmslm1
           fistp   QWORD PTR jmslm1    ;Save entire result
           fldcw   jmsls               ;Restore caller's rounding mode
           fstp    st(0)               ;Pop scale
           fwait
           mov     ax,WORD PTR jmslm1  ;Pick up low order for return
           mov     dx,WORD PTR jmslm1+2
           ret                         ;Return
jm64sl     ENDP
           END
