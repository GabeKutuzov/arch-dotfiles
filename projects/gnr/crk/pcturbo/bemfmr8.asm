           %TITLE "BEMFMR8 - Big-endian memory from 8-byte double real"
;
;          Module:     bemfmr8.asm
;
;          Synopsis:   void bemfmr8(char *m, double r8)
;
;          Arguments:  m = buffer location to store data
;                      r8 = data item to be swapped and stored
;
;          Note:       Although argument r8 is a double, no transfor-
;                      mations other than swapping are required on
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

           PUBLIC  bemfmr8
bemfmr8    PROC
           ARG     m:PTR,r8a:WORD,r8b:WORD,r8c:WORD,r8d:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,r8a
           mov     dx,r8b
           mov     BYTE PTR es:bx+7,al
           mov     BYTE PTR es:bx+6,ah
           mov     BYTE PTR es:bx+5,dl
           mov     BYTE PTR es:bx+4,dh
           mov     ax,r8c
           mov     dx,r8d
           mov     BYTE PTR es:bx+3,al
           mov     BYTE PTR es:bx+2,ah
           mov     BYTE PTR es:bx+1,dl
           mov     BYTE PTR es:bx,dh
           ret
bemfmr8    ENDP
           END
