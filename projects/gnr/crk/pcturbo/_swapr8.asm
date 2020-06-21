           %TITLE "SWAPR8 - Exchange byte order of an 8-byte float"
;
;          Module:     _swapr8.asm
;
;          Contains:   Byte-order swap routine for double floats
;
;          Synopsis:   double swapr8(double)
;
;          Argument:   Value to be swapped
;
;          Returns:    Swapped value in floating st(0)
;
;          V1A, 05/28/94, G. N. REEKE - Initial version
;
           .286
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  swapr8
swapr8     PROC
           ARG     r8a:WORD,r8b:WORD,r8c:WORD,r8d:WORD
           mov     dx,r8a
           mov     ax,r8d
           rol     dx,8
           rol     ax,8
           mov     r8d,dx
           mov     r8a,ax
           mov     dx,r8b
           mov     ax,r8c
           rol     dx,8
           rol     ax,8
           mov     r8c,dx
           mov     r8b,ax
           fld     QWORD PTR r8a
           fwait
           ret
swapr8     ENDP
           END
