%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart();
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

syntax_tree *gt;
void yyerror(const char *s);
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

%union {
    struct _syntax_tree_node* node;
}

/* TODO: Your tokens here. */
%token <node> ERROR
%token <node> CONST
%token <node> INT
%token <node> FLOAT
%token <node> ID
%token <node> VOID
%token <node> IF
%token <node> WHILE
%token <node> ELSE
%token <node> BREAK
%token <node> CONTINUE
%token <node> RETURN
%token <node> INTCONST
%token <node> FLOATCONST
%token <node> COMMA    
%token <node> SEMICOLON 
%token <node> ASSIGN    
%token <node> LEFTBRACE
%token <node> RIGHTBRACE
%token <node> LEFTBRACKET
%token <node> RIGHTBRACKET
%token <node> LEFTPARENTHESIS
%token <node> RIGHTPARENTHESIS
%token <node> ADD
%token <node> SUB
%token <node> NOT
%token <node> MUL
%token <node> DIV
%token <node> MOD
%token <node> LT
%token <node> LTE
%token <node> GT
%token <node> GTE
%token <node> EQ
%token <node> NEQ
%token <node> AND
%token <node> OR

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left COMMA
%right ASSIGN
%left OR
%left AND
%left EQ NEQ
%left LT LTE GT GTE
%left ADD SUB
%left MUL DIV MOD
%right NOT 
%left LEFTPARENTHESIS RIGHTPARENTHESIS LEFTBRACKET RIGHTBRACKET LEFTBRACE RIGHTBRACE

%type <node> compunit decl constdecl constdef_list basic_type
%type <node> constdef constexp_list constinitval constinitval_list vardecl
%type <node> vardef_list vardef initval initval_list funcdef
%type <node> funcfparams funcfparam_list funcfparam exp_list block
%type <node> block_list blockitem stmt 
%type <node> exp cond lval primaryexp number 
%type <node> unaryexp unaryop funcrparams exp_list_withcomma mulexp
%type <node> addexp relexp relop eqexp mulop
%type <node> eqop landexp lorexp constexp 
%type <node> integer float
%start compunit

%%

compunit : decl {$$ = node("compunit", 1, $1); gt->root = $$;}
        | funcdef {$$ = node("compunit", 1, $1); gt->root = $$;}
        | compunit decl {$$ = node("compunit", 2, $1, $2); gt->root = $$;}
        | compunit funcdef {$$ = node("compunit", 2, $1, $2); gt->root = $$;};

decl : constdecl {$$ = node("decl", 1, $1);}
        | vardecl {$$ = node("decl", 1, $1);};

constdecl : CONST basic_type constdef SEMICOLON {$$ = node("constdecl", 4, $1, $2, $3, $4);}
        | CONST basic_type constdef constdef_list SEMICOLON {$$ = node("constdecl", 5, $1, $2, $3, $4, $5);};

constdef_list : COMMA constdef {$$ = node("constdef_list", 2, $1, $2);}
        | constdef_list COMMA constdef {$$ = node("constdef_list", 3, $1, $2, $3);};

basic_type : INT {$$ = node("basic_type", 1, $1);}
        | FLOAT {$$ = node("basic_type", 1, $1);};

constdef : ID ASSIGN constinitval {$$ = node("constdef", 3, $1, $2, $3);}
        | ID constexp_list ASSIGN constinitval {$$ = node("constdef", 4, $1, $2, $3, $4);};

constexp_list : constexp_list LEFTBRACKET constexp RIGHTBRACKET {$$ = node("constexp_list", 4, $1, $2, $3, $4);}
        | LEFTBRACKET constexp RIGHTBRACKET {$$ = node("constexp_list", 3, $1, $2, $3);};

constinitval : constexp {$$ = node("constinitval", 1, $1);}
        | LEFTBRACE RIGHTBRACE {$$ = node("constinitval", 2, $1, $2);}
        | LEFTBRACE constinitval RIGHTBRACE {$$ = node("constinitval", 3, $1, $2, $3);}        | LEFTBRACE constinitval constinitval_list RIGHTBRACE {$$ = node("constinitval", 4, $1, $2, $3, $4);};

constinitval_list : constinitval_list COMMA constinitval {$$ = node("constinitval_list", 3, $1, $2, $3);}
        | COMMA constinitval {$$ = node("constinitval_list", 2, $1, $2);};

