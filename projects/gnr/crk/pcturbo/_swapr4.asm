           %TITLE "SWAPR4 - Exchange byte order of a 4-byte float"
;
;          Module:     _swapr4.asm
;
;          Contains:   Byte-order swap routine for floats
;
;          Synopsis:   float swapr4(float)
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

           PUBLIC  swapr4
swapr4     PROC
           ARG     r4a:WORD,r4b:WORD
           mov     dx,r4a
           mov     ax,r4b
           rol     dx,8
           rol     ax,8
           mov     r4b,dx
           mov     r4a,ax
           fld     DWORD PTR r4a
           fwait
           ret
swapr4     ENDP
           END
