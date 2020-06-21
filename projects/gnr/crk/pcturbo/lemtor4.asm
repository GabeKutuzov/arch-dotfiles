           %TITLE "LEMTOR4 - Little-endian memory to 4-byte real"
;
;          Module:     lemtor4.asm
;
;          Synopsis:   real lemtor4(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Real (float) from 4 little-endian bytes at m
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
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

           PUBLIC  lemtor4
lemtor4    PROC
           ARG     m:PTR
           LOCAL   r4:DWORD =r4size
           les     bx,m            ; Mem ptr to es:bx
           mov     al,BYTE PTR es:bx
           mov     ah,BYTE PTR es:bx+1
           mov     dl,BYTE PTR es:bx+2
           mov     dh,BYTE PTR es:bx+3
           mov     WORD PTR r4,ax
           mov     WORD PTR r4+2,dx
           fld     r4
           fwait
           ret
lemtor4    ENDP
           END
