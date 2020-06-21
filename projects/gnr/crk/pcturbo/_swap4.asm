           %TITLE "SWAP4 - Exchange byte order of a 4-byte int"
;
;          Module:     _swap4.asm
;
;          Contains:   Byte-order swap routine for long ints
;
;          Synopsis:   long swap4(long)
;
;          Argument:   Value to be swapped
;
;          Returns:    Swapped value in ax
;
;          V1A, 05/28/94, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  swap4
swap4      PROC
           ARG     i2a:WORD,i2b:WORD
           mov     dx,i2a
           mov     ax,i2b
           rol     dx,8
           rol     ax,8
           ret
swap4      ENDP
           END
