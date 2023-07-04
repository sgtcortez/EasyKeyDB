#include "database.hpp"
#include <bits/stdint-uintn.h>
#include <vector>

using namespace std;
using namespace easykey;

void Database::write(const string key, const vector<uint8_t> value)
{
    content[key] = value;
}

vector<uint8_t> Database::read(const string key)
{
    const auto result = content.find(key);
    if (result == content.end())
    {
        return {};
    }
    return result->second;
}