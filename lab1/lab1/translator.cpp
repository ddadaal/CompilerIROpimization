#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <assert.h>
#include <algorithm>

using namespace std;

char buff[200];

template <typename... Args>
string string_format(const char *format, Args... args)
{
    snprintf(buff, sizeof(buff), format, args...);
    return buff;
}

string string_join(vector<string> &vector, const char *delimiter)
{
    string result;
    for (auto iter = vector.begin(); iter != vector.end(); iter++)
    {
        result += *iter;
        if (iter != vector.end() - 1)
        {
            result += delimiter;
        }
    }
    return result;
}

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
};

struct Function
{
    vector<string *> parameters;
    vector<string *> local_variables;
    unordered_set<int> labeled_lines;
    int id;
    int parameters_count;
    int start_line;
    bool is_main = false;

    vector<ParsedLine> parsed_codes;
    vector<Instruction> instructions;

    template <typename... Args>
    void add_parsed_code(int line, const char *format, Args... args)
    {
        parsed_codes.emplace_back(line, string_format(format, args...));
    }

    string name()
    {
        return is_main ? "main" : string_format("function_%d", id);
    }

    int local_variables_count()
    {
        return local_variables.size();
    }

    void set_local_variables_count(int count)
    {
        for (int i = 0; i < count; i++)
        {
            local_variables.push_back(nullptr);
        }
    }

    void set_parameter_at_offset(string &name, int offset)
    {
        // if the size of parameters is smaller than offset
        // insert nullptr
        for (int i = parameters.size(); i <= offset; i++)
        {
            parameters.push_back(nullptr);
        }

        // set the parameter into the offset
        parameters[offset] = new string(name);
    }

    string signature()
    {
        vector<string> parameter_definitions;
        // the parameters are reverse order
        // TODO unused parameters
        for (auto iter = parameters.rbegin(); iter != parameters.rend(); iter++)
        {
            // all parameters are guranteed LL
            string def = string_format("LL %s", (**iter).c_str());
            parameter_definitions.push_back(def);
        }

        return string_format("void %s(%s)",
                             name().c_str(),
                             string_join(parameter_definitions, ", ").c_str());
    }
};

Instruction instr;

bool get_instruction()
{
    instr.operands.clear();

    string buffer;

    // eat instr
    if (!(cin >> buffer))
    {
        return false;
    }

    // get the line number
    cin >> instr.line;

    // eat the :
    cin >> buffer;

    // get the op
    cin >> instr.op;

    // add the rest as operands until reaching a line break
    while (cin.peek() != '\n')
    {
        cin >> buffer;
        instr.operands.push_back(buffer);
    }

    return true;
}

vector<Function> functions;

Function *curr_function;

Function &get_function_at_line(int line)
{
    for (int i = 0; i < functions.size(); i++)
    {
        if (functions[i].start_line == line)
        {
            return functions[i];
        }
    }
    assert(false);
}

bool ends_with(string &value, const char *ending)
{
    string ending_str = ending;
    if (ending_str.size() > value.size())
    {
        return false;
    }
    return equal(ending_str.rbegin(), ending_str.rend(), value.rbegin());
}

const int max_global_variables = 32768 / 8;

string *global_variables[max_global_variables];
int min_global_var_addr = max_global_variables;

// [11] -> 11
// (12) -> r12
// a_base#128 -> a
// 12 -> 12
// x_offset#8 -> 1
// struct inside function is not supported, so the all struct member offset can be replaced by the offset directly
// this function also handles adding parameters, local variables and global variables
// return the name of operand in code
string parse_operand(string &operand)
{
    if (operand[0] == '[')
    {
        return operand.substr(1, operand.size() - 2);
    }
    if (operand[0] == '(')
    {
        string name;
        name += 'r';
        name += operand.substr(1, operand.size() - 2);
        return name;
    }

    auto hash_index = operand.find("#");
    auto name = operand.substr(0, hash_index);

    if (hash_index == operand.npos)
    {
        return name;
    }

    int offset = stoi(operand.substr(hash_index + 1)) / 8;

    // if the name ends with _offset, just return the offset
    // since we are parsing struct as array directly
    if (ends_with(name, "_offset"))
    {
        return to_string(offset * 8);
    }

    // if the name ends with _base, remove it
    if (ends_with(name, "_base"))
    {
        name = name.substr(0, name.size() - 5);
    }

    // hack: if the offset >= 8192, consider the variable a global variable
    if (offset >= 8192 / 8)
    {
        if (global_variables[offset] == nullptr)
        {
            // assign a new global variable
            global_variables[offset] = new string(name);
            min_global_var_addr = min(min_global_var_addr, offset);
        }
    }
    else
    {
        if (offset < 0)
        {
            // it's local variable

            // convert the offset to the offset in the local_variables vector
            // and assign the local_variables array
            int i = (-offset) - 1;
            if (curr_function->local_variables[i] == nullptr)
            {
                curr_function->local_variables[i] = new string(name);
            }
        }
        else
        {
            // it's a parameter
            // convert the offset to the offset in the paramters vector
            // note: the parameters vector contains the parameters in reverse order
            int i = (offset)-2;
            curr_function->set_parameter_at_offset(name, i);
        }
    }

    return name;
}

