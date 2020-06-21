;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright 2007 by Jack Lloyd, All Rights Reserved
;                   jack@stormyskies.net
;
; FILE: nas_testing.nas
; DESC: Unit test for NAS
;
; REVISION HISTORY (Date, Modifier: Description)
; ----------------------------------------------
;
; 2007-08-22, Jack Lloyd:
; Creation.
;
; 2007-10-07, Jack Lloyd:
; [2.x] New instructions and better organization.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

defNum Zero  0
defNum ff    0xff
defNum 3ff   0x3ff
defNum fff   0xfff
defNum ffff  0xffff
defNum fffff 0xfffff

DuplicateLabelThatsNeverUsed: ; should generate a warning
; DuplicateLabelThatsNeverUsed: ; should generate an error if uncommented

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with no arguments.  Ensure the correct opcode is generated.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PreUseLabelDeclaration:

;; NOP                  000000000000000000
        nop                                                                     ;; ADDR_000 0x00000
            ; should generate 0x00000

;; CLRP                 000001000000000000
        clrp                                                                    ;; ADDR_001 0x01000
            ; should generate 0x01000

;; PUSHP                010000000000000000
        pushp                                                                   ;; ADDR_002 0x10000
            ; should generate 0x10000

;; POPP                 010001000000000000
        popp                                                                    ;; ADDR_003 0x11000
            ; should generate 0x11000

;; TESTP                010010000000000000
        testp                                                                   ;; ADDR_004 0x12000
            ; should generate 0x12000

;; SHFTP                010011000000000000
        shftp                                                                   ;; ADDR_005 0x13000
            ; should generate 0x13000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with just one register argument.  Ensure the correct opcode is
;;; generated and range-test the single register argument.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; ADDACC REGx          00 0100 RxRx 00000000
        addacc reg0                                                             ;; ADDR_006 0x04000
            ; should generate 0x04000
        addacc regf                                                             ;; ADDR_007 0x04f00
            ; should generate 0x04f00

;; SUBTACC REGx         000101RxRx00000000
        subtacc reg0                                                            ;; ADDR_008 0x05000
            ; should generate 0x05000
        subtacc regf                                                            ;; ADDR_009 0x05f00
            ; should generate 0x05f00

;; INC REGx             010100RxRx00000000
        inc reg0                                                                ;; ADDR_010 0x14000
            ; should generate 0x14000
        inc regf                                                                ;; ADDR_011 0x14f00
            ; should generate 0x14f00

;; DEC REGx             010110RxRx00000000
        dec reg0                                                                ;; ADDR_012 0x16000
            ; should generate 0x16000
        dec regf                                                                ;; ADDR_013 0x16f00
            ; should generate 0x16f00

;; SAVEPLO REGx         011001RxRx00000000
        saveplo reg0                                                            ;; ADDR_014 0x19000
            ; should generate 0x19000
        saveplo regf                                                            ;; ADDR_015 0x19f00
            ; should generate 0x19f00

;; SAVEPMD REGx         011010RxRx00000000
        savepmd reg0                                                            ;; ADDR_016 0x1a000
            ; should generate 0x1a000
        savepmd regf                                                            ;; ADDR_017 0x1af00
            ; should generate 0x1af00

;; SAVEPHI REGx         011011RxRx00000000
        savephi reg0                                                            ;; ADDR_018 0x1b000
            ; should generate 0x1b000
        savephi regf                                                            ;; ADDR_019 0x1bf00
            ; should generate 0x1bf00

;; SHFTAR REGx          110001RxRx00000000
        shftar reg0                                                             ;; ADDR_020 0x31000
            ; should generate 0x31000
        shftar regf                                                             ;; ADDR_021 0x31f00
            ; should generate 0x31f00

;; SHFTLL REGx          110010RxRx00000000
        shftll reg0                                                             ;; ADDR_022 0x32000
            ; should generate 0x32000
        shftll regf                                                             ;; ADDR_023 0x32f00
            ; should generate 0x32f00

;; SHFTLR REGx          110011RxRx00000000
        shftlr reg0                                                             ;; ADDR_024 0x33000
            ; should generate 0x33000
        shftlr regf                                                             ;; ADDR_025 0x33f00
            ; should generate 0x33f00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with two register arguments.  Ensure the correct opcode is
