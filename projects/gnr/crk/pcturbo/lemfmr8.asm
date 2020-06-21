           %TITLE "LEMFMR8 - Little-endian memory from 8-byte double"
;
;          Module:     lemfmr8.asm
;
;          Synopsis:   void lemfmr8(char *m, double r8)
;
;          Arguments:  m = buffer location to store data
;                      r8 = data item to be stored
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
;                      Although argument r8 is a double, no transfor-
;                      mations other than moving are required on
;                      this system, therefore, r8 is manipulated in
;                      fixed-point registers.
;
;          Returns:    Nothing
;
;          V1A, 07/06/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  lemfmr8
lemfmr8    PROC
           ARG     m:PTR,r8a:WORD,r8b:WORD,r8c:WORD,r8d:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,r8a
           mov     dx,r8b
           mov     BYTE PTR es:bx,al
           mov     BYTE PTR es:bx+1,ah
           mov     BYTE PTR es:bx+2,dl
           mov     BYTE PTR es:bx+3,dh
           mov     ax,r8c
           mov     dx,r8d
           mov     BYTE PTR es:bx+4,al
           mov     BYTE PTR es:bx+5,ah
           mov     BYTE PTR es:bx+6,dl
           mov     BYTE PTR es:bx+7,dh
           ret
lemfmr8    ENDP
           END
