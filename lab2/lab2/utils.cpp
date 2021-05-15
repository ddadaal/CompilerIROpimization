#include <string>
#include <vector>
#include <stdarg.h>
#include "utils.h"

using namespace std;

char string_format_buff[200];

const char *no_def_ops[] = {
    "enter", "nop", "param", "wrl", "write", "entrypc", "store",
    "br", "blbc", "blbs", "enter", "ret", "call"};

bool op_should_not_create_def(string &op)
{
    for (const char *o : no_def_ops)
    {
        if (op == o)
        {
            return true;
        }
    }
    return false;
}

string string_format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(string_format_buff, sizeof(string_format_buff), format, args);
    return string_format_buff;
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

bool ends_with(string &value, const char *ending)
{
    string ending_str = ending;
    if (ending_str.size() > value.size())
    {
        return false;
    }
    return equal(ending_str.rbegin(), ending_str.rend(), value.rbegin());
}

bool starts_with(string &value, const char *starting)
{
    return value.rfind(starting, 0) == 0;
}

void string_split(string &str, const char *delimiter, vector<string> &result)
{
    size_t last = 0;
    size_t next = 0;
    while ((next = str.find(delimiter, last)) != string::npos)
    {
        result.push_back(str.substr(last, next - last));
        last = next + 1;
    }
    result.push_back(str.substr(last));
}

string join_int_vector(vector<int> vec, const char *delimiter)
{
    string result;
    for (auto iter = vec.begin(); iter != vec.end(); iter++)
    {
        result += to_string(*iter);
        if (iter != vec.end() - 1)
        {
            result += delimiter;
        }
    }
    return result;
}

bool is_numeric(const string &str)
{
    if (str.size() == 0)
    {
        return false;
    }

    auto iter = str.begin();
    // if the first is -, ++iter
    if (*(iter) == '-')
    {
        iter++;
    }

    // iterate the string
    for (; iter != str.end(); iter++)
    {
        if (!isdigit(*iter))
        {
            return false;
        }
    }

    return true;
}
