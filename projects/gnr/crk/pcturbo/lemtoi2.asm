           %TITLE "LEMTOI2 - Little-endian memory to 2-byte int"
;
;          Module:     lemtoi2.asm
;
;          Synopsis:   short lemtoi2(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Short integer from 2 little-endian bytes at m
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
;
;          V1A, 07/05/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  lemtoi2
lemtoi2    PROC
           ARG     m:PTR
           les     bx,m            ; Mem ptr to es:bx
           mov     al,BYTE PTR es:bx
           mov     ah,BYTE PTR es:bx+1
           ret
lemtoi2    ENDP
           END
