           %TITLE "BEMTOR4 - Big-endian memory to 4-byte real"
;
;          Module:     bemtor4.asm
;
;          Synopsis:   real bemtor4(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Real (float) from 4 big-endian bytes at m
;
;          V1A, 07/05/99, G. N. REEKE - Initial version
;
           .286
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  bemtor4
bemtor4    PROC
           ARG     m:PTR
           LOCAL   r4:DWORD =r4size
           les     bx,m            ; Mem ptr to es:bx
           mov     dh,BYTE PTR es:bx
           mov     dl,BYTE PTR es:bx+1
           mov     ah,BYTE PTR es:bx+2
           mov     al,BYTE PTR es:bx+3
           mov     WORD PTR r4,ax
           mov     WORD PTR r4+2,dx
           fld     r4
           fwait
           ret
bemtor4    ENDP
           END
