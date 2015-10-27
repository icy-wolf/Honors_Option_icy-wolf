#ifndef AST_H
#define AST_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include "symbol_table.h"

extern void yyerror(std::string);
extern SymbolTable table;
extern std::ofstream fout;

using std::endl;

// Helper objects from ast.cc
extern std::map<std::string, std::string> op_to_instruction;
extern std::set<std::string> math_ops;
extern std::set<std::string> relational_ops;


class ASTNode_Base {
protected:
  Type::Typenames type;                            
  std::vector<ASTNode_Base *> children; 
  
public:
 ASTNode_Base(Type::Typenames in_type) : type(in_type) { ; }
  virtual ~ASTNode_Base() {
    for (int i = 0; i < children.size(); i++) {
      delete children[i];
    }
  }

  Type::Typenames getType() { return type; }

  ASTNode_Base * getChild(int id) { return children[id]; }
  int getNumChildren() { return children.size(); }
  void addChild(ASTNode_Base * in_child) { children.push_back(in_child); }
  void transferChildren(ASTNode_Base * in_node) {
    for (int i = 0; i < in_node->getNumChildren(); i++) {
      addChild(in_node->getChild(i));
    }
    in_node->children.resize(0);
  }

  // Process a single node's calculations and return a TableEntry that
  // represents the result.  Call child nodes recursively....
  virtual TableEntry * process() { return NULL; }
  virtual void debug(int indent=0) {
    // Print this class' name
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << getName()
	      << " [" << children.size() << " children]"
	      << std::endl;

    // Then run Debug on the children if there are any...
    for (int i = 0; i < children.size(); i++) {
      children[i]->debug(indent+1);
    }
  }

  virtual std::string getName() { return "ASTNode_Base"; }
};

// Helper function from ast.cc
// Needs be declared after ASTNode_Base
extern void math_val_check(ASTNode_Base * node);

// This node is built as a placeholder in the AST.
class ASTNode_Temp : public ASTNode_Base {
public:
  ASTNode_Temp(Type::Typenames in_type) : ASTNode_Base(in_type) { ; }
  ~ASTNode_Temp() { ; }
  virtual std::string getName() {
    return "ASTNode_Temp (under construction)";
  }
};

// Root...
class ASTNode_Root : public ASTNode_Base {
public:
  ASTNode_Root() : ASTNode_Base(Type::VOID) { ; }

  virtual std::string getName() { return "ASTNode_Root (container class)"; }

  virtual TableEntry * process() {
    fout << "# Processing Root" << endl;
    for (int i = 0; i < children.size(); i++) {
      children[i]->process();
    }
    return NULL;
  }    
};

// Leaves...

class ASTNode_Variable : public ASTNode_Base {
private:
  TableEntry * var_entry;
public:
  ASTNode_Variable(TableEntry * in_entry)
    : ASTNode_Base(in_entry->getType()), var_entry(in_entry) {;}

  virtual TableEntry * process() {
    fout << "# Process Variable" << endl;
    return var_entry;
  }
  
  virtual std::string getName() {
    std::string out_string = "ASTNode_Variable (";
    out_string += var_entry->getName();
    out_string += ")";
    return out_string;
  }
};

class ASTNode_Literal : public ASTNode_Base {
private:
  std::string lexeme;                 
public:
  ASTNode_Literal(Type::Typenames in_type, std::string in_lex)
    : ASTNode_Base(in_type), lexeme(in_lex) { 
}  

  virtual TableEntry * process() {
    // Handles the val and char literals
    if (type == Type::VAL || type == Type::CHAR) {
      TableEntry * var = table.createEntry(type);
      fout << "val_copy " << lexeme << " " << *var << endl;
      return var;
    } 
    yyerror("Internal Compiler Error: AST_Node Literal handling wrong type");
  }

  virtual std::string getName() {
    std::string out_string = "ASTNode_Literal (";
    out_string += lexeme;
    out_string += ")";
    return out_string;
  }
};

class ASTNode_Assign : public ASTNode_Base {
public:
  ASTNode_Assign(ASTNode_Base * lhs, ASTNode_Base * rhs)
    : ASTNode_Base(lhs->getType()) {  
    children.push_back(lhs);  
    children.push_back(rhs);  
    if(lhs -> getType() != rhs -> getType()) {
      yyerror("types do not match for assignment");
    }
  }
  ~ASTNode_Assign() { ; }

  virtual TableEntry * process() {
    fout << "# Process Assign" << endl;
    TableEntry * lhs_entry = children[0] -> process();
    TableEntry * rhs_entry = children[1] -> process();

    std::string copy_instruction;
    if (type == Type::VAL || type == Type::CHAR) {
      copy_instruction = "val_copy";
    } else {
      yyerror("Internal Compiler Error: Assign doesn't handle this type yet");
    }
    fout << copy_instruction << " " << *rhs_entry << " " << *lhs_entry << endl;
    return lhs_entry;
  }
  
  virtual std::string getName() { return "ASTNode_Assign (operator=)"; }
};