;;; generated and range-test both register arguments.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; MULT REGx REGy       000110RxRxRyRy0000
        mult reg0 reg0                                                          ;; ADDR_026 0x06000
            ; should generate 0x06000
        mult reg0 regf                                                          ;; ADDR_027 0x060f0
            ; should generate 0x060f0
        mult regf reg0                                                          ;; ADDR_028 0x06f00
            ; should generate 0x06f00
        mult regf regf                                                          ;; ADDR_029 0x06ff0
            ; should generate 0x06ff0

;; MULTACC REGx REGy    000111RxRxRyRy0000
        multacc reg0 reg0                                                       ;; ADDR_030 0x07000
            ; should generate 0x07000
        multacc reg0 regf                                                       ;; ADDR_031 0x070f0
            ; should generate 0x070f0
        multacc regf reg0                                                       ;; ADDR_032 0x07f00
            ; should generate 0x07f00
        multacc regf regf                                                       ;; ADDR_033 0x07ff0
            ; should generate 0x07ff0

;; LOAD REGx REGy       001000RxRxRyRy0000
        load reg0 reg0                                                          ;; ADDR_034 0x08000
            ; should generate 0x08000
        load reg0 regf                                                          ;; ADDR_035 0x080f0
            ; should generate 0x080f0
        load regf reg0                                                          ;; ADDR_036 0x08f00
            ; should generate 0x08f00
        load regf regf                                                          ;; ADDR_037 0x08ff0
            ; should generate 0x08ff0

;; COMPARE REGx REGy    001010RxRxRyRy0000
        compare reg0 reg0                                                       ;; ADDR_038 0x0a000
            ; should generate 0x0a000
        compare reg0 regf                                                       ;; ADDR_039 0x0a0f0
            ; should generate 0x0a0f0
        compare regf reg0                                                       ;; ADDR_040 0x0af00
            ; should generate 0x0af00
        compare regf regf                                                       ;; ADDR_041 0x0aff0
            ; should generate 0x0aff0

;; STASH REGx (REGy)    001110RxRxRyRy0000
        stash reg0 reg0                                                         ;; ADDR_042 0x0e000
            ; should generate 0x0e000
        stash reg0 regf                                                         ;; ADDR_043 0x0e0f0
            ; should generate 0x0e0f0
        stash regf reg0                                                         ;; ADDR_044 0x0ef00
            ; should generate 0x0ef00
        stash regf regf                                                         ;; ADDR_045 0x0eff0
            ; should generate 0x0eff0

;; OR REGx REGy         101000RxRxRyRy0000
        or reg0 reg0                                                            ;; ADDR_046 0x28000
            ; should generate 0x28000
        or reg0 regf                                                            ;; ADDR_047 0x280f0
            ; should generate 0x280f0
        or regf reg0                                                            ;; ADDR_048 0x28f00
            ; should generate 0x28f00
        or regf regf                                                            ;; ADDR_049 0x28ff0
            ; should generate 0x28ff0

;; AND REGx REGy        101010RxRxRyRy0000
        and reg0 reg0                                                           ;; ADDR_050 0x2a000
            ; should generate 0x2a000
        and reg0 regf                                                           ;; ADDR_051 0x2a0f0
            ; should generate 0x2a0f0
        and regf reg0                                                           ;; ADDR_052 0x2af00
            ; should generate 0x2af00
        and regf regf                                                           ;; ADDR_053 0x2aff0
            ; should generate 0x2aff0

;; FETCH REGx (REGy)    101110RxRxRyRy0000
        fetch reg0 reg0                                                         ;; ADDR_054 0x2e000
            ; should generate 0x2e000
        fetch reg0 regf                                                         ;; ADDR_055 0x2e0f0
            ; should generate 0x2e0f0
        fetch regf reg0                                                         ;; ADDR_056 0x2ef00
            ; should generate 0x2ef00
        fetch regf regf                                                         ;; ADDR_057 0x2eff0
            ; should generate 0x2eff0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with just one 8-bit numeric literal argument.  Ensure the
;;; correct opcode is generated and range test the 8-bit numeric in both raw and
;;; label-expansion forms.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; IMM num              0000100000cccccccc
        imm 0x0                                                                 ;; ADDR_058 0x02000
            ; should generate 0x02000
        imm 255                                                                 ;; ADDR_059 0x020ff
            ; should generate 0x020ff
        imm 0377                                                                ;; ADDR_060 0x020ff
            ; should generate 0x020ff
        imm 0xfff                                                               ;; ADDR_061 0x020ff
            ; should generate 0x020ff and a warning about truncation

