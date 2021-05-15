#include "ssa.h"
#include <assert.h>
#include "utils.h"
#include "cfg.h"
#include <algorithm>
#include <stack>
#include "peephole.h"

typedef long long LL;

// https://www.ed.tus.ac.jp/j-mune/keio/m/ssa2.pdf

const int ENTRY_NODE = -1;

struct DomTreeNode
{
    // -1 means ENTRY
    int root;
    DomTreeNode *parent = nullptr;
    vector<DomTreeNode *> children;

    DomTreeNode(int root) : root(root) {}
};

typedef vector<DomTreeNode *> DomTree;
typedef vector<unordered_set<int>> DF;

void build_dom_tree(vector<unordered_set<int>> &dom, DomTree &nodes)
{
    // the 0 to n range
    vector<int> range;
    for (int i = 0; i < dom.size(); i++)
    {
        range.push_back(i);
    }

    // use the definition to determine the idom for each node
    for (int i = 0; i < dom.size(); i++)
    {
        for (int d : dom[i])
        {
            // d idom i <=> d != i && no p that d dom p and p dom i
            if (d != i && find_if(range.begin(), range.end(), [&](int &p) {
                              return d != p && p != i && exists(dom[p], d) && exists(dom[i], p);
                          }) == range.end())
            {
                // set the idom
                nodes[i]->parent = nodes[d];
                break;
            }
        }
    }

    // populate the children of nodes
    for (int i = 0; i < nodes.size(); i++)
    {
        if (nodes[i]->parent != nullptr)
        {
            nodes[i]->parent->children.push_back(nodes[i]);
        }
    }
}

void compute_df(int n, CFG &cfg, vector<unordered_set<int>> &dom, DomTree &dom_tree, DF &df)
{
    unordered_set<int> s;
    for (int y : cfg.successors_of_nth_block(n))
    {
        if (dom_tree[y]->parent != dom_tree[n])
        {
            s.insert(y);
        }
    }
    for (DomTreeNode *c : dom_tree[n]->children)
    {
        compute_df(c->root, cfg, dom, dom_tree, df);
        for (int w : df[c->root])
        {
            if (!exists(dom[w], n))
            {
                s.insert(w);
            }
        }
    }
    df[n] = s;
}

void rename(int n,
            unordered_map<string, int> &count,
            unordered_map<string, stack<int>> &var_stack,
            CFG &cfg,
            Function &func,
            DomTree &dom_tree)
{
    unordered_set<string> pushed_vars;

    for (int line : cfg.bbs[n])
    {
        Instruction &s = func.get_instruction_at_line(line);
        if (s.op != "phi")
        {
            for (int op_index = 0; op_index < s.operands.size(); op_index++)
            {
                if (s.op == "move" && op_index == 1)
                {
                    continue;
                }
                string x = parse_operand(s.operands[op_index], func, true);
                if (exists(var_stack, x))
                {
                    int i = var_stack[x].top();
                    s.operands[op_index] = string_format("%s$%d", x.c_str(), i);
                }
            }
        }
        if (s.op == "move")
        {
            string a = parse_operand(s.operands[1], func, true);
            if (exists(var_stack, a))
            {
                count[a]++;
                int i = count[a];
                var_stack[a].push(i);
                pushed_vars.insert(a);
                s.operands[1] = string_format("%s$%d", a.c_str(), i);
            }
        }
    }

    for (int y : cfg.successors_of_nth_block(n))
    {
        // n is the jth predecessor of y
        unordered_set<int> predecessors = cfg.predecessors_of_nth_block(y);
        vector<int> precessors_vector = set_to_vector(predecessors);
        int j = find(precessors_vector.begin(), precessors_vector.end(), n) - precessors_vector.begin();

        for (int line : cfg.bbs[y])
        {
            Instruction &instr = func.get_instruction_at_line(line);
            if (instr.op == "phi")
            {
                string a = parse_operand(instr.operands[j], func, true);

                int i = var_stack[a].top();
                instr.operands[j] = string_format("%s$%d", a.c_str(), i);
            }
        }
    }

    for (DomTreeNode *child : dom_tree[n]->children)
    {
        rename(child->root, count, var_stack, cfg, func, dom_tree);
    }

    for (auto &a : pushed_vars)
    {
        var_stack[a].pop();
    }
}

