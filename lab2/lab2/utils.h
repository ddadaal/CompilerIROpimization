#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <unordered_set>
#include <set>
#include <unordered_map>

using namespace std;

bool starts_with(string &value, const char *starting);

bool ends_with(string &value, const char *ending);

string string_join(vector<string> &vector, const char *delimiter);

string string_format(const char *format, ...);

void string_split(string &str, const char *delimiter, vector<string> &result);

template <typename TK, typename TV>
bool exists(unordered_map<TK, TV> &set, TK &val)
{
    return set.find(val) != set.end();
}

template <typename T>
bool exists(unordered_set<T> &set, T &val)
{
    return set.find(val) != set.end();
}

template <typename T>
bool exists(set<T> &set, T &val)
{
    return set.find(val) != set.end();
}

template <typename T>
bool exists(vector<T> &set, T &val)
{
    return find(set.begin(), set.end(), val) != set.end();
}

template <typename T>
void set_minus(unordered_set<T> set1, unordered_set<T> set2, unordered_set<T> &result)
{
    for (auto v : set1)
    {
        if (!exists(set2, v))
        {
            result.insert(v);
        }
    }
}

string join_int_vector(vector<int> vec, const char *delimiter);

bool is_numeric(const string &str);

bool op_should_not_create_def(string &op);

#endif