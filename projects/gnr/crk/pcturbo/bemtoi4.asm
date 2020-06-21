           %TITLE "BEMTOI4 - Big-endian memory to 4-byte int"
;
;          Module:     bemtoi4.asm
;
;          Synopsis:   long bemtoi4(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Long integer from 4 big-endian bytes at m
;
;          V1A, 07/04/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  bemtoi4
bemtoi4    PROC
           ARG     m:PTR
           les     bx,m            ; Mem ptr to es:bx
           mov     dh,BYTE PTR es:bx
           mov     dl,BYTE PTR es:bx+1
           mov     ah,BYTE PTR es:bx+2
           mov     al,BYTE PTR es:bx+3
           ret
bemtoi4    ENDP
           END
