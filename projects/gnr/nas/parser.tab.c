/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     REG = 258,
     NUM = 259,
     NOP = 260,
     CLRP = 261,
     IMM = 262,
     ADDACC = 263,
     SUBTACC = 264,
     MULT = 265,
     MULTACC = 266,
     LOAD = 267,
     WLOAD = 268,
     COMPARE = 269,
     WCOMPARE = 270,
     STASH = 271,
     PUSHP = 272,
     POPP = 273,
     TESTP = 274,
     SHFTP = 275,
     INC = 276,
     DEC = 277,
     SAVEPLO = 278,
     SAVEPMD = 279,
     SAVEPHI = 280,
     JUMP = 281,
     JUMPCY = 282,
     JUMPN = 283,
     JUMPNZ = 284,
     JUMPZ = 285,
     CALL = 286,
     RET = 287,
     OR = 288,
     WOR = 289,
     AND = 290,
     WAND = 291,
     FETCH = 292,
     WFETCH = 293,
     SHFTAR = 294,
     SHFTBR = 295,
     SHFTLL = 296,
     SHFTLR = 297,
     LABEL = 298,
     DEF_NUM = 299
   };
#endif
/* Tokens.  */
#define REG 258
#define NUM 259
#define NOP 260
#define CLRP 261
#define IMM 262
#define ADDACC 263
#define SUBTACC 264
#define MULT 265
#define MULTACC 266
#define LOAD 267
#define WLOAD 268
#define COMPARE 269
#define WCOMPARE 270
#define STASH 271
#define PUSHP 272
#define POPP 273
#define TESTP 274
#define SHFTP 275
#define INC 276
#define DEC 277
#define SAVEPLO 278
#define SAVEPMD 279
#define SAVEPHI 280
#define JUMP 281
#define JUMPCY 282
#define JUMPN 283
#define JUMPNZ 284
#define JUMPZ 285
#define CALL 286
#define RET 287
#define OR 288
#define WOR 289
#define AND 290
#define WAND 291
#define FETCH 292
#define WFETCH 293
#define SHFTAR 294
#define SHFTBR 295
#define SHFTLL 296
#define SHFTLR 297
#define LABEL 298
#define DEF_NUM 299




/* Copy the first part of user declarations.  */



/*******************************************************************************
* Copyright 2007 by Jack Lloyd, All Rights Reserved
*                   jack@stormyskies.net
*
* FILE: parser.y
* DESC: GNU Bison grammar file for the NPU Assembler (nas).
*
* REVISION HISTORY (Date, Modifier: Description)
* ----------------------------------------------
*
* 2007-01-28, Jack Lloyd:
* Creation.
*
* 2007-07-09, Jack Lloyd:
* Added support for labels in the and, wand, compare, wcompare, imm, and wload
* instructions; Corrected the warning check on the 3rd terminal in the LOAD REG
* NUM rule (used to be 12-bit, now its 8-bit).
*
* 2007-08-16, Jack Lloyd:
* Fixed output for LOAD REG LABEL rule.
*
* 2007-08-19, Jack Lloyd:
* wload now loads the upper 8-bits into imm, instead of just 4.
*
* 2007-08-24, Jack Lloyd:
* WAND REG LABEL had a bug corrected where more than the low 8 bits of whatever
* number LABEL referred to were put in the and opcode.
*
* 2007-10-07, Jack Lloyd:
* [2.x] Added new instructions (see "GPU_CODES_2007_10_03.pdf").
* 
* 2009-09-14, Jack Lloyd:
* [2.5] Added new instructions JUMP, CALL, RET
*       Added ability to dump "symbol table"
*
*******************************************************************************/


#include <malloc.h>

// nas.h is included by .c and .cpp files, so we can't include stdafx.h, yet we
// need bool and it's values, so here goes...
#if !defined(bool)
typedef unsigned char bool;
#endif
#if !defined(true)
#define true 0xff
#endif
#if !defined(false)
#define false 0
#endif

#include "nas.h"

static label_record_t * label_record_scratch;



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 1
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union

symantic_value {
    complete_instruction_t instruction;
    label_record_t         *label_record;
    unsigned long int      operand;
    char                   *string;
}
/* Line 193 of yacc.c.  */

	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */


