#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <assert.h>
#include <algorithm>
#include <functional>
#include "utils.h"
#include "function.h"
#include "cfg.h"
#include "scp.h"
#include "peephole.h"
#include "dse.h"

using namespace std;

// scp, dse, wcp
vector<string> opts;
// c, cfg, 3addr, rep
string backend;

int repeatedly_run_opt(function<int(Function &)> opt, unordered_map<int, int> &all_results)
{
    int total = 0;
    unordered_map<int, int> results;
    unordered_set<int> changed;

    // initial changed map
    for (Function &func : functions)
    {
        changed.insert(func.start_line);
    }

    // iterative eliminate functions until all functions eliminates no statement
    while (!changed.empty())
    {
        int start_line = *changed.begin();
        changed.erase(start_line);

        // find the functions with start_line
        Function *func;
        for (Function &f : functions)
        {
            if (f.start_line == start_line)
            {
                func = &f;
            }
        }
        assert(func != nullptr);

        int result = opt(*func);

        results[start_line] += result;
        all_results[start_line] += result;
        total += result;

        if (result > 0)
        {
            changed.insert(start_line);
        }
    }

    return total;
}

void report_results(const char *report_template, unordered_map<int, int> &result, bool report)
{
    if (!report)
    {
        return;
    }
    for (Function &func : functions)
    {
        printf("Function: %d\n", func.start_line);
        printf(report_template, result[func.start_line]);
    }
}

void parse_arguments(int argc, char **argv)
{
    // parse arguments
    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];

        if (starts_with(arg, "-opt"))
        {
            // get the value and split by ,
            string value = arg.substr(5);
            string_split(value, ",", opts);
        }
        else if (starts_with(arg, "-backend"))
        {
            backend = arg.substr(9);
        }
    }
}

bool exists_opt(const char *opt)
{
    string opt_str(opt);
    return exists(opts, opt_str);
}

int main(int argc, char **argv)
{
    parse_arguments(argc, argv);

    create_functions();

    parse_cfgs();

    for (Function &func : functions)
    {
        peephole_opt(func);
    }

    parse_functions();

    // run optimizations
    bool report = backend == "rep";

    unordered_map<int, int> scp_result, dse_result;
    while (true)
    {
        int scp = exists_opt("scp") ? repeatedly_run_opt(simple_constant_propagation, scp_result) : 0;
        int dse = exists_opt("dse") ? repeatedly_run_opt(dead_code_elimination, dse_result) : 0;

        if (scp == 0 && dse == 0)
        {
            break;
        }
    }
    if (report)
    {
        report_results("Number of constants propagated: %d\n", scp_result, exists_opt("scp"));
        report_results("Number of statements eliminated: %d\n", dse_result, exists_opt("dse"));
    }

    parse_functions();

    if (backend == "cfg")
    {
        for (auto &func : functions)
        {
            printf("Function: %d\n", func.start_line);
            cfg_map[func.start_line].print();
        }
    }
    else if (backend == "3addr")
    {
        for (auto &func : functions)
        {
            func.print_3addr();
            printf("\n");
        }
    }
    else if (backend == "c")
    {
        print_headers();

        print_global_variables();

        for (auto &func : functions)
        {
            func.print_c();
        }
    }
}