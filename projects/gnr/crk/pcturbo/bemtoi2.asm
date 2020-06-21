           %TITLE "BEMTOI2 - Big-endian memory to 2-byte int"
;
;          Module:     bemtoi2.asm
;
;          Synopsis:   short bemtoi2(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Short integer from 2 big-endian bytes at m
;
;          V1A, 07/05/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  bemtoi2
bemtoi2    PROC
           ARG     m:PTR
           les     bx,m            ; Mem ptr to es:bx
           mov     ah,BYTE PTR es:bx
           mov     al,BYTE PTR es:bx+1
           ret
bemtoi2    ENDP
           END