#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   237

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  8
/* YYNRULES -- Number of rules.  */
#define YYNRULES  75
/* YYNRULES -- Number of states.  */
#define YYNSTATES  128

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      46,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    45,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,    11,    14,    16,    19,    21,
      24,    26,    29,    32,    36,    39,    42,    44,    46,    49,
      52,    55,    58,    62,    66,    70,    74,    78,    82,    86,
      90,    94,    98,   102,   106,   110,   112,   114,   116,   118,
     121,   124,   127,   130,   133,   136,   139,   142,   145,   148,
     151,   154,   157,   160,   163,   166,   169,   171,   175,   179,
     183,   187,   191,   195,   199,   203,   207,   211,   215,   219,
     223,   227,   231,   234,   237,   240
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      48,     0,    -1,    -1,    48,    53,    -1,    44,    43,     4,
      -1,    43,    45,    -1,    50,    -1,    51,    50,    -1,    54,
      -1,    52,    54,    -1,    46,    -1,    51,    46,    -1,    52,
      46,    -1,    51,    52,    46,    -1,    49,    46,    -1,     1,
      46,    -1,     5,    -1,     6,    -1,     7,     4,    -1,     7,
      43,    -1,     8,     3,    -1,     9,     3,    -1,    10,     3,
       3,    -1,    11,     3,     3,    -1,    12,     3,     4,    -1,
      13,     3,     4,    -1,    12,     3,    43,    -1,    13,     3,
      43,    -1,    12,     3,     3,    -1,    14,     3,     4,    -1,
      15,     3,     4,    -1,    14,     3,    43,    -1,    15,     3,
      43,    -1,    14,     3,     3,    -1,    16,     3,     3,    -1,
      17,    -1,    18,    -1,    19,    -1,    20,    -1,    21,     3,
      -1,    22,     3,    -1,    23,     3,    -1,    24,     3,    -1,
      25,     3,    -1,    26,     4,    -1,    26,    43,    -1,    27,
       4,    -1,    27,    43,    -1,    28,     4,    -1,    28,    43,
      -1,    29,     4,    -1,    29,    43,    -1,    30,     4,    -1,
      30,    43,    -1,    31,     4,    -1,    31,    43,    -1,    32,
      -1,    33,     3,     4,    -1,    33,     3,    43,    -1,    34,
       3,     4,    -1,    34,     3,    43,    -1,    33,     3,     3,
      -1,    35,     3,     4,    -1,    35,     3,    43,    -1,    36,
       3,     4,    -1,    36,     3,    43,    -1,    35,     3,     3,
      -1,    37,     3,     3,    -1,    37,     3,     4,    -1,    38,
       3,     4,    -1,    37,     3,    43,    -1,    38,     3,    43,
      -1,    39,     3,    -1,    40,     3,    -1,    41,     3,    -1,
      42,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   125,   128,   154,   176,   177,   180,   181,
     184,   185,   186,   187,   188,   189,   193,   197,   201,   212,
     236,   247,   258,   276,   294,   312,   331,   362,   394,   412,
     430,   449,   480,   512,   530,   548,   552,   556,   560,   564,
     575,   586,   597,   608,   619,   630,   654,   665,   689,   700,
     724,   735,   759,   770,   794,   805,   829,   833,   851,   882,
     901,   933,   951,   969,  1000,  1019,  1051,  1069,  1087,  1105,
    1124,  1155,  1187,  1198,  1209,  1220
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "REG", "NUM", "NOP", "CLRP", "IMM",
  "ADDACC", "SUBTACC", "MULT", "MULTACC", "LOAD", "WLOAD", "COMPARE",
  "WCOMPARE", "STASH", "PUSHP", "POPP", "TESTP", "SHFTP", "INC", "DEC",
  "SAVEPLO", "SAVEPMD", "SAVEPHI", "JUMP", "JUMPCY", "JUMPN", "JUMPNZ",
  "JUMPZ", "CALL", "RET", "OR", "WOR", "AND", "WAND", "FETCH", "WFETCH",
  "SHFTAR", "SHFTBR", "SHFTLL", "SHFTLR", "LABEL", "DEF_NUM", "':'",
  "'\\n'", "$accept", "input", "address_declaration", "label_declaration",
  "label_declarations", "instructions", "line", "instruction", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    58,    10
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    47,    48,    48,    49,    50,    51,    51,    52,    52,
      53,    53,    53,    53,    53,    53,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54,    54,    54,    54,    54,
      54,    54,    54,    54,    54,    54
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     3,     2,     1,     2,     1,     2,
       1,     2,     2,     3,     2,     2,     1,     1,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,    16,    17,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    35,    36,    37,    38,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    10,     0,     6,     0,     0,     3,
       8,    15,    18,    19,    20,    21,     0,     0,     0,     0,
       0,     0,     0,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,     0,     0,     0,     0,     0,    72,    73,    74,    75,
       5,     0,    14,    11,     7,     0,    12,     9,    22,    23,
      28,    24,    26,    25,    27,    33,    29,    31,    30,    32,
      34,    61,    57,    58,    59,    60,    66,    62,    63,    64,
      65,    67,    68,    70,    69,    71,     4,    13
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    45,    46,    47,    48,    49,    50
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -49
static const yytype_int16 yypact[] =
{
     -49,     2,   -49,   -45,   -49,   -49,   169,     1,     3,    85,
     126,   127,   191,   192,   193,   194,   -49,   -49,   -49,   -49,
     195,   196,   197,   198,   199,   179,   180,   181,   182,   183,
     184,   -49,   200,   201,   202,   203,   204,   205,   206,   207,
     210,   211,   -40,    46,   -49,   170,   -49,    44,    86,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   215,   217,   168,   185,
     172,   186,   230,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     174,   187,   176,   188,   178,   189,   -49,   -49,   -49,   -49,
     -49,   231,   -49,   -49,   -49,   128,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -49,   -49,   -49,    84,   -49,   190,   -49,   -48
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      97,    51,     2,     3,    54,    90,    55,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    97,    44,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    56,    91,
      93,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    57,
      58,    94,    96,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,   100,   101,    52,   127,   105,   106,   111,   112,   116,
     117,   121,   122,    68,    70,    72,    74,    76,    78,   103,
     108,   114,   119,   124,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    80,    81,    82,    83,    84,    85,    86,
      87,   102,    53,    88,    89,   107,    92,   113,    98,   118,
      99,   123,    69,    71,    73,    75,    77,    79,   104,   109,
     115,   120,   125,   110,     0,   126,     0,    95
};

