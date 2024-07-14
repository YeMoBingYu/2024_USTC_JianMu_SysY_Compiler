%option noyywrap
%{

#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines=1;
int pos_start=1;
int pos_end=1;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

%}

identifer    [a-zA-Z_][a-zA-Z0-9_]*
decimalconst [1-9][0-9]*
octalconst   0[0-7]*
hexconst     0[xX][0-9a-fA-F]+
commonfloat  [0-9]+\.|[0-9]*\.[0-9]+
hexfloat     0[xX]\.[0-9a-fA-F]*[pP][+-][0-9]*|0[xX][0-9a-fA-F]*\.[0-9a-fA-F]*[pP][+-][0-9]*
efloat       [0-9]*\.[0-9]+[eE][-+][0-9]+|[0-9]*\.[eE][0-9]+|[0-9]+[eE][-+][0-9]+|[0-9]+[eE][0-9]+
char         [^\*/]
linecomment  \/\/.*
COMMENT      \/\*{char}*((\*)+{char}+|\/{char}*)*(\*)*\*\/


%%

else      {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
if        {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
int       {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
return    {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}
void      {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}
while     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
float     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}
const     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return CONST;}
break     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return BREAK;}
continue  {pos_start = pos_end; pos_end += 8; pass_node(yytext); return CONTINUE;}


\+ 	 {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ADD;}
\-   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SUB;}
\*   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MUL;}
\%   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return MOD;}
\/   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return DIV;}
\<   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LT;}
\<\= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return LTE;}
\>   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return GT;}
\>\= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return GTE;}
\=\= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return EQ;}
\!\= {pos_start = pos_end; pos_end += 2; pass_node(yytext); return NEQ;}
\=   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return ASSIGN;}
\;   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return SEMICOLON;}
\,   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return COMMA;}
\(   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LEFTPARENTHESIS;}
\)   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RIGHTPARENTHESIS;}
\[   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LEFTBRACKET;}
\]   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RIGHTBRACKET;}
\{   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return LEFTBRACE;}
\}   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return RIGHTBRACE;}
\!   {pos_start = pos_end; pos_end += 1; pass_node(yytext); return NOT;}
\&\& {pos_start = pos_end; pos_end += 2; pass_node(yytext); return AND;}
\|\| {pos_start = pos_end; pos_end += 2; pass_node(yytext); return OR;}


{identifer}    {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ID;}
{decimalconst} {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}
{octalconst}   {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}
{hexconst}     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}
{hexfloat}     {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}
{efloat}       {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}
{commonfloat}  {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}


{COMMENT} {
     pos_start = pos_end;
     int i;
     for(i = 0; yytext[i] != '\0'; i++)
     {
          pos_end++;
          if(yytext[i] == '\n')
          {
               lines++;
               pos_end = 1;
          }
     }
     pass_node(yytext); 
}
{linecomment} {pos_start = 1; pos_end = pos_start;}
[ \t]         {pos_start = pos_end; pos_end += 1;}
\n            {pos_start = pos_end; pos_end = 1; lines++;}
\r            {pos_start = pos_end; pos_end = 1;}
.             {pos_start = pos_end; pos_end += 1; return ERROR;}

%%
