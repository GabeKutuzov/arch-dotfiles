/* (c) Copyright 2003-2018, The Rockefeller University *11115* */
/* $Id: d3opkeys.h 78 2018-08-02 18:36:58Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        D3OPKEYS Header File                          *
*                                                                      *
*  This file contains definitions of option keys that may be used both *
*  to interpret input options and to print them at d3echo time.        *
************************************************************************
*  V8C, 09/20/03, GNR - New header file                                *
*  V8D, 02/09/04, GNR - Add options for conductances and ions          *
*  Rev, 01/09/07, GNR - Add map options, 12/24/07 add value options    *
*  Rev, 12/27/07, GNR - Add okvalue, okrpkrp for repertoire level only *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8E, 02/01/09, GNR - Add SIMDATA codes for Izhikevich variables     *
*  V8F, 06/04/10, GNR - Add KGEN=2, move KGEN=E adjacent               *
*  V8G, 08/16/10, GNR - Add okautsc                                    *
*  V8H, 02/24/11, GNR - Add KAM=C,K, remove okeffop                    *
*  Rev, 04/27/11, GNR - Add ctopt=C, kgen=Y                            *
*  Rev, 04/09/12, GNR - Add mopt=F, ibopt=GF, recode ibopt=V           *
*  Rev, 05/08/12, GNR - Add AUTOSCALE OPT=T, CONNTYPE CNOPT=C          *
*  Rev, 06/16/12, GNR - Move KAMBY to cnopt, KAM=B is BCM rule         *
*  Rev, 08/03/12, GNR - Add saopt=M                                    *
*  Rev, 08/28/12, GNR - Add KAM=L and missing code for KAM=P           *
*  Rev, 03/29/13, GNR - Add cnopt=M, 04/03/13 add mopt=M               *
*  Rev, 04/23/13, GNR - Add kaut=D, 05/14/13, kaut=H, 07/14/13, kaut=W *
*  R63, 11/03/15, GNR - Add vdopt codes                                *
*  R66, 02/02/16, GNR - Remove ibopt 'G' code, add 'R'                 *
*  R72, 02/05/17, GNR - Add cnkgen code 'K', knopt codes, cmopt codes  *
*  R72, 03/09/17, GNR - Remove krp KRPRR,KRPSP region print, KGNDG     *
*  R76, 12/02/17, GNR - Add okcompat, svitms code 'H'                  *
*  R77, 02/08/18, GNR - Use 'R' for raw afference, 'T' for all joints  *
*  R78, 06/21/18, GNR - Add okcompat=V, okvdopt=V, 07/18/18 oknoise    *
***********************************************************************/

#define okautsc  "WHDNTFICAMGS"
#define okcdopt  "IER"
#define okcmopt  "FCBNGJIATX2KPVOSM"
#define okctopt  "FICPASQJON BMDR"
#define okctkctp "UXHSRFOABPDTNLKE"
#define okctsta  "UXYZHCMGF"
#define okcnkgen "KV2E YHWJXFSQPICBA1TOGNU LRM"
#define okcnkopt "PIE"
#define okcnopt  "MBCDIKOVPNSQ URJ"
#define okcnkam  "KPCRQZSTUVWEDLNMF  BH43JI"
#define okcnmop  "WYXTS FCGLARHIV"
#define okcompat "VPRBAKCS"
#define okibopt  "CV OFSQKMRX"
#define okionop  "CGR"
#define okkplop  "OEVC2XKDSQ GAF"
#define okmodop  "OFSQKMNU"
#define oknoise  "ISVR"
#define okphopt  "MD"
#define okrpkrp  "2OGV IWAL B "
#define oksaopt  "MILAC"
#define oksimcn  "R     UAPMJDBCL     "  /* Connection SIMDATA subset */
#define oksimct  "RYG XZUAPMJDBCLOFQSV"    /* Celltype SIMDATA subset */
#define oksimgl  "NIEWTRYG XZUAPMJDBCLOFQSV"  /* Global SIMDATA codes */
#define okusage  "XRHDEI"
#define okvalue  "2FABN"
#define okvdopt  "HNISVP"
