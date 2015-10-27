%{
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <fstream>
#include "symbol_table.h"
#include "ast.h"

extern int line_number;
extern int yylex();
extern FILE * yyin;

// Prints the given error message to std::cout and halts
void yyerror(std::string err_string) {
  std::cout << "ERROR(line " << line_number << "): "
            << err_string << std::endl;
  exit(1);
}

// Global Variables
SymbolTable table;
std::ofstream fout;

// Helper function
void check_already_declared(std::string name) {
  if (table.declaredInCurrentScope(name)) {
    yyerror("redeclaration of variable '" + name + "'");
  }
}

%}

%union {
  char * lexeme;
  Type::Typenames type_id;
  ASTNode_Base * ast_node;
}


%token <lexeme> VAL_LITERAL CHAR_LITERAL ID STRING_LITERAL
%token <type_id> TYPE

%token COMMAND_PRINT COMMAND_RANDOM WHILE BREAK CONTINUE IF ELSE ARRAY
 
%right '=' ASSIGN_ADD ASSIGN_SUB ASSIGN_MULT ASSIGN_DIV
%left BOOL_OR
%left BOOL_AND
%left COMP_EQU COMP_NEQU COMP_LESS COMP_LTE COMP_GTR COMP_GTE
%left '-' '+'
%left '*' '/'
%nonassoc UMINUS '!'

%nonassoc LONE_IF
%nonassoc ELSE

 // All the rule nodes in no particular order 
%type <ast_node> expression program statement_list statement declaration print_statement 
%type <ast_node> one_or_more_arguments assignment var_usage var_declare 
%type <ast_node> statement_block if_statement while_statement BREAK CONTINUE simple_id

%%

program: statement_list {
  //Runs last (after parsing)
  
  // Debug line
  //$1 -> debug();

  fout << "# Joshua Nahum's TubeIC Output" << std::endl;
  $1 -> process();
}

statement_list: {
  $$ = new ASTNode_Root();
}
| statement_list statement { $1 -> addChild($2); }

statement: expression ';' { $$ = $1; }
| print_statement ';' { $$ = $1; }
| declaration ';' { $$ = $1; }
| statement_block { $$ = $1; } // Code block are a statement 
| if_statement { $$ = $1; }
| ';' { $$ = new ASTNode_Root(); } // Allows Empty Statement
| while_statement { 
  $$ = $1; 
  }
| BREAK ';' { $$ = new ASTNode_Break(); } 
| CONTINUE ';' { $$ = new ASTNode_Continue(); }

// Note the LONE_IF precedence to handle dangling else's
// All if statements have an else block, but only some compile code there
if_statement: IF '(' expression ')' statement %prec LONE_IF { $$ = new ASTNode_If($3, $5, NULL); }
| IF '(' expression ')' statement ELSE statement {
  $$ = new ASTNode_If($3, $5, $7);
}

// Note the calls to the symbol table to push and pop from the while stack
while_statement: WHILE { table.push_to_while_stack(); } '(' expression ')' statement { 
  $$ = new ASTNode_While($4, $6); 
  table.pop_from_while_stack();
}

// Alternate method to ensure order of execution
// Statement blocks are composed of curly brackets and a statement list
statement_block: block_open statement_list block_close { $$ = $2; }
block_open: '{' { table.incrementScope(); }
block_close: '}' { table.decrementScope(); }

declaration: var_declare {
  $$ = $1;
}
| var_declare '=' expression {
  $$ = new ASTNode_Assign($1, $3);
}

var_declare: TYPE ID {
  std::string string_name($2);
  check_already_declared(string_name);
  table.addEntry($1, string_name);
  $$ = new ASTNode_Variable(table.getEntry(string_name));
}

print_statement: COMMAND_PRINT '(' one_or_more_arguments ')' {
  $$ = new ASTNode_Print();
  $$ -> transferChildren($3);
  delete $3;
}

one_or_more_arguments : expression { 
  $$ = new ASTNode_Temp(Type::VOID); 
  $$ -> addChild($1);
}
| one_or_more_arguments ',' expression {
  $1 -> addChild($3);
  $$ = $1;
}

expression: COMMAND_RANDOM '(' expression ')' { $$ = new ASTNode_Random($3); }
| expression COMP_EQU expression { $$ = new ASTNode_Math2($1, $3, "=="); }
| expression COMP_NEQU expression { $$ = new ASTNode_Math2($1, $3, "!="); }
| expression COMP_LESS expression { $$ = new ASTNode_Math2($1, $3, "<"); }
| expression COMP_LTE expression { $$ = new ASTNode_Math2($1, $3, "<="); }
| expression COMP_GTR expression { $$ = new ASTNode_Math2($1, $3, ">"); }
| expression COMP_GTE expression { $$ = new ASTNode_Math2($1, $3, ">="); }
| expression BOOL_AND expression { $$ = new ASTNode_BoolAnd($1, $3); }
| expression BOOL_OR expression { $$ = new ASTNode_BoolOr($1, $3); }
| expression '+' expression { $$ = new ASTNode_Math2($1, $3, "+"); }
| expression '-' expression { $$ = new ASTNode_Math2($1, $3, "-"); }
| expression '*' expression { $$ = new ASTNode_Math2($1, $3, "*"); }
| expression '/' expression { $$ = new ASTNode_Math2($1, $3, "/"); }
| '-' expression %prec UMINUS { $$ = new ASTNode_Math1_Minus($2); }
| '(' expression ')' { $$ = $2; }
| VAL_LITERAL { $$ = new ASTNode_Literal(Type::VAL, $1); }
| CHAR_LITERAL { $$ = new ASTNode_Literal(Type::CHAR, $1); }
| assignment { $$ = $1; }
| var_usage { $$ = $1; }
| '!' expression { $$ = new ASTNode_Not($2); }

assignment: var_usage '=' expression { $$ = new ASTNode_Assign($1, $3); }
| var_usage ASSIGN_ADD expression { $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, "+")); }
| var_usage ASSIGN_SUB expression { $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, "-")); }
| var_usage ASSIGN_MULT expression { $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, "*")); }
| var_usage ASSIGN_DIV expression { $$ = new ASTNode_Assign($1, new ASTNode_Math2($1, $3, "/")); }

var_usage: simple_id { $$ = $1; }

simple_id: ID {
  std::string string_name($1);
  if (!table.declared(string_name)) {
    yyerror("unknown variable '" + string_name + "'");
  }
  TableEntry * entry = table.getEntry(string_name);
  $$ = new ASTNode_Variable(entry);
}

%%

int main(int argc, char * argv[])
{
  // Make sure we have exactly one argument.
  if (argc != 3) {
    std::cerr << "Format: " << argv[0] << " [filename]" << std::endl;
    exit(1);
  }
  
  // Assume the argument is the filename.
  yyin = fopen(argv[1], "r");
  if (!yyin) {
    std::cerr << "Error opening " << argv[1] << std::endl;
    exit(1);
  }
  
  fout.open(argv[2]);

  // Parse the file!  If it doesn't exit, parse must have been successful.
  yyparse();
  std::cout << "Parse Successful!" << std::endl;

  fclose(yyin);
  fout.close();
  return 0;
}