;; IMM label            0000100000cccccccc
        imm Zero                                                                ;; ADDR_062 0x02000
            ; should generate 0x02000
        imm ff                                                                  ;; ADDR_063 0x020ff
            ; should generate 0x020ff
        imm fff                                                                 ;; ADDR_064 0x020ff
            ; should generate 0x020ff and a warning about truncation

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with just one 10-bit numeric literal argument.  Ensure the
;;; correct opcode is generated and range test the 10-bit numeric in both raw
;;; and label-expansion forms.  These instructions also branch, so test backward-
;;; and forward-label references.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; JUMPCY num           10000000iaiaiaiaia
        jumpcy 0x0                                                              ;; ADDR_065 0x20000
            ; should generate 0x20000
        jumpcy 255                                                              ;; ADDR_066 0x200ff
            ; should generate 0x200ff
        jumpcy 0377                                                             ;; ADDR_067 0x200ff
            ; should generate 0x200ff
        jumpcy 0x3ff                                                            ;; ADDR_068 0x203ff
            ; should generate 0x203ff
        jumpcy 0xfff                                                            ;; ADDR_069 0x20fff
            ; should generate 0x20fff and a warning

;; JUMPCY label         10000000iaiaiaiaia
        jumpcy Zero                                                             ;; ADDR_070 0x20000
            ; should generate 0x20000
        jumpcy ff                                                               ;; ADDR_071 0x200ff
            ; should generate 0x200ff
        jumpcy 3ff                                                              ;; ADDR_072 0x203ff
            ; should generate 0x203ff
        jumpcy fff                                                              ;; ADDR_073 0x20fff
            ; should generate 0x20fff
        jumpcy PreUseLabelDeclaration                                           ;; ADDR_074 0x20000
            ; should generate 0x20???
        jumpcy PostUseLabelDeclaration                                          ;; ADDR_075 0x20093
            ; should generate 0x20???

;; JUMPN num            10000100iaiaiaiaia
        jumpn 0x0                                                               ;; ADDR_076 0x21000
            ; should generate 0x21000
        jumpn 255                                                               ;; ADDR_077 0x210ff
            ; should generate 0x210ff
        jumpn 0377                                                              ;; ADDR_078 0x210ff
            ; should generate 0x210ff
        jumpn 0x3ff                                                             ;; ADDR_079 0x213ff
            ; should generate 0x213ff
        jumpn 0xfff                                                             ;; ADDR_080 0x21fff
            ; should generate 0x21fff and a warning

;; JUMPN label          10000100iaiaiaiaia
        jumpn Zero                                                              ;; ADDR_081 0x21000
            ; should generate 0x21000
        jumpn ff                                                                ;; ADDR_082 0x210ff
            ; should generate 0x210ff
        jumpn 3ff                                                               ;; ADDR_083 0x213ff
            ; should generate 0x213ff
        jumpn fff                                                               ;; ADDR_084 0x21fff
            ; should generate 0x21fff and a warning about truncation
        jumpn  PreUseLabelDeclaration                                           ;; ADDR_085 0x21000
            ; should generate 0x21???
        jumpn  PostUseLabelDeclaration                                          ;; ADDR_086 0x21093
            ; should generate 0x21???

;; JUMPNZ num           10001000iaiaiaiaia
        jumpnz 0x0                                                              ;; ADDR_087 0x22000
            ; should generate 0x22000
        jumpnz 255                                                              ;; ADDR_088 0x220ff
            ; should generate 0x220ff
        jumpnz 0377                                                             ;; ADDR_089 0x220ff
            ; should generate 0x220ff
        jumpnz 0x3ff                                                            ;; ADDR_090 0x223ff
            ; should generate 0x223ff
        jumpnz 0xfff                                                            ;; ADDR_091 0x22fff
            ; should generate 0x22fff and a warning