vardecl : basic_type vardef SEMICOLON {$$ = node("vardecl", 3, $1, $2, $3);}
        | basic_type vardef vardef_list SEMICOLON {$$ = node("vardecl", 4, $1, $2, $3, $4);};

vardef_list : vardef_list COMMA vardef {$$ = node("vardef_list", 3, $1, $2 ,$3);}
        | COMMA vardef {$$ = node("vardef_list", 2, $1, $2);};

vardef : ID {$$ = node("vardef", 1, $1);}
        | ID constexp_list {$$ = node("vardef", 2, $1, $2);}
        | ID ASSIGN initval {$$ = node("vardef", 3, $1, $2, $3);}
        | ID constexp_list ASSIGN initval {$$ = node("vardef", 4, $1, $2, $3, $4);};

initval : exp {$$ = node("initval", 1, $1);}
        | LEFTBRACE RIGHTBRACE {$$ = node("initval", 2, $1, $2);}
        | LEFTBRACE initval RIGHTBRACE {$$ = node("initval", 3, $1, $2, $3);}
        | LEFTBRACE initval initval_list RIGHTBRACE {$$ = node("initval", 4, $1, $2, $3, $4);};

initval_list : initval_list COMMA initval {$$ = node("initval_list", 3, $1, $2, $3);}
        | COMMA initval {$$ = node("initval_list", 2, $1, $2);};

funcdef : basic_type ID LEFTPARENTHESIS RIGHTPARENTHESIS block {$$ = node("funcdef", 5, $1, $2, $3, $4, $5);}
        | basic_type ID LEFTPARENTHESIS funcfparams RIGHTPARENTHESIS block {$$ = node("funcdef", 6, $1, $2, $3, $4, $5, $6);}
        | VOID ID LEFTPARENTHESIS RIGHTPARENTHESIS block {$$ = node("funcdef", 5, $1, $2, $3, $4, $5);}
        | VOID ID LEFTPARENTHESIS funcfparams RIGHTPARENTHESIS block {$$ = node("funcdef", 6, $1, $2, $3, $4, $5, $6);};

funcfparams : funcfparam {$$ = node("funcfparams", 1, $1);}
        | funcfparam funcfparam_list {$$ = node("funcfparams", 2, $1, $2);};

funcfparam_list : funcfparam_list COMMA funcfparam {$$ = node("funcfparam_list", 3, $1, $2 ,$3);}
        | COMMA funcfparam {$$ = node("funcfparam_list", 2, $1, $2);};

funcfparam : basic_type ID {$$ = node("funcfparam", 2, $1, $2);}
        | basic_type ID LEFTBRACKET RIGHTBRACKET {$$ = node("funcfparam", 4, $1, $2, $3, $4);}
        | basic_type ID LEFTBRACKET RIGHTBRACKET exp_list {$$ = node("funcfparam", 5, $1, $2, $3, $4, $5);};

exp_list : exp_list LEFTBRACKET exp RIGHTBRACKET {$$ = node("exp_list", 4, $1, $2, $3, $4);}
        | LEFTBRACKET exp RIGHTBRACKET {$$ = node("exp_list", 3, $1, $2, $3);};

block : LEFTBRACE RIGHTBRACE {$$ = node("block", 2, $1, $2);}
        | LEFTBRACE block_list RIGHTBRACE {$$ = node("block", 3, $1, $2, $3);};

block_list : block_list blockitem {$$ = node("block_list", 2, $1, $2);}
        | blockitem {$$ = node("block_list", 1, $1);};

blockitem : decl {$$ = node("blockitem", 1, $1);}
        | stmt {$$ = node("blockitem", 1, $1);};