void convert_to_ssa(Function &func)
{
    if (func.in_ssa_form)
    {
        return;
    }

    int start_line = func.start_line;

    // assert the cfg of the func has been created
    assert(exists(cfg_map, start_line));

    CFG &cfg = cfg_map[start_line];

    func.in_ssa_form = true;

    // 1. calculate dominance relation
    int bb_count = cfg.bbs.size();

    // the value of in and out sets are actual line numbers
    vector<unordered_set<int>> dom(bb_count);

    // initial all doms to all nodes
    for (auto &d : dom)
    {
        for (int i = 0; i < bb_count; i++)
        {
            d.insert(i);
        }
    }

    unordered_set<int> changed;
    changed.insert(0);

    while (!changed.empty())
    {
        auto n = *changed.begin();
        changed.erase(n);

        unordered_set<int> prev_dom = dom[n];
        dom[n].clear();
        dom[n].insert(n);

        // the intersection of all predecessors
        vector<unordered_set<int> *> all_pred_doms;
        for (int s : cfg.predecessors_of_nth_block(n))
        {
            all_pred_doms.push_back(&dom[s]);
        }
        vector<int> intersection_result = intersect(all_pred_doms);
        dom[n].insert(intersection_result.begin(), intersection_result.end());

        if (prev_dom != dom[n])
        {
            for (int s : cfg.successors_of_nth_block(n))
            {
                changed.insert(s);
            }
        }
    }

    // 2. build dom tree
    DomTree dom_tree;
    for (int i = 0; i < bb_count; i++)
    {
        dom_tree.push_back(new DomTreeNode(i));
    }

    build_dom_tree(dom, dom_tree);

    // 3. compute dominance frontier
    DF df(bb_count);
    compute_df(0, cfg, dom, dom_tree, df);

    // 4. insert phi functions
    // consider only scalar local vars

    vector<string> local_vars = func.get_scalar_local_variables();

    // Aorig[n] is the vars defined in bb n
    // defsites[a] are the bbs that defines var
    vector<unordered_set<string>> A_orig(bb_count);
    unordered_map<string, unordered_set<int>> defsites, A_phi;
    for (int n = 0; n < bb_count; n++)
    {
        for (int line : cfg.bbs[n])
        {
            Instruction &instr = func.get_instruction_at_line(line);
            if (instr.op == "move")
            {
                string var = parse_operand(instr.operands[1], func);
                if (exists(local_vars, var))
                {
                    A_orig[n].insert(var);
                    defsites[var].insert(n);
                }
            }
        }
    }

    for (string a : local_vars)
    {
        unordered_set<int> W = defsites[a];
        while (!W.empty())
        {
            int n = *W.begin();
            W.erase(n);

            for (int y : df[n])
            {
                if (!exists(A_phi[a], y))
                {
                    // insert the statemenet
                    int leader = cfg.leaders[y];
                    vector<string> phi_operands;
                    for (int pred : cfg.predecessors_of_nth_block(y))
                    {
                        phi_operands.push_back(string_format("%s$?", a.c_str()));
                    }
                    int phi_line = func.insert_instruction_before_line("phi", phi_operands, leader);

                    // insert the move
                    vector<string> move_operands;
                    move_operands.push_back(string_format("(%d)", phi_line));
                    move_operands.push_back(string_format("%s$?", a.c_str()));

                    int move_line = func.insert_instruction_before_line("move", move_operands, leader);

                    // update the cfg
                    cfg.leaders[y] = phi_line;
                    cfg.bbs[y].insert(cfg.bbs[y].begin(), move_line);
                    cfg.bbs[y].insert(cfg.bbs[y].begin(), phi_line);

                    // create a new edge cfg
                    map<int, vector<int>> new_edges;
                    for (auto &pair : cfg.edges)
                    {
                        // new values
                        vector<int> values = pair.second;
                        for (int i = 0; i < values.size(); i++)
                        {
                            if (values[i] == leader)
                            {
                                values[i] = phi_line;
                            }
                        }

                        new_edges[pair.first == leader ? phi_line : pair.first] = values;
                    }
                    cfg.edges = new_edges;

                    A_phi[a].insert(y);
                    if (!exists(A_orig[y], a))
                    {
                        W.insert(y);
                    }
                }
            }
        }
    }
    // 5. rename variables

    // initialization
    unordered_map<string, int> count;
    unordered_map<string, stack<int>> var_stack;

    for (string var : local_vars)
    {
        count[var] = 0;
        var_stack[var].push(0);
    }

    rename(0, count, var_stack, cfg, func, dom_tree);
}

void convert_all_to_ssa()
{
    for (Function &func : functions)
    {
        convert_to_ssa(func);
    }
}

bool is_ssa_var(string &str)
{
    return str.find("$") != str.npos;
}