class ASTNode_Math1_Minus : public ASTNode_Base {
public:
  ASTNode_Math1_Minus(ASTNode_Base * child) : ASTNode_Base(Type::VAL)
  { 
    children.push_back(child);
    math_val_check(child);
  }
  ~ASTNode_Math1_Minus() { ; }

  virtual TableEntry * process() {
    TableEntry * entry = children[0] -> process();
    TableEntry * result = table.createEntry(Type::VAL);
    fout << "# Unary Minus" << endl;
    fout << "sub 0 " << *entry << " " << *result << endl;
    return result;
  }

  virtual std::string getName() { return "ASTNode_Math1_Minus (unary -)"; }
};

class ASTNode_Math2 : public ASTNode_Base {
protected:
  std::string math_instruction;
public:
  ASTNode_Math2(ASTNode_Base * in1, ASTNode_Base * in2, std::string op) : ASTNode_Base(Type::VAL) {
    children.push_back(in1);
    children.push_back(in2);

    // Type checking
    Type::Typenames type_1 = in1 -> getType(); 
    Type::Typenames type_2 = in2 -> getType(); 
    
    // Check Relationship Operators 
    if((relational_ops.find(op) != relational_ops.end()) && 
       (type_1 != type_2)) {
      yyerror("types do not match for relationship operator.");
    }
    
    // Check Math Operators
    if (math_ops.find(op) != math_ops.end()) { 
      math_val_check(in1);
      math_val_check(in2);
    } 

    math_instruction = op_to_instruction[op];
  }

  virtual ~ASTNode_Math2() { ; }

  virtual TableEntry * process() {
    TableEntry * lhs = children[0] -> process();
    TableEntry * rhs = children[1] -> process();
    TableEntry * result = table.createEntry(Type::VAL);

    fout << "# Math2 " << math_instruction << endl;
    fout << math_instruction << " " << *lhs << " " << *rhs << " " << *result << endl;
    return result;
  }

  virtual std::string getName() {
    std::string out_string = "ASTNode_Math2 (operator";
    out_string += math_instruction;
    out_string += ")";
    return out_string;
  }
};

class ASTNode_BoolAnd : public ASTNode_Base {
public:
 ASTNode_BoolAnd(ASTNode_Base * in1, ASTNode_Base * in2) : ASTNode_Base(Type::VAL) {
    children.push_back(in1);
    children.push_back(in2);
    math_val_check(in1);
    math_val_check(in2);
  }
  virtual ~ASTNode_BoolAnd() { ; }

  virtual TableEntry * process() {
    fout << "# Process And" << endl;
    TableEntry * lhs_entry = children[0] -> process();
    TableEntry * result_is_true = table.createEntry(Type::VAL);
    fout << "test_nequ " << *lhs_entry << " 0 " << *result_is_true << endl;
    std::string label = table.getNextLabel("And_ShortCircut_");
    fout << "jump_if_0 " << *result_is_true << " " << label << endl;
    TableEntry * rhs_entry = children[1] -> process();
    fout << "test_nequ " << *rhs_entry << " 0 " << *result_is_true << endl;
    fout << label << ":" << endl;
    return result_is_true;
  }
  
  virtual std::string getName() { return "ASTNode_BoolAnd"; }
};

class ASTNode_BoolOr : public ASTNode_Base {
public:
 ASTNode_BoolOr(ASTNode_Base * in1, ASTNode_Base * in2) : ASTNode_Base(Type::VAL) {
    children.push_back(in1);
    children.push_back(in2);
    math_val_check(in1);
    math_val_check(in2);
  }
  virtual ~ASTNode_BoolOr() { ; }

  virtual TableEntry * process() {
    fout << "# Process Or" << endl;
    TableEntry * lhs_entry = children[0] -> process();
    TableEntry * result_is_true = table.createEntry(Type::VAL);
    fout << "test_nequ " << *lhs_entry << " 0 " << *result_is_true <<  endl;
    std::string label = table.getNextLabel("Or_ShortCircuit_");
    fout << "jump_if_n0 " << *result_is_true << " " << label << endl;
    TableEntry * rhs_entry = children[1] -> process();
    fout << "test_nequ " << *rhs_entry << " 0 " << *result_is_true << endl;
    fout << label << ":" << endl;
    return result_is_true;
  }

  virtual std::string getName() { return "ASTNode_BoolOr"; }
};


class ASTNode_Print : public ASTNode_Base {
public:
  ASTNode_Print() : ASTNode_Base(Type::VOID) { ; }
  virtual ~ASTNode_Print() {;}

  virtual TableEntry * process() {
    fout << "# Process Print" << endl;
    // Output each child
    for (int i = 0; i < children.size(); i++) {
      TableEntry * item = children[i] -> process();

      // Figure out what to do based on type
      Type::Typenames item_type = item -> getType();
      std::string out_instruction;
      if (item_type == Type::VAL) {
	out_instruction = "out_val";
      } else if (item_type == Type::CHAR) {
	out_instruction = "out_char";
      } else {
	yyerror("Internal Compiler Error: trying to print an type I can't handle yet");
      }
      fout << out_instruction << " " << item -> getLocation() << endl;
    }
    fout << "out_char '\\n'" << endl;
    return NULL;
  }