void print_indentation()
{
    printf("    ");
}

unordered_map<string, const char *> operators;

int main()
{
    // populate operators
    operators["sub"] = "LL r%d = %s - %s;";
    operators["mul"] = "LL r%d = %s * %s;";
    operators["div"] = "LL r%d = %s / %s;";
    operators["mod"] = "LL r%d = %s %% %s;";
    operators["cmpeq"] = "LL r%d = %s == %s;";
    operators["cmple"] = "LL r%d = %s <= %s;";
    operators["cmplt"] = "LL r%d = %s < %s;";

    bool next_function_main = false;

    stack<string> parameters;

    decltype(operators.end()) op_format;

    // get all functions
    // need to get the parameter count to handle recursive call
    while (get_instruction())
    {
        if (instr.op == "nop")
        {
            continue;
        }
        if (instr.op == "entrypc")
        {
            next_function_main = true;
        }
        else if (instr.op == "enter")
        {
            functions.emplace_back();
            curr_function = &functions.back();
            curr_function->is_main = next_function_main;
            next_function_main = false;

            curr_function->id = functions.size();
            curr_function->set_local_variables_count(stoi(instr.operands[0]) / 8);
            curr_function->start_line = instr.line;
        }
        else if (instr.op == "ret")
        {
            curr_function->parameters_count = stoi(instr.operands[0]) / 8;
            curr_function->instructions.push_back(instr);
        }
        else
        {
            curr_function->instructions.push_back(instr);
        }
    }

    // for each function, parse instructions
    for (Function &func : functions)
    {
        curr_function = &func;
        for (Instruction &instr : func.instructions)
        {

            if (instr.op == "add")
            {
                string &first = instr.operands[0], &second = instr.operands[1];
                // if the second is GP or FP, consider it as a get address op
                if (second == "GP" || second == "FP")
                {
                    string name = parse_operand(first);
                    func.add_parsed_code(instr.line, "LL r%d = (LL)&%s;", instr.line, name.c_str());
                }
                else
                {
                    auto first = parse_operand(instr.operands[0]);
                    auto second = parse_operand(instr.operands[1]);

                    // print LL r{line} = {first.name} + {second.name};
                    func.add_parsed_code(instr.line, "LL r%d = %s + %s;", instr.line, first.c_str(), second.c_str());
                }
            }
            else if (operators.find(instr.op.c_str()) != operators.end())
            {
                auto first = parse_operand(instr.operands[0]);
                auto second = parse_operand(instr.operands[1]);

                const char *&format = operators[instr.op.c_str()];
                func.add_parsed_code(instr.line, format, instr.line, first.c_str(), second.c_str());
            }
            else if (instr.op == "neg")
            {
                auto first = parse_operand(instr.operands[0]);
                func.add_parsed_code(instr.line, "LL r%d = -%s;", instr.line, first.c_str());
            }
            else if (instr.op == "br")
            {
                auto target = parse_operand(instr.operands[0]);
                func.labeled_lines.insert(stoi(target));

                func.add_parsed_code(instr.line, "goto l%s;", target.c_str());
            }
            else if (instr.op == "blbc" || instr.op == "blbs")
            {
                auto cond = parse_operand(instr.operands[0]);
                auto target = parse_operand(instr.operands[1]);

                // the target line need to be labelled
                func.labeled_lines.insert(stoi(target));

                if (instr.op == "blbs")
                {
                    // blbs: print if ({cond}) goto l{target};
                    func.add_parsed_code(instr.line, "if (%s) goto l%s;", cond.c_str(), target.c_str());
                }
                else
                {
                    // blbc: print if (!({cond})) goto l{target}
                    func.add_parsed_code(instr.line, "if (!(%s)) goto l%s;", cond.c_str(), target.c_str());
                }
            }
            else if (instr.op == "param")
            {
                auto param = parse_operand(instr.operands[0]);
                parameters.push(param);
                // add a dummy statement for possible label
                func.add_parsed_code(instr.line, ";");
            }
            else if (instr.op == "call")
            {
                auto target = parse_operand(instr.operands[0]);
                auto line = stoi(target);

                auto called_function = get_function_at_line(line);

                // pop the number of arguments from parameters stack
                vector<string> used_parameters;
                for (int i = 0; i < called_function.parameters_count; i++)
                {
                    used_parameters.push_back(parameters.top());
                    parameters.pop();
                }
                reverse(used_parameters.begin(), used_parameters.end());
                func.add_parsed_code(instr.line, "%s(%s);", called_function.name().c_str(), string_join(used_parameters, ", ").c_str());
            }
            else if (instr.op == "load")
            {
                auto first = parse_operand(instr.operands[0]);
                func.add_parsed_code(instr.line, "LL r%d = *((LL *)%s);", instr.line, first.c_str());
            }
            else if (instr.op == "store")
            {
                auto first = parse_operand(instr.operands[0]);
                auto second = parse_operand(instr.operands[1]);

                // print *((LL *){second}) = {first};
                func.add_parsed_code(instr.line, "*((LL *)%s) = %s;", second.c_str(), first.c_str());
            }
            else if (instr.op == "move")
            {
                // the second should be a ({virtual register})
                auto first = parse_operand(instr.operands[0]);
                auto second = parse_operand(instr.operands[1]);

                func.add_parsed_code(instr.line, "%s = %s;", second.c_str(), first.c_str());
            }
            else if (instr.op == "write")
            {
                auto first = parse_operand(instr.operands[0]);
                func.add_parsed_code(instr.line, "WriteLong(%s);", first.c_str());
            }
            else if (instr.op == "wrl")
            {
                func.add_parsed_code(instr.line, "WriteLine();");
            }
            else if (instr.op == "ret")
            {
                curr_function->add_parsed_code(instr.line, "return;");
            }
            else
            {
                throw string_format("Unrecognizable op: %s", instr.op.c_str());
            }
        }
    }

    // print stage
    // 1. print headers

    printf("#include <stdio.h>\n");
    printf("#define LL long long\n");
    fprintf(stderr, "#define LL long long\n");
    printf("#define WriteLong(x) printf(\" %%lld\", (long)x)\n");
    printf("#define WriteLine() printf(\"\\n\")\n");
    printf("\n");

    // 2. print global variables

    // iterate from the min addr
    // determine the length of each var
    int curr_length = 1;
    for (int addr = max_global_variables - 1; addr >= min_global_var_addr; addr--)
    {
        if (global_variables[addr] == nullptr)
        {
            curr_length++;
        }
        else
        {
            // an variable is reached
            // current length is the length of the variable
            if (curr_length == 1)
            {
                printf("LL %s;\n", global_variables[addr]->c_str());
            }
            else
            {
                printf("LL %s[%d];\n", global_variables[addr]->c_str(), curr_length);
            }
            curr_length = 1;
        }
    }
    printf("\n");

    // 3. print functions
    for (auto &func : functions)
    {
        string signature = func.signature();

        // print the signature to stderr as header file
        fprintf(stderr, "%s;\n", signature.c_str());

        // print the signature to stdout
        printf("%s {\n", signature.c_str());

        // print the local variables
        // iterate from the min addr
        // determine the length of each var
        int curr_length = 1;
        for (int i = 0; i < func.local_variables.size(); i++)
        {
            string *local_variable = func.local_variables[i];
            if (local_variable == nullptr)
            {
                curr_length++;
            }
            else
            {
                // an variable is reached
                // current length is the length of the variable
                if (curr_length == 1)
                {
                    print_indentation();
                    printf("LL %s;\n", local_variable->c_str());
                }
                else
                {
                    print_indentation();
                    printf("LL %s[%d];\n", local_variable->c_str(), curr_length);
                }
                curr_length = 1;
            }
        }

        printf("\n");

        // print the parsed lines
        for (ParsedLine &line : func.parsed_codes)
        {
            // check if the line needs label
            if (func.labeled_lines.find(line.line) != func.labeled_lines.end())
            {
                printf("l%d:;\n", line.line);
            }
            // print the code itself
            print_indentation();
            printf("%s\n", line.code.c_str());
        }

        // print the end bracket
        printf("}\n\n");
    }
}