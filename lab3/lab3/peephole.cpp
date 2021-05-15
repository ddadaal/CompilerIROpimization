#include "peephole.h"
#include "utils.h"

// optimize:
// instr 11: add {var} GP/FP
// instr 12: store {data} (11) // load (11)
// to
// instr 11: nop
// instr 12: move {data} {var} // assign {var}

// instruction assign: assign the var to the r{line} var
void peephole_opt(Function &func)
{
    for (auto iter = func.instructions.begin(); iter != func.instructions.end() - 1; iter++)
    {
        auto next_iter = iter + 1;

        // match the pattern
        if (iter->op == "add" && (iter->operands[1] == "GP" || iter->operands[1] == "FP"))
        {
            if (next_iter->op == "store")
            {
                string second_operand = next_iter->operands[1];
                if (second_operand == string_format("(%d)", iter->line))
                {
                    // add and store pattern, change to nop and store directly to data
                    next_iter->op = "move";
                    next_iter->operands[1] = iter->operands[0];
                    iter->op = "nop";
                    iter->operands.clear();
                }
            }
            else if (next_iter->op == "load")
            {
                if (next_iter->operands[0] == string_format("(%d)", iter->line))
                {
                    // add and load pattern ,chang to nop and assign
                    next_iter->op = "assign";
                    next_iter->operands[0] = iter->operands[0];
                    iter->op = "nop";
                    iter->operands.clear();
                }
            }
        }
    }

    // optimize move a to a
    // happens in ssa
    for (Instruction &instr : func.instructions)
    {
        if (instr.op == "move" && instr.operands[0] == instr.operands[1])
        {
            instr.op = "nop";
            instr.operands.clear();
        }
    }
}