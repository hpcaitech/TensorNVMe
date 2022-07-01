#pragma once

#include <vector>
#include <utility>

using std::vector;
using ull = unsigned long long;
// (offset, bytes) pair
using SpaceInfo = std::pair<ull, ull>;

class SpaceManager
{
private:
    ull limit, used_bytes;
    vector<SpaceInfo> avail_spaces;

public:
    SpaceManager(ull limit);
    ~SpaceManager();
    ull alloc(ull bytes);
    void free(ull offset, ull bytes);
    void print();
};