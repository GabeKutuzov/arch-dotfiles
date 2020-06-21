           %TITLE "BEMFMR4 - Big-endian memory from 4-byte real"
;
;          Module:     bemfmr4.asm
;
;          Synopsis:   void bemfmr4(char *m, real r4)
;
;          Arguments:  m = buffer location to store data
;                      r4 = data item to be swapped and stored
;
;          Note:       Although argument r4 is a real, no transfor-
;                      mations other than swapping are required on
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

           PUBLIC  bemfmr4
bemfmr4    PROC
           ARG     m:PTR,r4a:WORD,r4b:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,r4a          ; Fetch low(r4)
           mov     BYTE PTR es:bx+2,ah
           mov     BYTE PTR es:bx+3,al
           mov     ax,r4b          ; Fetch high(r4)
           mov     BYTE PTR es:bx,ah
           mov     BYTE PTR es:bx+1,al
           ret
bemfmr4    ENDP
           END
