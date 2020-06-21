           %TITLE "BEMTOR8 - Big-endian memory to 8-byte double real"
;
;          Module:     bemtor8.asm
;
;          Synopsis:   double bemtor8(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Double real from 8 big-endian bytes at m
;
;          V1A, 07/06/99, G. N. REEKE - Initial version
;
           .286
           .287
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  bemtor8
bemtor8    PROC
           ARG     m:PTR
           LOCAL   r8:QWORD =r8size
           les     bx,m            ; Mem ptr to es:bx
           mov     dh,BYTE PTR es:bx
           mov     dl,BYTE PTR es:bx+1
           mov     ah,BYTE PTR es:bx+2
           mov     al,BYTE PTR es:bx+3
           mov     WORD PTR r8+4,ax
           mov     WORD PTR r8+6,dx
           mov     dh,BYTE PTR es:bx+4
           mov     dl,BYTE PTR es:bx+5
           mov     ah,BYTE PTR es:bx+6
           mov     al,BYTE PTR es:bx+7
           mov     WORD PTR r8,ax
           mov     WORD PTR r8+2,dx
           fld     r8
           fwait
           ret
bemtor8    ENDP
           END
