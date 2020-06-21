/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union

symantic_value {
    complete_instruction_t instruction;
    label_record_t         *label_record;
    unsigned long int      operand;
    char                   *string;
}
/* Line 1529 of yacc.c.  */

	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

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

extern YYLTYPE yylloc;
