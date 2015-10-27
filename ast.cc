#include "ast.h"

std::map<std::string, std::string> op_to_instruction = {
  {"==", "test_equ"},
  {"!=", "test_nequ"},
  {"<", "test_less"},
  {"<=", "test_lte"},
  {">", "test_gtr"},
  {">=", "test_gte"},
  {"+", "add"},
  {"-", "sub"},
  {"*", "mult"},
  {"/", "div"},
};

std::set<std::string> relational_ops = {"==", "!=", "<", "<=", ">", ">="};

std::set<std::string> math_ops = {"+", "-", "*", "/", "&&", "||", "!", "+=", "-=", "*=", "/="};

void math_val_check(ASTNode_Base * node) {
  if (node -> getType() != Type::VAL) {
    yyerror("cannot use type in mathematical expression");
  }
}
