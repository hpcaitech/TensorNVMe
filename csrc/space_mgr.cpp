#include <stdexcept>
#include "space_mgr.h"

SpaceManager::SpaceManager(unsigned long long limit) : limit(limit)
{
}

SpaceManager::~SpaceManager()
{
}

ull SpaceManager::alloc(ull bytes)
{
    if (bytes == 0)
        throw std::runtime_error("Invalid alloc size (0)");
    auto target_iter = avail_spaces.end();
    ull target_bytes = 0;
    for (auto iter = avail_spaces.begin(); iter != avail_spaces.end(); iter++)
    {
        if (iter->second >= bytes && (target_iter == avail_spaces.end() || iter->second < target_bytes))
        {
            target_iter = iter;
            target_bytes = iter->second;
        }
    }
    // no available space, use new space
    if (target_iter == avail_spaces.end())
    {
        if (limit > 0 && limit - used_bytes < bytes)
            throw std::runtime_error("File size exceed limit");
        ull offset = used_bytes;
        used_bytes += bytes;
        return offset;
    }
    ull offset = target_iter->first;
    target_iter->second -= bytes;
    if (target_iter->second == 0)
        avail_spaces.erase(target_iter);
    return offset;
}

void SpaceManager::free(ull offset, ull bytes)
{
    if (bytes == 0)
        throw std::runtime_error("Invalid free size (0)");
    SpaceInfo new_avail_space(offset, bytes);
    auto front_iter = avail_spaces.end();
    auto rear_iter = avail_spaces.end();
    for (auto iter = avail_spaces.begin(); iter != avail_spaces.end(); iter++)
    {
        if (offset > iter->first && offset - iter->first == iter->second)
        {
            front_iter = iter;
            new_avail_space.first = iter->first;
            new_avail_space.second += iter->second;
        }
        else if (offset < iter->first && iter->first - offset == bytes)
        {
            rear_iter = iter;
            new_avail_space.second += iter->second;
        }
    }
    if (front_iter != avail_spaces.end())
        avail_spaces.erase(front_iter);
    if (rear_iter != avail_spaces.end())
        avail_spaces.erase(rear_iter);
    avail_spaces.push_back(new_avail_space);
}