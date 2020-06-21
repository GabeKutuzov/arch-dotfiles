           %TITLE "BEMFMI2 - Big-endian memory from 2-byte int"
;
;          Module:     bemfmi2.asm
;
;          Synopsis:   void bemfmi2(char *m, short i2)
;
;          Arguments:  m = buffer location to store data
;                      i2 = data item to be swapped and stored
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

           PUBLIC  bemfmi2
bemfmi2    PROC
           ARG     m:PTR,i2:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,i2           ; Fetch i2
           mov     BYTE PTR es:bx,ah
           mov     BYTE PTR es:bx+1,al
           ret
bemfmi2    ENDP
           END
