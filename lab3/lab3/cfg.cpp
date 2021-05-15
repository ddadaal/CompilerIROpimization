#include <vector>
#include <unordered_map>
#include <algorithm>
#include <set>
#include "function.h"
#include "cfg.h"
#include "utils.h"

using namespace std;

unordered_map<int, CFG> cfg_map;

void parse_cfgs()
{
    for (Function &func : functions)
    {
        CFG cfg;

        // find leaders
        unordered_set<int> leader_set;

        leader_set.insert(func.start_line);

        for (auto iter = func.instructions.begin(); iter != func.instructions.end(); iter++)
        {
            Instruction &instr = *iter;
            if (instr.op == "blbc" || instr.op == "blbs" || instr.op == "br" || instr.op == "call")
            {
                auto target = parse_operand(instr.operands[starts_with(instr.op, "bl") ? 1 : 0], func);
                auto target_line = stoi(target);

                bool is_call_other_function = instr.op == "call" && (stoi(parse_operand(instr.operands[0], func)) != func.start_line);

                // if it's not func call to other function, the target is leader
                if (!is_call_other_function)
                {
                    leader_set.insert(target_line);
                }
                if ((++iter) != func.instructions.end())
                {
                    leader_set.insert(iter->line);
                }
            }
        }

        // insert the elements in leader_set into the cfg
        cfg.leaders.insert(cfg.leaders.begin(), leader_set.begin(), leader_set.end());
        sort(cfg.leaders.begin(), cfg.leaders.end());

        // create basic blocks
        set<int> worklist(cfg.leaders.begin(), cfg.leaders.end());
        while (!worklist.empty())
        {
            int x = *worklist.begin();
            worklist.erase(x);

            vector<int> bb;
            bb.push_back(x);

            for (int i = x + 1; i <= func.instructions.back().line && !exists(cfg.leaders, i); i++)
            {
                bb.push_back(i);
            }

            cfg.bbs.push_back(bb);
        }

        // create edges
        for (auto bb_iter = cfg.bbs.begin(); bb_iter != cfg.bbs.end(); bb_iter++)
        {
            int leader = bb_iter->at(0);
            int last = bb_iter->at(bb_iter->size() - 1);
            Instruction &x = func.get_instruction_at_line(last);

            bool is_recursive_call = x.op == "call" && (stoi(parse_operand(x.operands[0], func)) == func.start_line);

            if (x.op == "blbc" || x.op == "blbs" || x.op == "br")
            {
                auto target = parse_operand(x.operands[starts_with(x.op, "bl") ? 1 : 0], func);
                cfg.add_edge(leader, stoi(target));
            }
            // if it's not unconditional branch
            if (x.op != "br")
            {
                // connect the bb with the next bb
                // if it is the last block, set the target edge to -1
                cfg.add_edge(leader, bb_iter == cfg.bbs.end() - 1 ? -1 : (bb_iter + 1)->at(0));
            }
        }

        // sort the edges
        for (auto &key : cfg.edges)
        {
            sort(key.second.begin(), key.second.end());
        }

        // set the map
        cfg_map.insert(make_pair(func.start_line, cfg));
    }
}

void CFG::add_edge(int from, int to)
{
    if (edges.find(from) == edges.end())
    {
        edges[from] = vector<int>();
    }
    // keep the vector sorted

    edges[from].push_back(to);
}

void CFG::print()
{
    // transform the leaders int to string
    printf("Basic blocks: %s\n", join_int_vector(leaders, " ").c_str());
    printf("CFG:\n");

    for (auto edge : edges)
    {
        // don't print the -1 edge
        string list;

        for (auto target : edge.second)
        {
            if (target == -1)
            {
                continue;
            }
            list += " ";
            list += to_string(target);
        }

        printf("%d ->%s\n", edge.first, list.c_str());
    }
}

// return the index of the predecessor blocks
unordered_set<int> CFG::predecessors_of_nth_block(int n)
{
    int leader = leaders[n];
    unordered_set<int> result;
    for (int i = 0; i < bbs.size(); i++)
    {
        int bb_leader = leaders[i];
        if (exists(edges[bb_leader], leader))
        {
            result.insert(i);
        }
    }
    return result;
}

// return the index of the successor blocks
unordered_set<int> CFG::successors_of_nth_block(int n)
{
    int leader = leaders[n];
    unordered_set<int> result;

    for (int edge_target : edges[leader])
    {
        // find the index of each leader
        for (int i = 0; i < leaders.size(); i++)
        {
            if (leaders[i] == edge_target)
            {
                result.insert(i);
                break;
            }
        }
    }

    return result;
}