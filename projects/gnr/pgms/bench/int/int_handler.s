;          This is intended to be a generic PP interrupt handler
;          that will call a C program called int_inner that will
;          do the real work.  All registers that can possibly be
;          used by a C program are saved and restored per TI 
;          guidelines.             08/11/95    GNR
           .global $int_handler,$int_inner
$int_handler:
           *--sp = iprs
           *--sp = d0
           *--sp = d1
           *--sp = d2
           *--sp = d3
           *--sp = d4
           *--sp = d5
           *--sp = a0
           *--sp = a1
           *--sp = a2
           *--sp = a3
           *--sp = a8
           *--sp = a9
           *--sp = a10
           *--sp = a11
           *--sp = x0
           *--sp = x1
           *--sp = x2
           *--sp = x8
           *--sp = x9
           *--sp = x10
           *--sp = ls0
           *--sp = ls1
           *--sp = ls2
           *--sp = le0
           *--sp = le1
           *--sp = le2
           *--sp = lc0
           *--sp = lc1
           *--sp = lc2
           *--sp = lr0
           *--sp = lr1
           *--sp = lr2
           *--sp = lctl

; Note that the two instructions involving the intflags are
; executed in the branch delay slots following the call
          call = $int_inner
          inten = inten&~%1
          intflg = intflg

           lctl = *sp++
           lr2 = *sp++
           lr1 = *sp++
           lr0 = *sp++
           lc2 = *sp++
           lc1 = *sp++
           lc0 = *sp++
           le2 = *sp++
           le1 = *sp++
           le0 = *sp++
           ls2 = *sp++
           ls1 = *sp++
           ls0 = *sp++
           x10 = *sp++
           x9 = *sp++
           x8 = *sp++
           x2 = *sp++
           x1 = *sp++
           x0 = *sp++
           a11 = *sp++
           a10 = *sp++
           a9 = *sp++
           a8 = *sp++
           a3 = *sp++
           a2 = *sp++
           a1 = *sp++
           a0 = *sp++
           d5 = *sp++
           d4 = *sp++
           d3 = *sp++
           d2 = *sp++
           d1 = *sp++
           d0 = *sp++
           iprs = *sp++
           reti1
           reti2
           reti3
           reti4

