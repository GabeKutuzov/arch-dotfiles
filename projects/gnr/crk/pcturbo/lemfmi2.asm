           %TITLE "LEMFMI2 - Little-endian memory from 2-byte int"
;
;          Module:     lemfmi2.asm
;
;          Synopsis:   void lemfmi2(char *m, short i2)
;
;          Arguments:  m = buffer location to store data
;                      i2 = data item to be stored
;
;          Returns:    Nothing
;
;          Note:       There is no swapping here, but item is
;                      moved byte-by-byte to handle misaligned m.
;
;          V1A, 07/05/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  lemfmi2
lemfmi2    PROC
           ARG     m:PTR,i2:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,i2           ; Fetch i2
           mov     BYTE PTR es:bx,al
           mov     BYTE PTR es:bx+1,ah
           ret
lemfmi2    ENDP
           END