static const yytype_int8 yycheck[] =
{
      48,    46,     0,     1,     3,    45,     3,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    95,    46,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,     3,    43,
      46,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,     3,
       3,    47,    46,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,     3,     4,     4,    46,     3,     4,     3,     4,     3,
       4,     3,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,    43,    43,     3,     3,    43,    46,    43,     3,    43,
       3,    43,    43,    43,    43,    43,    43,    43,    43,    43,
      43,    43,    43,     3,    -1,     4,    -1,    47
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    48,     0,     1,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    46,    49,    50,    51,    52,    53,
      54,    46,     4,    43,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,    43,
       4,    43,     4,    43,     4,    43,     4,    43,     4,    43,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
      45,    43,    46,    46,    50,    52,    46,    54,     3,     3,
       3,     4,    43,     4,    43,     3,     4,    43,     4,    43,
       3,     3,     4,    43,     4,    43,     3,     4,    43,     4,
      43,     3,     4,    43,     4,    43,     4,    46
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:

    {
                        if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                        {
                          // see if the label already exists by trying to get it;
                          // if it already exists, that's an error
                          if (NULL != ((yyval.label_record) = get_label_record((yyvsp[(2) - (3)].string))))
                          {
                            if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                              nas_error((yylsp[(1) - (3)]), "Duplicate label declaration for '%s'.", (yyvsp[(2) - (3)].string));
                          }

                          // otherwise, try to add the new label to the label
                          // table; if we can't that's an error, too
                          else if (NULL == ((yyval.label_record) = put_label_record(generate_new_label_record((yyvsp[(2) - (3)].string)))))
                          {
                            nas_error((yylsp[(1) - (3)]), "internal: unable to add new label '%s' for some strange reason...freakin' weird, man!!", (yyvsp[(2) - (3)].string));
                          }
                          else 
			  {
			    (yyval.label_record)->address = (yyvsp[(3) - (3)].operand);
			    (yyval.label_record)->IsLabel = false;
			  }
                        }
                      ;}
    break;

  case 5:

    {
                       if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                       {
                         // see if the label already exists by trying to get it;
                         // if it already exists, that's an error
                         if (NULL != ((yyval.label_record) = get_label_record((yyvsp[(1) - (2)].string))))
                         {
                           if (NAS_OUTPUT_STATE_First_Pass == nasOutputState)
                             nas_error((yylsp[(1) - (2)]), "Duplicate label declaration for '%s'.", (yyvsp[(1) - (2)].string));
                         }

                         // otherwise, try to add the new label to the label
                         // table; if we can't that's an error, too
                         else if (NULL == ((yyval.label_record) = put_label_record(generate_new_label_record((yyvsp[(1) - (2)].string)))))
                         {
                           nas_error((yylsp[(1) - (2)]), "internal: unable to add new label '%s' for some strange reason...freakin' weird, man!!", (yyvsp[(1) - (2)].string));
                         }
			 (yyval.label_record)->IsLabel = true;
                       }
                     ;}
    break;

  case 8:

    { if (NAKED_INSTR_no_instruction != (yyvsp[(1) - (1)].instruction)) nas_output((yyvsp[(1) - (1)].instruction)); ;}
    break;

  case 9:

    { if (NAKED_INSTR_no_instruction != (yyvsp[(2) - (2)].instruction)) nas_output((yyvsp[(2) - (2)].instruction)); ;}
    break;

  case 15:

    { yyerrok; ;}
    break;

  case 16:

    {
                    (yyval.instruction) = NAKED_INSTR_nop;
                  ;}
    break;

  case 17:

    {
                    (yyval.instruction) = NAKED_INSTR_clrp;
                  ;}
    break;

  case 18:

    {
                    if (0xff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "constant truncation (8-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_imm | (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 19:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_imm | num;
                    }
                  ;}
    break;

  case 20:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_addacc | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 21:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_subtacc | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 22:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_mult | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 23:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_multacc | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 24:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x000000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_loadRC | ((yyvsp[(2) - (3)].operand) << 8) |  (yyvsp[(3) - (3)].operand);
                  ;}
    break;

  case 25:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xffff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (((yyvsp[(3) - (3)].operand) & 0xff00) >> 8));
                    (yyval.instruction) = NAKED_INSTR_loadRC | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) & 0xff);
                  ;}
    break;

  case 26:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                        num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_loadRC | ((yyvsp[(2) - (3)].operand) << 8) | num;
                    }
                  ;}
    break;

  case 27:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      (yyval.instruction) = NAKED_INSTR_loadRC | ((yyvsp[(2) - (3)].operand) << 8) | (num & 0xff);
                    }
                  ;}
    break;

  case 28:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_loadRR | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 29:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x000000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_compareRC | ((yyvsp[(2) - (3)].operand) << 8) | (yyvsp[(3) - (3)].operand);
                  ;}
    break;

  case 30:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xffff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (((yyvsp[(3) - (3)].operand) & 0xff00) >> 8));
                    (yyval.instruction) = NAKED_INSTR_compareRC | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) & 0xff);
                  ;}
    break;

  case 31:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (8-bit)");
                        num &= 0x00000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_compareRC | ((yyvsp[(2) - (3)].operand) << 8) | num;
                    }
                  ;}
    break;

  case 32:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (16-bit)");
                        num &= 0x000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      (yyval.instruction) = NAKED_INSTR_compareRC | ((yyvsp[(2) - (3)].operand) << 8) | (num & 0xff);
                    }
                  ;}
    break;

  case 33:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_compareRR | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 34:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_stash | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 35:

    {
                    (yyval.instruction) = NAKED_INSTR_pushp;
                  ;}
    break;

  case 36:

    {
                    (yyval.instruction) = NAKED_INSTR_popp;
                  ;}
    break;

  case 37:

    {
                    (yyval.instruction) = NAKED_INSTR_testp;
                  ;}
    break;

  case 38:

    {
                    (yyval.instruction) = NAKED_INSTR_shftp;
                  ;}
    break;

  case 39:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_inc | ((yyvsp[(2) - (2)].operand) << 8)
                  ;}
    break;

  case 40:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_dec | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 41:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_saveplo | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 42:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_savepmd | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 43:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_savephi | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 44:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_jump | (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 45:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_jump | num;
                    }
                  ;}
    break;

  case 46:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_jumpcy | (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 47:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_jumpcy | num;
                    }
                  ;}
    break;

  case 48:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_jumpn | (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 49:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_jumpn | num;
                    }
                  ;}
    break;

  case 50:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_jumpnz |  (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 51:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_jumpnz | num;
                    }
                  ;}
    break;

  case 52:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_jumpz |  (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 53:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_jumpz | num;
                    }
                  ;}
    break;

  case 54:

    {
                    if (0xfff < (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                        (yyvsp[(2) - (2)].operand) &= 0x000000fff;
                    }

                    (yyval.instruction) = NAKED_INSTR_call |  (yyvsp[(2) - (2)].operand);
                  ;}
    break;

  case 55:

    {
                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(2) - (2)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(2) - (2)]), "label '%s' does not exist", (yyvsp[(2) - (2)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      unsigned int num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(2) - (2)]), "instruction address truncation (12-bit)");
                          num &= 0x000000fff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_call | num;
                    }
                  ;}
    break;

  case 56:

    {
                    (yyval.instruction) = NAKED_INSTR_ret;
                  ;}
    break;

  case 57:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x000000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_orRC | ((yyvsp[(2) - (3)].operand) << 8) | (yyvsp[(3) - (3)].operand);
                  ;}
    break;

  case 58:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_orRC | ((yyvsp[(2) - (3)].operand) << 8) | num;
                    }
                  ;}
    break;

  case 59:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xffff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (((yyvsp[(3) - (3)].operand) & 0xff00) >> 8));
                    (yyval.instruction) = NAKED_INSTR_orRC | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) & 0xff);
                  ;}
    break;

  case 60:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      (yyval.instruction) = NAKED_INSTR_orRC | ((yyvsp[(2) - (3)].operand) << 8) | (num & 0xff);
                    }
                  ;}
    break;

  case 61:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_orRR | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 62:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x000000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_andRC | ((yyvsp[(2) - (3)].operand) << 8) | (yyvsp[(3) - (3)].operand);
                  ;}
    break;

  case 63:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(3) - (3)]), "constant truncation (8-bit)");
                          num &= 0x000000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_andRC | ((yyvsp[(2) - (3)].operand) << 8) | num;
                    }
                  ;}
    break;

  case 64:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xffff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x0000ffff;
                    }

                    nas_output(NAKED_INSTR_imm | (((yyvsp[(3) - (3)].operand) & 0xff00) >> 8));
                    (yyval.instruction) = NAKED_INSTR_andRC | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) & 0xff);
                  ;}
    break;

  case 65:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xffff < num)
                      {
                          if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                            nas_warn((yylsp[(3) - (3)]), "constant truncation (16-bit)");
                          num &= 0x0000ffff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0xff00) >> 8));
                      (yyval.instruction) = NAKED_INSTR_andRC | ((yyvsp[(2) - (3)].operand) << 8) | (num & 0xff);
                    }
                  ;}
    break;

  case 66:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_andRR | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 67:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (16 <= (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(3) - (3)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_fetchRR   | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) << 4);
                  ;}
    break;

  case 68:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (12-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x00000ff;
                    }

                    (yyval.instruction) = NAKED_INSTR_fetchRD | ((yyvsp[(2) - (3)].operand) << 8) |  (yyvsp[(3) - (3)].operand);
                  ;}
    break;

  case 69:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (0xfff < (yyvsp[(3) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (12-bit)");
                        (yyvsp[(3) - (3)].operand) &= 0x00000fff;
                    }

                    nas_output(NAKED_INSTR_imm | (((yyvsp[(3) - (3)].operand) & 0x0f00) >> 8));
                    (yyval.instruction) = NAKED_INSTR_fetchRD | ((yyvsp[(2) - (3)].operand) << 8) | ((yyvsp[(3) - (3)].operand) & 0xff);
                  ;}
    break;

  case 70:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (8-bit)");
                        num &= 0x00000ff;
                      }

                      label_record_scratch->fUsed = true;
                      (yyval.instruction) = NAKED_INSTR_fetchRD | ((yyvsp[(2) - (3)].operand) << 8) | num;
                    }
                  ;}
    break;

  case 71:

    {
                    if (16 <= (yyvsp[(2) - (3)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (3)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (3)].operand) &= 0xf;
                    }

                    if (NULL == (label_record_scratch = get_label_record((yyvsp[(3) - (3)].string))))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_error((yylsp[(3) - (3)]), "label '%s' does not exist", (yyvsp[(3) - (3)].string));
                        (yyval.instruction) = NAKED_INSTR_nop;
                    }

                    else
                    {
                      nas_address_t num = label_record_scratch->address;

                      if (0xfff < num)
                      {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(3) - (3)]), "data address truncation (12-bit)");
                        num &= 0x00000fff;
                      }

                      label_record_scratch->fUsed = true;
                      nas_output(NAKED_INSTR_imm | ((num & 0x0f00) >> 8));
                      (yyval.instruction) = NAKED_INSTR_fetchRD | ((yyvsp[(2) - (3)].operand) << 8) | (num & 0xff);
                    }
                  ;}
    break;

  case 72:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_shftar | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 73:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_shftbr | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 74:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_shftll | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;

  case 75:

    {
                    if (16 <= (yyvsp[(2) - (2)].operand))
                    {
                        if (NAS_OUTPUT_STATE_First_Pass != nasOutputState)
                          nas_warn((yylsp[(2) - (2)]), "register truncation [0, 16)");
                        (yyvsp[(2) - (2)].operand) &= 0xf;
                    }

                    (yyval.instruction) = NAKED_INSTR_shftlr | ((yyvsp[(2) - (2)].operand) << 8);
                  ;}
    break;


/* Line 1267 of yacc.c.  */

      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}