;; JUMPNZ label          10001100iaiaiaiaia
        jumpnz Zero                                                             ;; ADDR_092 0x22000
            ; should generate 0x22000
        jumpnz ff                                                               ;; ADDR_093 0x220ff
            ; should generate 0x220ff
        jumpnz 3ff                                                              ;; ADDR_094 0x223ff
            ; should generate 0x223ff
        jumpnz fff                                                              ;; ADDR_095 0x22fff
            ; should generate 0x22fff and a warning about truncation
        jumpnz PreUseLabelDeclaration                                           ;; ADDR_096 0x22000
            ; should generate 0x22???
        jumpnz PostUseLabelDeclaration                                          ;; ADDR_097 0x22093
            ; should generate 0x22???

;; JUMPZ num           10001100iaiaiaiaia
        jumpz 0x0                                                               ;; ADDR_098 0x23000
            ; should generate 0x23000
        jumpz 255                                                               ;; ADDR_099 0x230ff
            ; should generate 0x230ff
        jumpz 0377                                                              ;; ADDR_100 0x230ff
            ; should generate 0x230ff
        jumpz 0x3ff                                                             ;; ADDR_101 0x233ff
            ; should generate 0x233ff
        jumpz 0xfff                                                             ;; ADDR_102 0x23fff
            ; should generate 0x23fff and a warning

;; JUMPZ label          10001100iaiaiaiaia
        jumpz Zero                                                              ;; ADDR_103 0x23000
            ; should generate 0x23000
        jumpz ff                                                                ;; ADDR_104 0x230ff
            ; should generate 0x230ff
        jumpz 3ff                                                               ;; ADDR_105 0x233ff
            ; should generate 0x233ff
        jumpz fff                                                               ;; ADDR_106 0x23fff
            ; should generate 0x23fff and a warning about truncation
        jumpz  PreUseLabelDeclaration                                           ;; ADDR_107 0x23000
            ; should generate 0x23???
        jumpz  PostUseLabelDeclaration                                          ;; ADDR_108 0x23093
            ; should generate 0x23???

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with one register argument and one 8-bit numeric literal
;;; argument.  Ensure the correct opcode is generated, range test the register
;;; argument, and range test the 8-bit numeric in both raw and label-expansion
;;; forms.  These instructions also have wide variants which allow for 12-bits,
;;; so test everything with those, too.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; FETCH REGx num       101111RxRxaaaaaaaa
        fetch reg0 0x0                                                          ;; ADDR_109 0x2f000
            ; should generate 0x2f000
        fetch regf 0x0                                                          ;; ADDR_110 0x2ff00
            ; should generate 0x2ff00
        fetch reg0 255                                                          ;; ADDR_111 0x2f0ff
            ; should generate 0x2f0ff
        fetch regf 255                                                          ;; ADDR_112 0x2ffff
            ; should generate 0x2ffff
        fetch reg0 0377                                                         ;; ADDR_113 0x2f0ff
            ; should generate 0x2f0ff
        fetch regf 0377                                                         ;; ADDR_114 0x2ffff
            ; should generate 0x2ffff
        fetch reg0 0xfff                                                        ;; ADDR_115 0x2f0ff
            ; should generate 0x2f0ff and a warning about truncation
        fetch regf 0xfff                                                        ;; ADDR_116 0x2ffff
            ; should generate 0x2ffff and a warning about truncation

;; FETCH REGx label     101111RxRxaaaaaaaa
        fetch reg0 Zero                                                         ;; ADDR_117 0x2f000
            ; should generate 0x2f000
        fetch regf Zero                                                         ;; ADDR_118 0x2ff00
            ; should generate 0x2ff00
        fetch reg0 ff                                                           ;; ADDR_119 0x2f0ff
            ; should generate 0x2f0ff
        fetch regf ff                                                           ;; ADDR_120 0x2ffff
            ; should generate 0x2ffff
        fetch reg0 fff                                                          ;; ADDR_121 0x2f0ff
            ; should generate 0x2f0ff and a warning about truncation
        fetch regf fff                                                          ;; ADDR_122 0x2ffff
            ; should generate 0x2ffff and a warning about truncation

