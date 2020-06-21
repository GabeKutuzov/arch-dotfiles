/* (c) Copyright 2003-2012, Neurosciences Research Foundation, Inc. */
/* $Id: d3opkeys.h 51 2012-05-24 20:53:36Z  $ */
/***********************************************************************
*                             CNS Program                              *
*                        D3OPKEYS Header File                          *
*                                                                      *
*  This file contains definitions of option keys that may be used both *
*  to interpret input options and to print them at d3echo time.        *
*                                                                      *
*  Revisions:                                                          *
*  V8C, 09/20/03, GNR - New header file                                *
*  V8D, 02/09/04, GNR - Add options for conductances and ions          *
*  Rev, 01/09/07, GNR - Add map options, 12/24/07 add value options    *
*  Rev, 12/27/07, GNR - Add okvalue, okrpkrp for repertoire level only *
*  ==>, 12/28/07, GNR - Last mod before committing to svn repository   *
*  V8E, 02/01/09, GNR - Add SIMDATA codes for Izhikevich variables     *
*  V8F, 06/04/10, GNR - Add KGEN=2, move KGEN=E adjacent               *
*  V8G, 08/16/10, GNR - Add okautsc                                    *
*  V8H, 02/24/11, GNR - Add KAM=C,K, remove okeffop                    *
*  Rev, 04/27/11, GNR - Add ctopt=C, kgen=Y, okkract                   *
*  Rev, 04/09/12, GNR - Add mopt=F, ibopt=GF, recode ibopt=V           *
*  Rev, 05/08/12, GNR - Add AUTOSCALE OPT=T, CONNTYPE CNOPT=C          *
***********************************************************************/

#define okautsc  "NTFICAMGS"
#define okcdopt  "IER"
#define okctopt  "FICPASQ ON BMDR"
#define okctkrp  "LEOABXHYZCMGFPDTNU3KQ"
#define okcnkgen "V2E YHWJXFSQPICBADTOGNU LRM"
#define okcnopt  "CDIKOVHPSQ URJ"
#define okcnkam  "K CRQZSTUVWEDBNMF   H43JI"
#define okcnmop  "YXTS FCGLARHIV"
#define okibopt  "TOFSQKVGX"
#define okionop  "CGR"
#define okkplop  "OEVC2XKDSQ GAF"
#define okkract  "NEWAITGRVPC B"               /* RESET action codes */
#define okmodop  "OFSQKUN "
#define okphopt  "MD"
#define okrpkrp  "2OGV IWALSBR"
#define oksaopt  "ILAC"
#define oksimcn  "UAPMJDBCL     "       /* Connection SIMDATA subset */
#define oksimct  "XZUAPMJDBCLOFQSV"       /* Celltype SIMDATA subset */
#define oksimgl  "NIEWRT   XZUAPMJDBCLOFQSV" /* Global SIMDATA codes */
#define okvalue  "2FABN"
