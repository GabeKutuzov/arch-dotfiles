           %TITLE "BEMFMI4 - Big-endian memory from 4-byte int"
;
;          Module:     bemfmi4.asm
;
;          Synopsis:   void bemfmi4(char *m, long i4)
;
;          Arguments:  m = buffer location to store data
;                      i4 = data item to be swapped and stored
;
;          Returns:    Nothing
;
;          V1A, 07/04/99, G. N. REEKE - Initial version
;
           .286
           IFNDEF  MDL
MDL        EQU     LARGE
           ENDIF
           .MODEL  MDL,C
           .CODE

           PUBLIC  bemfmi4
bemfmi4    PROC
           ARG     m:PTR,i4a:WORD,i4b:WORD
           les     bx,m            ; Mem ptr to es:bx
           mov     ax,i4a          ; Fetch low(i4)
           mov     BYTE PTR es:bx+2,ah
           mov     BYTE PTR es:bx+3,al
           mov     ax,i4b          ; Fetch high(i4)
           mov     BYTE PTR es:bx,ah
           mov     BYTE PTR es:bx+1,al
           ret
bemfmi4    ENDP
           END