;; WFETCH REGx num      101111RxRxaaaaaaaa
        wfetch reg0 0x0                                                         ;; ADDR_123 0x02000 ADDR_124 0x2f000
            ; should generate 0x02000 then 0x2f000
        wfetch regf 0x0                                                         ;; ADDR_125 0x02000 ADDR_126 0x2ff00
            ; should generate 0x02000 then 0x2ff00
        wfetch reg0 0xfff                                                       ;; ADDR_127 0x0200f ADDR_128 0x2f0ff
            ; should generate 0x0200f then 0x2f0ff
        wfetch regf 0xfff                                                       ;; ADDR_129 0x0200f ADDR_130 0x2ffff
            ; should generate 0x0200f then 0x2ffff
        wfetch reg0 0xffff                                                      ;; ADDR_131 0x0200f ADDR_132 0x2f0ff
            ; should generate 0x0200f then 0x2f0ff and a warning about truncation
        wfetch regf 0xffff                                                      ;; ADDR_133 0x0200f ADDR_134 0x2ffff
            ; should generate 0x0200f then 0x2ffff and a warning about truncation

;; WFETCH REGx label    101111RxRxaaaaaaaa
        wfetch reg0 Zero                                                        ;; ADDR_135 0x02000 ADDR_136 0x2f000
            ; should generate 0x02000 then 0x2f000
        wfetch regf Zero                                                        ;; ADDR_137 0x02000 ADDR_138 0x2ff00
            ; should generate 0x02000 then 0x2ff00
        wfetch reg0 fff                                                         ;; ADDR_139 0x0200f ADDR_140 0x2f0ff
            ; should generate 0x0200f then 0x2f0ff
        wfetch regf fff                                                         ;; ADDR_141 0x0200f ADDR_142 0x2ffff
            ; should generate 0x0200f then 0x2ffff
        wfetch reg0 ffff                                                        ;; ADDR_143 0x0200f ADDR_144 0x2f0ff
            ; should generate 0x0200f then 0x2f0ff and a warning about truncation
        wfetch regf ffff                                                        ;; ADDR_145 0x0200f ADDR_146 0x2ffff
            ; should generate 0x0200f then 0x2ffff and a warning about truncation

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Instructions with one register argument and one 8-bit numeric literal
;;; argument.  Ensure the correct opcode is generated, range test the register
;;; argument, and range test the 8-bit numeric in both raw and label-expansion
;;; forms.  These instructions also have wide variants which allow for 16-bits,
;;; so test everything with those, too.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PostUseLabelDeclaration :

;; OR REGx num          101001RxRxcccccccc
        or reg0 0x0                                                             ;; ADDR_147 0x29000
            ; should generate 0x29000
        or regf 0x0                                                             ;; ADDR_148 0x29f00
            ; should generate 0x29f00
        or reg0 255                                                             ;; ADDR_149 0x290ff
            ; should generate 0x290ff
        or regf 255                                                             ;; ADDR_150 0x29fff
            ; should generate 0x29fff
        or reg0 0377                                                            ;; ADDR_151 0x290ff
            ; should generate 0x290ff
        or regf 0377                                                            ;; ADDR_152 0x29fff
            ; should generate 0x29fff
        or reg0 0xfff                                                           ;; ADDR_153 0x290ff
            ; should generate 0x290ff and a warning about truncation
        or regf 0xfff                                                           ;; ADDR_154 0x29fff
            ; should generate 0x29fff and a warning about truncation

;; OR REGx label        101001RxRxcccccccc
        or reg0 Zero                                                            ;; ADDR_155 0x29000
            ; should generate 0x29000
        or regf Zero                                                            ;; ADDR_156 0x29f00
            ; should generate 0x29f00
        or reg0 ff                                                              ;; ADDR_157 0x290ff
            ; should generate 0x290ff
        or regf ff                                                              ;; ADDR_158 0x29fff
            ; should generate 0x29fff
        or reg0 fff                                                             ;; ADDR_159 0x290ff
            ; should generate 0x290ff and a warning about truncation
        or regf fff                                                             ;; ADDR_160 0x29fff
            ; should generate 0x29fff and a warning about truncation

;; WOR REGx num         101001RxRxcccccccc
        wor reg0 0x0                                                            ;; ADDR_161 0x02000 ADDR_162 0x29000
            ; should generate 0x02000 then 0x29000
        wor regf 0x0                                                            ;; ADDR_163 0x02000 ADDR_164 0x29f00
            ; should generate 0x02000 then 0x29f00
        wor reg0 0xffff                                                         ;; ADDR_165 0x020ff ADDR_166 0x290ff
            ; should generate 0x020ff then 0x290ff
        wor regf 0xffff                                                         ;; ADDR_167 0x020ff ADDR_168 0x29fff
            ; should generate 0x020ff then 0x29fff
        wor reg0 0xfffff                                                        ;; ADDR_169 0x020ff ADDR_170 0x290ff
            ; should generate 0x020ff then 0x290ff and a warning about truncation
        wor regf 0xfffff                                                        ;; ADDR_171 0x020ff ADDR_172 0x29fff
            ; should generate 0x020ff then 0x29fff and a warning about truncation

