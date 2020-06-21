           %TITLE "LEMTOR8 - Little-endian memory to 8-byte double"
;
;          Module:     lemtor8.asm
;
;          Synopsis:   double lemtor8(char *m)
;
;          Arguments:  m = buffer location where item is stored
;
;          Returns:    Double real from 8 little-endian bytes at m
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
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

           PUBLIC  lemtor8
lemtor8    PROC
           ARG     m:PTR
           LOCAL   r8:QWORD =r8size
           les     bx,m            ; Mem ptr to es:bx
           mov     al,BYTE PTR es:bx
           mov     ah,BYTE PTR es:bx+1
           mov     dl,BYTE PTR es:bx+2
           mov     dh,BYTE PTR es:bx+3
           mov     WORD PTR r8,ax
           mov     WORD PTR r8+2,dx
           mov     al,BYTE PTR es:bx+4
           mov     ah,BYTE PTR es:bx+5
           mov     dl,BYTE PTR es:bx+6
           mov     dh,BYTE PTR es:bx+7
           mov     WORD PTR r8+4,ax
           mov     WORD PTR r8+6,dx
           fld     r8
           fwait
           ret
lemtor8    ENDP
           END
