;***************************************************************
;* MVP PP ANSI C Codegen        Version 1.10                   *
;* Date/Time created: Fri Aug  4 14:02:39 1995                 *
;***************************************************************
;	ppac ppmt.c ppmt.if 
	.global	$s1
	.global	$s2
	.global	$l1
	.global	$l2
	.global	$r1
	.global	$r2
	.global	$ppmt
;>>>> 	void ppmt(void) {
;***************************************************************
;* FUNCTION DEF: $ppmt                                         *
;***************************************************************
$ppmt:
;>>>> 	   r1 = s1*s2;
         d1 =sh  *(xba + $s2)
         d2 =sh  *(xba + $s1)
         d1 =s d1 * d2
         *(xba + $r1) =w  d1
;>>>> 	   r2 = l1*l2;
         d1 =sw  *(xba + $l2)
         d2 =sw  *(xba + $l1)
         d4 =uh1  d1
         d4 =u d4 * d2
||         d3 =uh1  d2
         d3 =u d1 * d3
         d1 =u d1 * d2
||         d2 = d4 + d3
         d1 = d1 + (d2 << 16)
         *(xba + $r2) =w  d1
         br = iprs
	 nop
	 nop
;        branch occurs here
	.global	$l1
$l1:	.usect	.pbss,4,4
	.global	$l2
$l2:	.usect	.pbss,4,4
	.global	$r1
$r1:	.usect	.pbss,4,4
	.global	$r2
$r2:	.usect	.pbss,4,4
	.global	$s1
$s1:	.usect	.pbss,2,2
	.global	$s2
$s2:	.usect	.pbss,2,2
