#include "dse.h"
#include "function.h"
#include "utils.h"
#include <algorithm>
#include <assert.h>
#include "cfg.h"

// require a parse_function call prior to this call
// returns the count of assignments which have been eliminated
// call this method iteratively until it returns 0
int dead_code_elimination(Function &func)
{

    int start_line = func.start_line;

    // assert the cfg of the func has been created
    assert(exists(cfg_map, start_line));

    CFG &cfg = cfg_map[start_line];

    // variables to be removed
    // including local variables and line vars
    vector<string> scalar_local_vars = func.get_scalar_local_variables();
    unordered_set<string> variables_to_eliminate(scalar_local_vars.begin(), scalar_local_vars.end());

    // scan the instructions for line vars
    for (Instruction &instr : func.instructions)
    {
        if (!op_should_not_create_def(instr.op))
        {
            variables_to_eliminate.insert(string_format("r%d", instr.line));
        }
    }

    // 1. calculate def and use for each block
    vector<unordered_set<string>> defs, uses;

    for (vector<int> &bb : cfg.bbs)
    {
        defs.emplace_back();
        uses.emplace_back();

        unordered_set<string> &def = defs.back(), &use = uses.back();

        for (int line : bb)
        {
            Instruction &instr = func.get_instruction_at_line(line);

            if (instr.op == "move")
            {
                // if the first operand is local var and it's not yet defined, add it to use
                string first = parse_operand(instr.operands[0], func);
                if (exists(variables_to_eliminate, first) && !exists(def, first))
                {
                    use.insert(first);
                }

                // if the second operand of move is local var,
                // add it to def
                string var = parse_operand(instr.operands[1], func);
                if (exists(variables_to_eliminate, var))
                {
                    def.insert(var);
                }
            }
            else
            {
                // for all other op, if operand is local variable and not yet defined, add to use
                for (string operand : instr.operands)
                {
                    string parsed = parse_operand(operand, func);
                    if (exists(variables_to_eliminate, parsed) && !exists(def, parsed))
                    {
                        use.insert(parsed);
                    }
                }
            }
        }
    }

    // 2. do iterative method

    int bb_count = cfg.bbs.size();

    // the value of in and out sets are actual line numbers
    vector<unordered_set<string>> in(bb_count), out(bb_count);

    unordered_set<int> changed;

    for (int i = 0; i < bb_count; i++)
    {
        changed.insert(i);
    }

    while (!changed.empty())
    {
        auto n = *changed.begin();
        changed.erase(n);

        out[n].clear();
        for (int p : cfg.successors_of_nth_block(n))
        {
            out[n].insert(in[p].begin(), in[p].end());
        }

        unordered_set<string> prev_in_n = in[n];

        in[n].clear();
        in[n].insert(uses[n].begin(), uses[n].end());

        set_minus(out[n], defs[n], in[n]);

        if (prev_in_n != in[n])
        {
            for (int s : cfg.predecessors_of_nth_block(n))
            {
                changed.insert(s);
            }
        }
    }

    int elimination_count = 0;

    // 3. eliminate unused variables' assignment(move)
    for (int i = 0; i < bb_count; i++)
    {
        vector<int> &bb = cfg.bbs[i];

        // for each block, update the use set backwards,
        unordered_set<string> use = out[i];

        for (auto iter = bb.rbegin(); iter != bb.rend(); iter++)
        {
            int line = *iter;

            Instruction &instr = func.get_instruction_at_line(line);

            if (instr.op == "move")
            {
                // if the first operand is local var, add it to use
                string first = parse_operand(instr.operands[0], func);
                if (exists(variables_to_eliminate, first))
                {
                    use.insert(first);
                }

                // if the second operand of move is local var,
                // it's a def, remove it if is not in use
                string var = parse_operand(instr.operands[1], func);
                if (exists(variables_to_eliminate, var))
                {
                    if (!exists(use, var))
                    {
                        instr.op = "nop";
                        instr.operands.clear();
                        elimination_count++;
                    }
                    else
                    {
                        // it's in use, remove the use
                        use.erase(var);
                    }
                }
            }
            else
            {
                // for all other op, if operand is local variable, add to use
                for (string operand : instr.operands)
                {
                    string parsed = parse_operand(operand, func);
                    if (exists(variables_to_eliminate, parsed))
                    {
                        use.insert(parsed);
                    }
                }

                // for statements that generates a line var, remove it if it is not use
                if (!op_should_not_create_def(instr.op))
                {
                    string var = string_format("r%d", instr.line);
                    if (exists(variables_to_eliminate, var) && !exists(use, var))
                    {
                        instr.op = "nop";
                        instr.operands.clear();
                        elimination_count++;
                    }
                }
            }
        }
    }

    return elimination_count;
}