stmt : lval ASSIGN exp SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
        | SEMICOLON {$$ = node("stmt", 1, $1);}
        | exp SEMICOLON {$$ = node("stmt", 2, $1, $2);}
        | block {$$ = node("stmt", 1, $1);}
        | IF LEFTPARENTHESIS cond RIGHTPARENTHESIS stmt %prec LOWER_THAN_ELSE {$$ = node("stmt", 5, $1, $2, $3, $4, $5);}
        | IF LEFTPARENTHESIS cond RIGHTPARENTHESIS stmt ELSE stmt {$$ = node("stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
        | WHILE LEFTPARENTHESIS cond RIGHTPARENTHESIS stmt {$$ = node("stmt", 5, $1, $2, $3, $4, $5);}
        | BREAK SEMICOLON {$$ = node("stmt", 2, $1, $2);}
        | CONTINUE SEMICOLON {$$ = node("stmt", 2, $1, $2);}
        | RETURN SEMICOLON {$$ = node("stmt", 2, $1, $2);}
        | RETURN exp SEMICOLON {$$ = node("stmt", 3, $1, $2, $3);};

exp : addexp {$$ = node("exp", 1, $1);};

cond : lorexp {$$ = node("cond", 1, $1);};

lval : ID {$$ = node("lval", 1, $1);}
        | ID exp_list {$$ = node("lval", 2, $1, $2);};

primaryexp : LEFTPARENTHESIS exp RIGHTPARENTHESIS {$$ = node("primaryexp", 3, $1, $2 ,$3);}
        | lval {$$ = node("primaryexp", 1, $1);}
        | number {$$ = node("primaryexp", 1, $1);};

number : integer {$$ = node("number", 1, $1);}
        | float {$$ = node("number", 1, $1);};

integer : INTCONST {$$ = node("integer", 1, $1);};

float   : FLOATCONST {$$ = node("float", 1, $1);};

unaryexp : primaryexp {$$ = node("unaryexp", 1, $1);}
        | ID LEFTPARENTHESIS RIGHTPARENTHESIS {$$ = node("unaryexp", 3, $1, $2 ,$3);}
        | ID LEFTPARENTHESIS funcrparams RIGHTPARENTHESIS {$$ = node("unaryexp", 4, $1, $2 ,$3, $4);}
        | unaryop unaryexp {$$ = node("unaryexp", 2, $1, $2);};

unaryop : ADD {$$ = node("unaryop", 1, $1);}
        | SUB {$$ = node("unaryop", 1, $1);}
        | NOT {$$ = node("unaryop", 1, $1);};

funcrparams : exp {$$ = node("funcrparams", 1, $1);}
        | exp exp_list_withcomma {$$ = node("funcrparams", 2, $1, $2);};

exp_list_withcomma : exp_list_withcomma COMMA exp {$$ = node("exp_list_withcomma", 3, $1, $2 ,$3);}
        | COMMA exp {$$ = node("exp_list_withcomma", 2, $1, $2);};

mulexp : unaryexp {$$ = node("mulexp", 1, $1);}
        | mulexp mulop unaryexp {$$ = node("mulexp", 3, $1, $2 ,$3);};

mulop : MUL {$$ = node("mulop", 1, $1);}
        | DIV {$$ = node("mulop", 1, $1);}
        | MOD {$$ = node("mulop", 1, $1);};
addexp : mulexp {$$ = node("addexp", 1, $1);}
        | addexp ADD mulexp {$$ = node("addexp", 3, $1, $2, $3);}
        | addexp SUB mulexp {$$ = node("addexp", 3, $1, $2, $3);};

relexp : addexp {$$ = node("relexp", 1, $1);}
        | relexp relop addexp {$$ = node("relexp", 3, $1, $2, $3);};

relop : LT {$$ = node("relop", 1, $1);}
        | GT {$$ = node("relop", 1, $1);}
        | LTE {$$ = node("relop", 1, $1);}
        | GTE {$$ = node("relop", 1, $1);};

eqexp : relexp {$$ = node("eqexp", 1, $1);}
        | eqexp eqop relexp {$$ = node("eqexp", 3, $1, $2, $3);};

eqop : EQ {$$ = node("eqop", 1, $1);}
        | NEQ {$$ = node("eqop", 1, $1);};

landexp : eqexp {$$ = node("landexp", 1, $1);}
        | landexp AND eqexp {$$ = node("landexp", 3, $1, $2, $3);};

lorexp : landexp {$$ = node("lorexp", 1, $1);}
        | lorexp OR landexp {$$ = node("lorexp", 3, $1, $2, $3);};

constexp : addexp {$$ = node("constexp", 1, $1);};
%%

void yyerror(const char * s)
{
    fprintf(stderr, "at line %d column %d: %s\n", lines, pos_start, s);
}

syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r")))  {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
