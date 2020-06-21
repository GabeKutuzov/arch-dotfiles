           %TITLE "LEMFMR4 - Little-endian memory from 4-byte real"
;
;          Module:     lemfmr4.asm
;
;          Synopsis:   void lemfmr4(char *m, real r4)
;
;          Arguments:  m = buffer location to store data
;                      r4 = data item to be stored
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
;                      Although argument r4 is a real, no transfor-
;                      mations other than moving are required on
;                      this system, therefore, r4 is manipulated in
;                      fixed-point registers.
;
;          Returns:    Nothing
;
;          V1A, 07/05/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  lemfmr4
lemfmr4    PROC
           ARG     m:PTR,r4a:WORD,r4b:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,r4a          ; Fetch low(r4)
           mov     dx,r4b          ; Fetch high(r4)
           mov     BYTE PTR es:bx,al
           mov     BYTE PTR es:bx+1,ah
           mov     BYTE PTR es:bx+2,dl
           mov     BYTE PTR es:bx+3,dh
           ret
lemfmr4    ENDP
           END