  virtual std::string getName() { return "ASTNode_Print (print command)"; }
};

class ASTNode_Random : public ASTNode_Base {
public:
  ASTNode_Random(ASTNode_Base * in) : ASTNode_Base(Type::VAL) { 
    children.push_back(in); 
    if (in -> getType() != Type::VAL) {
      yyerror("cannot use type as argument to random");
    }
  }

  virtual ~ASTNode_Random() {;}

  virtual TableEntry * process() {
    fout << "# Process Random" << endl;
    TableEntry * input = children[0] -> process();
    TableEntry * result = table.createEntry(Type::VAL);
    fout << "random " << *input << " " << *result << endl;
    return result; 
  }

  virtual std::string getName() { return "ASTNode_Random (random command)"; }
};

class ASTNode_Not : public ASTNode_Base {
public:
  ASTNode_Not(ASTNode_Base * in) : ASTNode_Base(Type::VAL) { 
    children.push_back(in);
    math_val_check(in);
  }
  virtual ~ASTNode_Not() {;}

  virtual TableEntry * process() {
    fout << "# Process Not" << endl;
    TableEntry * input = children[0] -> process();
    TableEntry * result = table.createEntry(Type::VAL);
    fout << "test_equ 0 " << *input << " " << *result << endl;
    return result;
  }

  virtual std::string getName() { return "ASTNode_Not (!)"; }
};

class ASTNode_If : public ASTNode_Base {
public:
 ASTNode_If(ASTNode_Base * cond, ASTNode_Base * if_statement, ASTNode_Base * else_statement) 
   : ASTNode_Base(Type::VOID) { 
    children.push_back(cond);
    children.push_back(if_statement);
    children.push_back(else_statement);
    Type::Typenames cond_type = cond -> getType();
    if (cond_type != Type::VAL) {
      yyerror("condition for if statements must evaluate to type val");
    }
  }

  virtual ~ASTNode_If() {;}

  virtual TableEntry * process() {
    fout << "# Process If" << endl;
    TableEntry * cond = children[0] -> process();
    std::string else_start = table.getNextLabel("else_start_");
    fout << "jump_if_0 " << *cond << " " << else_start << endl;
    fout << "#If True Statement" << endl;
    children[1] -> process();
    std::string else_end = table.getNextLabel("else_end_");
    fout << "jump " << else_end << endl;
    fout << else_start << ":" << endl;
    fout << "#If False Statement" << endl;
    // Process Else Statement if it exists
    if (children[2] != NULL) {
      children[2] -> process();
    }
    fout << else_end << ":" << endl;  
    return NULL;
  }

  virtual std::string getName() { return "ASTNode_If"; }
};

class ASTNode_While : public ASTNode_Base {
public:
  std::string while_label;
  ASTNode_While(ASTNode_Base * cond, ASTNode_Base * statement) 
    : ASTNode_Base(Type::VOID) { 
    children.push_back(cond);
    children.push_back(statement);
    while_label = table.while_stack_top();
    Type::Typenames cond_type = cond -> getType();
    if (cond_type != Type::VAL) {
      yyerror("condition for while statements must evaluate to type val");
    }
  }
  virtual ~ASTNode_While() {;}

  virtual TableEntry * process() {
    fout << "# Process While" << endl;
    std::string while_start = while_label + "_start";    
    std::string while_end = while_label + "_end";
    
    fout << while_start << ":" << endl;
    TableEntry * cond = children[0] -> process();
    fout << "jump_if_0 " << *cond << " " << while_end << endl;

    children[1] -> process();

    fout << "jump " << while_start << endl;
    fout << while_end << ":" << endl;
    
    return NULL;
  }

  virtual std::string getName() { return "ASTNode_While"; }
};

class ASTNode_Break : public ASTNode_Base {
public:
  std::string while_label;
  ASTNode_Break() 
    : ASTNode_Base(Type::VOID) { 
    if (table.while_stack_size() <= 0) {
      yyerror("'break' command used outside of any loop");
    }   
    while_label = table.while_stack_top();
  }

  virtual ~ASTNode_Break() {;}

  virtual TableEntry * process() {
    fout << "# Process Break" << endl;
    fout << "jump " << while_label << "_end"<< endl;    
    return NULL;
  }

  virtual std::string getName() { return "ASTNode_Break"; }
};

class ASTNode_Continue : public ASTNode_Base {
public:
  std::string while_label;
  ASTNode_Continue() 
    : ASTNode_Base(Type::VOID) { 
    if (table.while_stack_size() <= 0) {
      yyerror("'continue' command used outside of any loop");
    }
    while_label = table.while_stack_top();
  }
  virtual ~ASTNode_Continue() {;}

  virtual TableEntry * process() {
    fout << "# Process Continue" << endl;
    fout << "jump " << while_label << "_start" << endl;
    return NULL;
  }

  virtual std::string getName() { return "ASTNode_Continue"; }
};

#endif
