%{
  #include <iostream>
  #include <string.h>
  #include "ast.h"
  #include "tube.tab.hh"
  int line_number = 1;
%}

WHITESPACE [ \t\r]
ID [_A-Za-z][_A-Za-z0-9]*
VAL_LITERAL [0-9]+|[0-9]*([.][0-9]+)
CHAR_LITERAL "'"."'"|"'"[\\]n"'"|"'"[\\]t"'"|"'"[\\][']"'"|"'"[\\][\\]"'"
STRING_LITERAL \"(\\.|[^"])*\"
NON_TERMINATING_STRING "\""[^"\""]*
ASCII_CHAR "+"|"-"|"*"|"/"|"("|")"|"="|","|"{"|"}"|"["|"]"|"."|";"|"!"
SINGLE_LINE_COMMENT #[^\n]*
%x IN_COMMENT
  
%%

 // From http://flex.sourceforge.net/manual/How-can-I-match-C_002dstyle-comments_003f.html
<INITIAL>{
  "/*" BEGIN(IN_COMMENT);
}
<IN_COMMENT>{
"*/"      BEGIN(INITIAL);
[^*\n]+   // eat comment in chunks
"*"       // eat the lone star
\n        line_number++;
}

{SINGLE_LINE_COMMENT} {
  // Nothing to do
}  

{WHITESPACE} {
  //Nothing to do
}

val {
  yylval.type_id = Type::VAL;
  return TYPE;
}

char {
  yylval.type_id = Type::CHAR;
  return TYPE;
}

string {
  yylval.type_id = Type::STRING;
  return TYPE;
}

array {
  return ARRAY;
}

print {
  return COMMAND_PRINT;
}

random {
  return COMMAND_RANDOM;
}

if {
  return IF;
}

else {
  return ELSE;
}

while {
  return WHILE;
}

break {
  return BREAK;
}

continue {
  return CONTINUE;
}

{ID} {
  yylval.lexeme = strdup(yytext);
  return ID;
}

{VAL_LITERAL} {
  yylval.lexeme = strdup(yytext);
  return VAL_LITERAL;
}

{CHAR_LITERAL} {  
  yylval.lexeme = strdup(yytext);
  return CHAR_LITERAL;
}

{STRING_LITERAL} {
  yylval.lexeme = strdup(yytext);
  return STRING_LITERAL;
}

{ASCII_CHAR} {
  return yytext[0];
}

"+=" {
  return ASSIGN_ADD;
}

"-=" {
  return ASSIGN_SUB;
}

"*=" {
  return ASSIGN_MULT;
}

"/=" {
  return ASSIGN_DIV;
}

"==" {
  return COMP_EQU;
}

"!=" {
  return COMP_NEQU;
}

"<" {
  return COMP_LESS;
}

"<=" {
  return COMP_LTE;
}

">" {
  return COMP_GTR;
}

">=" {
  return COMP_GTE;
}

"&&" {
  return BOOL_AND;
}

"||" {
  return BOOL_OR;
}

\n {
  ++line_number;
}

{NON_TERMINATING_STRING} {
  std::cout << "ERROR(line " << line_number << "): Unterminating string." << std::endl;
  exit(1);
}

. {
  std::cout << "Unknown token on line " << line_number 
       << ": " << yytext << std::endl;
  exit(1);
}
     
%%