;; WOR REGx label       101001RxRxcccccccc
        wor reg0 Zero                                                           ;; ADDR_173 0x02000 ADDR_174 0x29000
            ; should generate 0x02000 then 0x29000
        wor regf Zero                                                           ;; ADDR_175 0x02000 ADDR_176 0x29f00
            ; should generate 0x02000 then 0x29f00
        wor reg0 ffff                                                           ;; ADDR_177 0x020ff ADDR_178 0x290ff
            ; should generate 0x020ff then 0x290ff
        wor regf ffff                                                           ;; ADDR_179 0x020ff ADDR_180 0x29fff
            ; should generate 0x020ff then 0x29fff
        wor reg0 fffff                                                          ;; ADDR_181 0x020ff ADDR_182 0x290ff
            ; should generate 0x020ff then 0x290ff and a warning about truncation
        wor regf fffff                                                          ;; ADDR_183 0x020ff ADDR_184 0x29fff
            ; should generate 0x020ff then 0x29fff and a warning about truncation

;; AND REGx num         101011RxRxcccccccc
        and reg0 0x0                                                            ;; ADDR_185 0x2b000
            ; should generate 0x2b000
        and regf 0x0                                                            ;; ADDR_186 0x2bf00
            ; should generate 0x2bf00
        and reg0 255                                                            ;; ADDR_187 0x2b0ff
            ; should generate 0x2b0ff
        and regf 255                                                            ;; ADDR_188 0x2bfff
            ; should generate 0x2bfff
        and reg0 0377                                                           ;; ADDR_189 0x2b0ff
            ; should generate 0x2b0ff
        and regf 0377                                                           ;; ADDR_190 0x2bfff
            ; should generate 0x2bfff
        and reg0 0xfff                                                          ;; ADDR_191 0x2b0ff
            ; should generate 0x2b0ff and a warning about truncation
        and regf 0xfff                                                          ;; ADDR_192 0x2bfff
            ; should generate 0x2bfff and a warning about truncation

;; AND REGx label       101011RxRxcccccccc
        and reg0 Zero                                                           ;; ADDR_193 0x2b000
            ; should generate 0x2b000
        and regf Zero                                                           ;; ADDR_194 0x2bf00
            ; should generate 0x2bf00
        and reg0 ff                                                             ;; ADDR_195 0x2b0ff
            ; should generate 0x2b0ff
        and regf ff                                                             ;; ADDR_196 0x2bfff
            ; should generate 0x2bfff
        and reg0 fff                                                            ;; ADDR_197 0x2b0ff
            ; should generate 0x2b0ff and a warning about truncation
        and regf fff                                                            ;; ADDR_198 0x2bfff
            ; should generate 0x2bfff and a warning about truncation

;; WAND REGx num        101011RxRxcccccccc
        wand reg0 0x0                                                           ;; ADDR_199 0x02000 ADDR_200 0x2b000
            ; should generate 0x02000 then 0x2b000
        wand regf 0x0                                                           ;; ADDR_201 0x02000 ADDR_202 0x2bf00
            ; should generate 0x02000 then 0x2bf00
        wand reg0 0xffff                                                        ;; ADDR_203 0x020ff ADDR_204 0x2b0ff
            ; should generate 0x020ff then 0x2b0ff
        wand regf 0xffff                                                        ;; ADDR_205 0x020ff ADDR_206 0x2bfff
            ; should generate 0x020ff then 0x2bfff
        wand reg0 0xfffff                                                       ;; ADDR_207 0x020ff ADDR_208 0x2b0ff
            ; should generate 0x020ff then 0x2b0ff and a warning about truncation
        wand regf 0xfffff                                                       ;; ADDR_209 0x020ff ADDR_210 0x2bfff
            ; should generate 0x020ff then 0x2bfff and a warning about truncation

