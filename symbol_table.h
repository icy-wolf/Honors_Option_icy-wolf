#ifndef symbol_table
#define symbol_table
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <vector>

namespace Type {
  enum Typenames { VOID = 0, VAL, CHAR, STRING, VAL_ARRAY };
}

class TableEntry {
 private:
  Type::Typenames type_id;
  std::string name;
  int location;

 public:
  TableEntry()  {}
  TableEntry(Type::Typenames in_type, int in_location) 
    : type_id(in_type), location(in_location) {}

  TableEntry(Type::Typenames in_type, std::string in_name, int in_location) 
    : type_id(in_type), name(in_name), location(in_location) {}

  // Nice method for allowing << to output the entry's location (s# or a#)
  friend std::ostream& operator<<(std::ostream& os, TableEntry& entry) {
     os << entry.getLocation();
     return os;
  }

  ~TableEntry() {}
  Type::Typenames getType() { return type_id; }
  std::string getName() { return name; }
  
  // Returns a string representing the entry's memory location
  std::string getLocation() { 
    std::string letter;
    if (type_id == Type::VAL || type_id == Type::CHAR) letter = "s";
    else {
      std::cout << "ERROR: Internal compiler error: don't know how to handle that type in memory yet: " 
		<< name << std::endl;
      exit(1);
    }

    std::ostringstream output;
    output << letter << location;
    return output.str(); 
  }
};

class SymbolTable {
  private:
  // The stack of tables representing all the scopes
  std::vector<std::map<std::string, TableEntry * > > table_stack;
  // The list of entries (for use later)
  std::vector<TableEntry * > archive;

  // Counters for generating unique memory locations and labels
  int location_allotment;
  int label_allotment;

  // Stack of while contexts for break and continue
  std::vector<std::string> while_stack;

  public:
  SymbolTable() {
    // When a table is created, we need a global scope
    incrementScope();
  }

  ~SymbolTable() {
    decrementScope();
  }

 private:
  // Descends the scopes until identifier is found, otherwise returns null 
  TableEntry * find(std::string name) {
    for (int i = table_stack.size() - 1; i >= 0; --i) {
      std::map<std::string, TableEntry *> table = table_stack[i];
      if (table.find(name) != table.end()) {
	return table[name];
      }
    }
    return NULL;
  }

 public:  
  void push_to_while_stack() {
    while_stack.push_back(getNextLabel("while"));
    std::cout << "Pushing to stack " << while_stack_top() << std::endl;   
  }

  std::string while_stack_top() {
    return while_stack[while_stack.size() - 1];
  }

  int while_stack_size() {
    return while_stack.size();
  }

  void pop_from_while_stack() {
    std::cout << "Popping from stack " << std::endl;     
    std::string top = while_stack_top();
    while_stack.pop_back();
}

  int getNextLocation() {
    return ++location_allotment;
  }

  void incrementScope() {
    std::map<std::string, TableEntry *> s_table;
    table_stack.push_back(s_table);
  }
  
  void decrementScope() {
    std::map<std::string, TableEntry *> finished = table_stack[table_stack.size() - 1];
    for (std::map<std::string, TableEntry *>::iterator it = finished.begin(); 
	 it != finished.end(); ++it) {
      archive.push_back(it -> second);
    }
    table_stack.pop_back();
  }

  std::string getNextLabel(std::string label) {
    std::ostringstream output;
    output << label << "_" << ++label_allotment;
    return output.str();
  }

  // Adds variable to current scope
  void addEntry(Type::Typenames in_type, std::string in_name) {
    table_stack[table_stack.size() - 1][in_name] = new TableEntry(in_type, in_name, getNextLocation());
  }

  // Used to create TableEntries by the ASTNodes
  TableEntry * createEntry(Type::Typenames in_type) {
    TableEntry * entry = new TableEntry(in_type, getNextLocation());
    archive.push_back(entry);
    return entry;
  }

  // Returns a pointer to to TableEntry associated with a variable
  TableEntry * getEntry(std::string name) {
    TableEntry * entry = find(name);
    if (entry == NULL) {
      std::cout << "ERROR: Internal compiler error: getEntry on unknown name: " 
		<< name << std::endl;
      exit(1);
    }
    return entry;
  }

  // Returns true if name has been declared in some scope
  bool declared(std::string in_name) {
    TableEntry * entry = find(in_name);
    return entry != NULL;
  }

  // Returns true if name has been declared in current scope
  bool declaredInCurrentScope(std::string in_name) {
    std::map<std::string, TableEntry *> scope = table_stack[table_stack.size() - 1];
    return scope.find(in_name) != scope.end();
  }
};

#endif
