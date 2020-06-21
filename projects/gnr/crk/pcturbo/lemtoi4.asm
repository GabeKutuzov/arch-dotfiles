           %TITLE "LEMTOI4 - Little-endian memory to 4-byte int"
;
;          Module:     lemtoi4.asm
;
;          Synopsis:   long lemtoi4(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Long integer from 4 little-endian bytes at m
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
;
;          V1A, 07/04/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  lemtoi4
lemtoi4    PROC
           ARG     m:PTR
           les     bx,m            ; Mem ptr to es:bx
           mov     al,BYTE PTR es:bx
           mov     ah,BYTE PTR es:bx+1
           mov     dl,BYTE PTR es:bx+2
           mov     dh,BYTE PTR es:bx+3
           ret
lemtoi4    ENDP
           END