int ssa_constant_propagation(Function &func)
{
    assert(func.in_ssa_form);
    int start_line = func.start_line;
    assert(exists(cfg_map, start_line));
    CFG &cfg = cfg_map[start_line];

    int constants_propagated = 0;

    // 1. record the constant
    unordered_map<string, LL> constants;

    for (auto it = func.instructions.begin(); it < func.instructions.end(); it++)
    {
        Instruction &i = *it;
        if (i.op == "move" && is_ssa_var(i.operands[1]))
        {
            LL constant;
            if (is_constant(i.operands[0], constants, constant))
            {
                constants[i.operands[1]] = constant;
            }
        }
        else if (i.op == "phi")
        {
            // if all of the operands are the same constant,
            // the target var is also constant

            assert(i.operands.size() > 0);
            LL constant;
            bool all_same_constant = is_constant(i.operands[0], constants, constant);

            for (int j = 1; j < i.operands.size(); j++)
            {
                LL new_constant;
                all_same_constant = all_same_constant && is_constant(i.operands[j], constants, new_constant);
                all_same_constant = all_same_constant && (constant == new_constant);
                if (!all_same_constant)
                {
                    break;
                }
            }
            if (!all_same_constant)
            {
                continue;
            }

            // it is a constant
            assert((it + 1)->op == "move");
            constants[(it + 1)->operands[1]] = constant;

            // clear the two ops
            it->op = "nop";
            it->operands.clear();

            it++;
            it->op = "nop";
            it->operands.clear();
        }
    }

    // 2. change the constant vars to the constant
    for (Instruction &i : func.instructions)
    {
        for (int j = 0; j < i.operands.size(); j++)
        {
            if (i.op == "move" && j == 1)
                continue;

            if (exists(constants, i.operands[j]))
            {
                i.operands[j] = to_string(constants[i.operands[j]]);
                constants_propagated++;
            }
        }
    }

    return constants_propagated;
}

void convert_back_to_normal(Function &func)
{
    if (!func.in_ssa_form)
    {
        return;
    }

    int start_line = func.start_line;
    assert(exists(cfg_map, start_line));
    CFG &cfg = cfg_map[start_line];
    int bb_count = cfg.bbs.size();

    // find all phi functions, and add move value to the respective predecessor
    for (int i = 0; i < bb_count; i++)
    {
        auto preds = cfg.predecessors_of_nth_block(i);
        auto preds_vector = set_to_vector(preds);
        for (auto it = 0; it < cfg.bbs[i].size() - 1; it++)
        {
            int line = cfg.bbs[i][it];
            Instruction &instr = func.get_instruction_at_line(line);
            if (instr.op == "phi")
            {
                Instruction phi_instr = instr;
                Instruction &next_instr_ref = func.get_instruction_at_line(cfg.bbs[i][it + 1]);
                Instruction next_instr = next_instr_ref;

                // change the instrs to nop first
                instr.op = "nop";
                instr.operands.clear();
                next_instr_ref.op = "nop";
                next_instr_ref.operands.clear();

                string var = parse_operand(next_instr.operands[1], func, true);

                string var_with_offset = string_format("%s#%d", var.c_str(), func.symbol_table[var]);

                for (int pred_i = 0; pred_i < phi_instr.operands.size(); pred_i++)
                {
                    // the pred_i th operand should be inserted
                    // at the back of the pred_i th predecessor of the block

                    int pred = preds_vector[pred_i];
                    int last_line = cfg.bbs[pred].back();

                    int new_line = func.insert_instruction_after_line("move", {phi_instr.operands[pred_i], var_with_offset}, last_line);

                    // add the newly created line to bb
                    cfg.bbs[i].push_back(new_line);
                }

                // skip the move
                it++;
            }
        }
    }

    // modify the operands to normal form
    for (int i = 0; i < bb_count; i++)
    {
        for (int line : cfg.bbs[i])
        {
            Instruction &instr = func.get_instruction_at_line(line);
            for (int op_i = 0; op_i < instr.operands.size(); op_i++)
            {
                if (is_ssa_var(instr.operands[op_i]))
                {
                    string var = parse_operand(instr.operands[op_i], func, true);

                    string var_with_offset = string_format("%s#%d", var.c_str(), func.symbol_table[var]);

                    instr.operands[op_i] = var_with_offset;
                }
            }
        }
    }

    func.in_ssa_form = false;
}

void convert_all_back_to_normal()
{
    for (Function &func : functions)
    {
        convert_back_to_normal(func);
        peephole_opt(func);
    }
}