%skeleton "lalr1.cc"
%require "3.0.4"
%defines
%define api.namespace { bpftrace }
%define parser_class_name { Parser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%define parse.error verbose

%param { bpftrace::Driver &driver }
%param { void *yyscanner }
%locations

// Forward declarations of classes referenced in the parser
%code requires
{
namespace bpftrace {
class Driver;
namespace ast {
class Node;
} // namespace ast
} // namespace bpftrace
#include "ast.h"

struct TypeDeclarator
{
  std::string name;
  bool is_ptr;
  int array_size = 1;
};
}

%{
#include <iostream>

#include "driver.h"

void yyerror(bpftrace::Driver &driver, const char *s);
%}

%token
  END 0    "end of file"
  COLON    ":"
  SEMI     ";"
  LBRACE   "{"
  RBRACE   "}"
  LBRACKET "["
  RBRACKET "]"
  LPAREN   "("
  RPAREN   ")"
  ENDPRED  "end predicate"
  COMMA    ","
  ASSIGN   "="
  EQ       "=="
  NE       "!="
  LE       "<="
  GE       ">="
  LT       "<"
  GT       ">"
  LAND     "&&"
  LOR      "||"
  PLUS     "+"
  MINUS    "-"
  MUL      "*"
  DIV      "/"
  MOD      "%"
  BAND     "&"
  BOR      "|"
  BXOR     "^"
  LNOT     "!"
  BNOT     "~"
  INCLUDE  "#include"
  STRUCT   "struct"
  UNION    "union"
  DOT      "."
  PTR      "->"
;

%token <std::string> TYPE "type specifier"
%token <std::string> BUILTIN "builtin"
%token <std::string> IDENT "identifier"
%token <std::string> PATH "path"
%token <std::string> HEADER "header"
%token <std::string> STRING "string"
%token <std::string> MAP "map"
%token <std::string> VAR "variable"
%token <int> INT "integer"

%type <ast::IncludeList *> includes
%type <ast::Include *> include
%type <ast::StructList *> structs
%type <ast::Struct *> struct
%type <ast::FieldList *> fields
%type <ast::FieldList *> field
%type <ast::ProbeList *> probes
%type <ast::Probe *> probe
%type <ast::Predicate *> pred
%type <ast::StatementList *> block stmts
%type <ast::Statement *> stmt
%type <ast::Expression *> expr
%type <ast::Call *> call
%type <ast::Map *> map
%type <ast::Variable *> var
%type <ast::ExpressionList *> vargs
%type <ast::AttachPointList *> attach_points
%type <ast::AttachPoint *> attach_point
%type <std::string> wildcard
%type <std::string> cast_type
%type <std::string> ident
%type <std::string> type_specifier
%type <std::vector<TypeDeclarator>> field_names
%type <TypeDeclarator> declarator direct_declarator

%right ASSIGN
%left LOR
%left LAND
%left BOR
%left BXOR
%left BAND
%left EQ NE
%left LE GE LT GT
%left PLUS MINUS
%left MUL DIV MOD
%right LNOT BNOT DEREF CAST
%left DOT PTR

%start program

%%

program : includes structs probes
        {
          driver.includes_ = $1;
          driver.structs_ = $2;
          driver.root_ = new ast::Program($3);
        }
        ;

includes : includes include { $$ = $1; $1->push_back($2); }
         |                  { $$ = new ast::IncludeList; }
         ;

include : INCLUDE STRING { $$ = new ast::Include($2, false); }
        | INCLUDE HEADER { $$ = new ast::Include($2.substr(1, $2.size()-2), true); }
        ;

structs : structs struct { $$ = $1; $1->push_back($2); }
        |                { $$ = new ast::StructList; }
        ;

struct : struct_or_union IDENT "{" fields "}" { $$ = new ast::Struct($2, $4); }
       ;

struct_or_union : STRUCT | UNION ;

fields : fields field ";" { $$ = $1; $1->insert($1->end(), $2->begin(), $2->end()); }
       |                  { $$ = new ast::FieldList; }
       ;

field : type_specifier field_names
      {
        $$ = new ast::FieldList;
        for (auto &decl : $2)
        {
          $$->push_back(new ast::Field($1.c_str(), decl.name.c_str(), decl.is_ptr, decl.array_size));
        }
      }
      ;

type_specifier : TYPE                  { $$ = $1; }
               | struct_or_union IDENT { $$ = $2; }
               ;

field_names : field_names "," declarator { $$ = $1; $$.push_back($3); }
            | declarator                 { $$.push_back($1); }
            ;

declarator : direct_declarator     { $$ = $1; $$.is_ptr = false; }
           | MUL direct_declarator { $$ = $2; $$.is_ptr = true; }
           ;

direct_declarator : IDENT                         { $$.name = $1; }
                  | direct_declarator "[" INT "]" { $$ = $1; $$.array_size = $3; }
                  ;

probes : probes probe { $$ = $1; $1->push_back($2); }
       | probe        { $$ = new ast::ProbeList; $$->push_back($1); }
       ;

probe : attach_points pred block { $$ = new ast::Probe($1, $2, $3); }
      ;

attach_points : attach_points "," attach_point { $$ = $1; $1->push_back($3); }
              | attach_point                   { $$ = new ast::AttachPointList; $$->push_back($1); }
              ;

attach_point : ident               { $$ = new ast::AttachPoint($1); }
             | ident ":" wildcard  { $$ = new ast::AttachPoint($1, $3); }
             | ident PATH wildcard { $$ = new ast::AttachPoint($1, $2.substr(1, $2.size()-2), $3); }
             | ident PATH INT      { $$ = new ast::AttachPoint($1, $2.substr(1, $2.size()-2), $3); }
             ;

wildcard : wildcard ident    { $$ = $1 + $2; }
         | wildcard MUL      { $$ = $1 + "*"; }
         | wildcard LBRACKET { $$ = $1 + "["; }
         | wildcard RBRACKET { $$ = $1 + "]"; }
         |                   { $$ = ""; }
         ;

pred : DIV expr ENDPRED { $$ = new ast::Predicate($2); }
     |                  { $$ = nullptr; }
     ;

block : "{" stmts "}"     { $$ = $2; }
      | "{" stmts ";" "}" { $$ = $2; }
      ;

stmts : stmts ";" stmt { $$ = $1; $1->push_back($3); }
      | stmt           { $$ = new ast::StatementList; $$->push_back($1); }
      ;

stmt : expr         { $$ = new ast::ExprStatement($1); }
     | map "=" expr { $$ = new ast::AssignMapStatement($1, $3); }
     | var "=" expr { $$ = new ast::AssignVarStatement($1, $3); }
     ;

expr : INT             { $$ = new ast::Integer($1); }
     | STRING          { $$ = new ast::String($1); }
     | BUILTIN         { $$ = new ast::Builtin($1); }
     | map             { $$ = $1; }
     | var             { $$ = $1; }
     | call            { $$ = $1; }
     | "(" expr ")"    { $$ = $2; }
     | expr EQ expr    { $$ = new ast::Binop($1, token::EQ, $3); }
     | expr NE expr    { $$ = new ast::Binop($1, token::NE, $3); }
     | expr LE expr    { $$ = new ast::Binop($1, token::LE, $3); }
     | expr GE expr    { $$ = new ast::Binop($1, token::GE, $3); }
     | expr LT expr    { $$ = new ast::Binop($1, token::LT, $3); }
     | expr GT expr    { $$ = new ast::Binop($1, token::GT, $3); }
     | expr LAND expr  { $$ = new ast::Binop($1, token::LAND,  $3); }
     | expr LOR expr   { $$ = new ast::Binop($1, token::LOR,   $3); }
     | expr PLUS expr  { $$ = new ast::Binop($1, token::PLUS,  $3); }
     | expr MINUS expr { $$ = new ast::Binop($1, token::MINUS, $3); }
     | expr MUL expr   { $$ = new ast::Binop($1, token::MUL,   $3); }
     | expr DIV expr   { $$ = new ast::Binop($1, token::DIV,   $3); }
     | expr MOD expr   { $$ = new ast::Binop($1, token::MOD,   $3); }
     | expr BAND expr  { $$ = new ast::Binop($1, token::BAND,  $3); }
     | expr BOR expr   { $$ = new ast::Binop($1, token::BOR,   $3); }
     | expr BXOR expr  { $$ = new ast::Binop($1, token::BXOR,  $3); }
     | LNOT expr       { $$ = new ast::Unop(token::LNOT, $2); }
     | BNOT expr       { $$ = new ast::Unop(token::BNOT, $2); }
     | MUL  expr %prec DEREF { $$ = new ast::Unop(token::MUL,  $2); }
     | expr DOT ident  { $$ = new ast::FieldAccess($1, $3); }
     | expr PTR ident  { $$ = new ast::FieldAccess(new ast::Unop(token::MUL, $1), $3); }
     | "(" cast_type ")" expr %prec CAST  { $$ = new ast::Cast($2, $4); }
     ;

cast_type : IDENT     { $$ = $1; }
          | IDENT MUL { $$ = $1 + "*"; }
          ;

ident : IDENT   { $$ = $1; }
      | BUILTIN { $$ = $1; }
      ;

call : ident "(" ")"       { $$ = new ast::Call($1); }
     | ident "(" vargs ")" { $$ = new ast::Call($1, $3); }
     ;

map : MAP               { $$ = new ast::Map($1); }
    | MAP "[" vargs "]" { $$ = new ast::Map($1, $3); }
    ;

var : VAR { $$ = new ast::Variable($1); }
    ;

vargs : vargs "," expr { $$ = $1; $1->push_back($3); }
      | expr           { $$ = new ast::ExpressionList; $$->push_back($1); }
      ;

%%

void bpftrace::Parser::error(const location &l, const std::string &m)
{
  driver.error(l, m);
}