;; WAND REGx label      101011RxRxcccccccc
        wand reg0 Zero                                                          ;; ADDR_211 0x02000 ADDR_212 0x2b000
            ; should generate 0x02000 then 0x2b000
        wand regf Zero                                                          ;; ADDR_213 0x02000 ADDR_214 0x2bf00
            ; should generate 0x02000 then 0x2bf00
        wand reg0 ffff                                                          ;; ADDR_215 0x020ff ADDR_216 0x2b0ff
            ; should generate 0x020ff then 0x2b0ff
        wand regf ffff                                                          ;; ADDR_217 0x020ff ADDR_218 0x2bfff
            ; should generate 0x020ff then 0x2bfff
        wand reg0 fffff                                                         ;; ADDR_219 0x020ff ADDR_220 0x2b0ff
            ; should generate 0x020ff then 0x2b0ff and a warning about truncation
        wand regf fffff                                                         ;; ADDR_221 0x020ff ADDR_222 0x2bfff
            ; should generate 0x020ff then 0x2bfff and a warning about truncation

;; LOAD REGx num        001001RxRxcccccccc
        load reg0 0x0                                                           ;; ADDR_223 0x09000
            ; should generate 0x09000
        load regf 0x0                                                           ;; ADDR_224 0x09f00
            ; should generate 0x09f00
        load reg0 255                                                           ;; ADDR_225 0x090ff
            ; should generate 0x090ff
        load regf 255                                                           ;; ADDR_226 0x09fff
            ; should generate 0x09fff
        load reg0 0377                                                          ;; ADDR_227 0x090ff
            ; should generate 0x090ff
        load regf 0377                                                          ;; ADDR_228 0x09fff
            ; should generate 0x09fff
        load reg0 0xfff                                                         ;; ADDR_229 0x090ff
            ; should generate 0x090ff and a warning about truncation
        load regf 0xfff                                                         ;; ADDR_230 0x09fff
            ; should generate 0x09fff and a warning about truncation

;; LOAD REGx label      001001RxRxcccccccc
        load reg0 Zero                                                          ;; ADDR_231 0x09000
            ; should generate 0x09000
        load regf Zero                                                          ;; ADDR_232 0x09f00
            ; should generate 0x09f00
        load reg0 ff                                                            ;; ADDR_233 0x090ff
            ; should generate 0x090ff
        load regf ff                                                            ;; ADDR_234 0x09fff
            ; should generate 0x09fff
        load reg0 fff                                                           ;; ADDR_235 0x090ff
            ; should generate 0x090ff and a warning about truncation
        load regf fff                                                           ;; ADDR_236 0x09fff
            ; should generate 0x09fff and a warning about truncation

;; WLOAD REGx num       001001RxRxcccccccc
        wload reg0 0x0                                                          ;; ADDR_237 0x02000 ADDR_238 0x09000
            ; should generate 0x02000 then 0x09000
        wload regf 0x0                                                          ;; ADDR_239 0x02000 ADDR_240 0x09f00
            ; should generate 0x02000 then 0x09f00
        wload reg0 0xffff                                                       ;; ADDR_241 0x020ff ADDR_242 0x090ff
            ; should generate 0x020ff then 0x090ff
        wload regf 0xffff                                                       ;; ADDR_243 0x020ff ADDR_244 0x09fff
            ; should generate 0x020ff then 0x09fff
        wload reg0 0xfffff                                                      ;; ADDR_245 0x020ff ADDR_246 0x090ff
            ; should generate 0x020ff then 0x090ff and a warning about truncation
        wload regf 0xfffff                                                      ;; ADDR_247 0x020ff ADDR_248 0x09fff
            ; should generate 0x020ff then 0x09fff and a warning about truncation

;; WLOAD REGx label     001001RxRxcccccccc
        wload reg0 Zero                                                         ;; ADDR_249 0x02000 ADDR_250 0x09000
            ; should generate 0x02000 then 0x09000
        wload regf Zero                                                         ;; ADDR_251 0x02000 ADDR_252 0x09f00
            ; should generate 0x02000 then 0x09f00
        wload reg0 ffff                                                         ;; ADDR_253 0x020ff ADDR_254 0x090ff
            ; should generate 0x020ff then 0x090ff
        wload regf ffff                                                         ;; ADDR_255 0x020ff ADDR_256 0x09fff
            ; should generate 0x020ff then 0x09fff
        wload reg0 fffff                                                        ;; ADDR_257 0x020ff ADDR_258 0x090ff
            ; should generate 0x020ff then 0x090ff and a warning about truncation
        wload regf fffff                                                        ;; ADDR_259 0x020ff ADDR_260 0x09fff
            ; should generate 0x020ff then 0x09fff and a warning about truncation

