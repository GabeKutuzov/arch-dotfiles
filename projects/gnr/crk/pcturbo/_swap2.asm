           %TITLE "SWAP2 - Swap the two halves of a two-byte int"
;
;          Module:     _swap2.asm
;
;          Contains:   Byte-order swap routine for short ints
;
;          Synopsis:   short swap2(short)
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

           PUBLIC  swap2
swap2      PROC
           ARG     i2:WORD
           mov     ax,i2
           rol     ax,8
           ret
swap2      ENDP
           END
