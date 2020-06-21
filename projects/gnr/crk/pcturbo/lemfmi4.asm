           %TITLE "LEMFMI4 - Little-endian memory from 4-byte int"
;
;          Module:     lemfmi4.asm
;
;          Synopsis:   void lemfmi4(char *m, long i4)
;
;          Arguments:  m = buffer location to store data
;                      i4 = data item to be stored
;
;          Returns:    Nothing
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

           PUBLIC  lemfmi4
lemfmi4    PROC
           ARG     m:PTR,i4a:WORD,i4b:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,i4a          ; Fetch low(i4)
           mov     dx,i4b          ; Fetch high(i4)
           mov     BYTE PTR es:bx,al
           mov     BYTE PTR es:bx+1,ah
           mov     BYTE PTR es:bx+2,dl
           mov     BYTE PTR es:bx+3,dh
           ret
lemfmi4    ENDP
           END
