#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include "function.h"
#include <assert.h>
#include "cfg.h"
#include "utils.h"

typedef long long LL;

using namespace std;

int simple_constant_propagation(Function &func)
{
    int start_line = func.start_line;

    // assert the cfg of the func has been created
    assert(exists(cfg_map, start_line));

    CFG &cfg = cfg_map[start_line];

    // 1. get GEN and KILL sets for each block
    // the value of the sets are actual line numbers in the 3addr file
    // globals are the line numbers that assigns to global vars,
    // which should be killed if the last instruction of the block is a call
    vector<unordered_set<int>> gens, kills, globals;

    // 2. get the related var for each instruction i (what's the x in di(x))
    // the index of di are the relative line number (line - start_line)
    // if di == "", this line doesn't create a definition
    vector<string> di;

    for (Instruction &instr : func.instructions)
    {
        string var;
        // move gens the second operand
        if (instr.op == "move")
        {
            di.push_back(instr.operands[1]);
        }
        // other instruction gens the (line)
        else if (!op_should_not_create_def(instr.op))
        {
            di.push_back(string_format("(%d)", instr.line));
        }
        else
        {
            di.push_back("");
        }
    }

    // 3. for each block, generate the gen and kill set

    // get all the defs to global vars
    unordered_set<string> global_vars = get_scalar_global_vars();
    vector<int> di_to_globals;

    for (int i = 0; i < di.size(); i++)
    {
        if (exists(global_vars, di[i]))
        {
            di_to_globals.push_back(i + start_line);
        }
    }

    for (vector<int> &bb : cfg.bbs)
    {

        gens.emplace_back();
        kills.emplace_back();
        globals.emplace_back();

        unordered_set<int> &gen = gens.back(), &kill = kills.back(), &global = globals.back();

        for (int line : bb)
        {
            int index_at_di = line - start_line;

            // if di is not null, this line generates itself
            string &x = di[index_at_di];
            if (x.size() > 0)
            {
                gen.insert(line);

                // and kills other di with the same x

                for (int i = 0; i < di.size(); i++)
                {
                    if (i == index_at_di)
                    {
                        continue;
                    }
                    string &xi = di[i];
                    if (x == xi)
                    {
                        kill.insert(i + start_line);
                    }
                }
            }
        }

        // if block n ends with function call,
        // the block will kill all global var defs
        Instruction &last_instr = func.get_instruction_at_line(bb[bb.size() - 1]);
        if (last_instr.op == "call")
        {
            global.insert(di_to_globals.begin(), di_to_globals.end());
        }
    }

    // 4. perform iterative method
    int bb_count = cfg.bbs.size();

    // the value of in and out sets are actual line numbers
    vector<unordered_set<int>> in(bb_count), out(bb_count);

    unordered_set<int> changed;

    for (int i = 0; i < bb_count; i++)
    {
        changed.insert(i);
    }

    while (!changed.empty())
    {
        auto n = *changed.begin();
        changed.erase(n);

        in[n].clear();

        for (int p : cfg.predecessors_of_nth_block(n))
        {
            in[n].insert(out[p].begin(), out[p].end());
        }

        unordered_set<int> prev_out_n = out[n];

        out[n].clear();

        // don't insert the defs to globals if global[n] contains them
        for (int i : gens[n])
        {
            if (!exists(globals[n], i))
            {
                out[n].insert(i);
            }
        }

        set_minus(in[n], kills[n], out[n]);

        if (prev_out_n != out[n])
        {
            for (int s : cfg.successors_of_nth_block(n))
            {
                changed.insert(s);
            }
        }
    }

    // 5. perform optimization

    int constant_propagated = 0;

    // for each block, check in defs
    for (int i = 0; i < bb_count; i++)
    {
        vector<int> &bb = cfg.bbs[i];
        unordered_set<int> &in_set = in[i];

        unordered_map<string, LL> constant_operands;
        unordered_set<string> not_constant_set;

        for (int in_val : in_set)
        {
            int di_index = in_val - start_line;

            string &var = di[di_index];

            if (var.size() != 0)
            {
                // if instructions[in_val] is a move/store/assign instr and the 1st operand is a constant
                // it's a constant definition
                Instruction &instr = func.get_instruction_at_line(in_val);
                LL constant;
                if ((instr.op == "move" || instr.op == "store" || instr.op == "assign") && is_numeric(instr.operands[0]))
                {
                    LL constant = stoll(instr.operands[0]);
                    // if a constant to the var already exists
                    if (exists(constant_operands, var))
                    {
                        // if the two constants is not equal, this var is not constant
                        if (constant != constant_operands[var])
                        {
                            not_constant_set.insert(var);
                            constant_operands.erase(var);
                        }
                    }
                    // if not exists and it's not impossible to be constant, set the constant
                    else if (!exists(not_constant_set, var))
                    {
                        constant_operands.insert(make_pair(var, constant));
                    }
                }
                // if the var is not constant, it's never constant
                else
                {
                    constant_operands.erase(var);
                    not_constant_set.insert(var);
                }
            }
        }

        // for each line in bb,
        // update the def information with defs in the block
        // and change the operands
        for (int line : bb)
        {
            Instruction &instr = func.get_instruction_at_line(line);

            // if it's move,
            // the first operand is constant, the var is also a constant
            // if it's not, it's not a constant
            LL constant;
            if (instr.op == "move")
            {
                string var = instr.operands[1];
                if (is_numeric(instr.operands[0]))
                {
                    constant_operands[var] = stoll(instr.operands[0]);
                }
                else
                {
                    constant_operands.erase(var);
                    not_constant_set.insert(var);
                }
            }
            else if (instr.op == "assign" && is_constant(instr.operands[0], constant_operands, constant))
            {
                constant_operands[string_format("(%d)", instr.line)] = constant;
            }

            for (int operand_index = 0; operand_index < instr.operands.size(); operand_index++)
            {
                // skip the second operand of move
                if (instr.op == "move" && operand_index == 1)
                {
                    continue;
                }
                string operand = instr.operands[operand_index];
                if (exists(constant_operands, operand))
                {
                    long long constant = constant_operands[operand];
                    instr.operands[operand_index] = to_string(constant);
                    constant_propagated++;
                }
            }
        }
    }

    return constant_propagated;
}
