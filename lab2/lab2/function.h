#ifndef FUNCTION_H
#define FUNCTION_H

#include <string>
#include <vector>
#include <unordered_set>

using namespace std;

const int max_global_variables = 32768 / 8;

extern string *global_variables[max_global_variables];
extern int min_global_var_addr;

unordered_set<string> get_scalar_global_vars();

struct ParsedLine
{
    int line;
    string code;

    ParsedLine(int line, string code) : line(line), code(code) {}
};

struct Instruction
{
    int line;
    string op;
    vector<string> operands;

    void print();

    
};

struct Function
{
    vector<string *> parameters;
    vector<string *> local_variables;
    unordered_set<int> labeled_lines;
    int parameters_count;
    int start_line;
    bool is_main = false;

    vector<ParsedLine> parsed_codes;
    vector<Instruction> instructions;

    void add_parsed_code(int line, string code);

    string name();

    vector<string> get_scalar_local_variables();

    int local_variables_count();

    void set_local_variables_count(int count);

    void set_parameter_at_offset(string &name, int offset);

    string get_signature();

    void print_c();

    void print_3addr();

    Instruction &get_instruction_at_line(int line)
    {
        return instructions[line - start_line];
    }
};

extern vector<Function> functions;

void parse_functions();
void create_functions();

void print_headers();

void print_global_variables();

string parse_operand(string &operand, Function& func);

#endif