;; COMPARE REGx num     001011RxRxcccccccc
        compare reg0 0x0                                                        ;; ADDR_261 0x0b000
            ; should generate 0x0b000
        compare regf 0x0                                                        ;; ADDR_262 0x0bf00
            ; should generate 0x0bf00
        compare reg0 255                                                        ;; ADDR_263 0x0b0ff
            ; should generate 0x0b0ff
        compare regf 255                                                        ;; ADDR_264 0x0bfff
            ; should generate 0x0bfff
        compare reg0 0377                                                       ;; ADDR_265 0x0b0ff
            ; should generate 0x0b0ff
        compare regf 0377                                                       ;; ADDR_266 0x0bfff
            ; should generate 0x0bfff
        compare reg0 0xfff                                                      ;; ADDR_267 0x0b0ff
            ; should generate 0x0b0ff and a warning about truncation
        compare regf 0xfff                                                      ;; ADDR_268 0x0bfff
            ; should generate 0x0bfff and a warning about truncation

;; COMPARE REGx label   001011RxRxcccccccc
        compare reg0 Zero                                                       ;; ADDR_269 0x0b000
            ; should generate 0x0b000
        compare regf Zero                                                       ;; ADDR_270 0x0bf00
            ; should generate 0x0bf00
        compare reg0 ff                                                         ;; ADDR_271 0x0b0ff
            ; should generate 0x0b0ff
        compare regf ff                                                         ;; ADDR_272 0x0bfff
            ; should generate 0x0bfff
        compare reg0 fff                                                        ;; ADDR_273 0x0b0ff
            ; should generate 0x0b0ff and a warning about truncation
        compare regf fff                                                        ;; ADDR_274 0x0bfff
            ; should generate 0x0bfff and a warning about truncation

;; WCOMPARE REGx num    001011RxRxcccccccc
        wcompare reg0 0x0                                                       ;; ADDR_275 0x02000 ADDR_276 0x0b000
            ; should generate 0x02000 then 0x0b000
        wcompare regf 0x0                                                       ;; ADDR_277 0x02000 ADDR_278 0x0bf00
            ; should generate 0x02000 then 0x0bf00
        wcompare reg0 0xffff                                                    ;; ADDR_279 0x020ff ADDR_280 0x0b0ff
            ; should generate 0x020ff then 0x0b0ff
        wcompare regf 0xffff                                                    ;; ADDR_281 0x020ff ADDR_282 0x0bfff
            ; should generate 0x020ff then 0x0bfff
        wcompare reg0 0xfffff                                                   ;; ADDR_283 0x020ff ADDR_284 0x0b0ff
            ; should generate 0x020ff then 0x0b0ff and a warning about truncation
        wcompare regf 0xfffff                                                   ;; ADDR_285 0x020ff ADDR_286 0x0bfff
            ; should generate 0x020ff then 0x0bfff and a warning about truncation

;; WCOMPARE REGx label  001011RxRxcccccccc
        wcompare reg0 Zero                                                      ;; ADDR_287 0x02000 ADDR_288 0x0b000
            ; should generate 0x02000 then 0x0b000
        wcompare regf Zero                                                      ;; ADDR_289 0x02000 ADDR_290 0x0bf00
            ; should generate 0x02000 then 0x0bf00
        wcompare reg0 ffff                                                      ;; ADDR_291 0x020ff ADDR_292 0x0b0ff
            ; should generate 0x020ff then 0x0b0ff
        wcompare regf ffff                                                      ;; ADDR_293 0x020ff ADDR_294 0x0bfff
            ; should generate 0x020ff then 0x0bfff
        wcompare reg0 fffff                                                     ;; ADDR_295 0x020ff ADDR_296 0x0b0ff
            ; should generate 0x020ff then 0x0b0ff and a warning about truncation
        wcompare regf fffff                                                     ;; ADDR_297 0x020ff ADDR_298 0x0bfff
            ; should generate 0x020ff then 0x0bfff and a warning about truncation
