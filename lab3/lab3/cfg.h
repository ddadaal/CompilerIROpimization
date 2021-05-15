#ifndef CFG_H
#define CFG_H

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include "function.h"

using namespace std;

struct CFG
{
    // line number
    vector<vector<int>> bbs;
    vector<int> leaders;

    // the target of an edge is -1 means it leads to the end block
    map<int, vector<int>> edges;

    void add_edge(int from, int to);

    void print();

    unordered_set<int> predecessors_of_nth_block(int n);
    unordered_set<int> successors_of_nth_block(int n);


};

extern unordered_map<int, CFG> cfg_map;

void parse_cfgs();

#endif