#include "asyncio.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <cstdlib>
#include <iostream>

std::unordered_set<std::string> get_backends();

bool probe_backend(const std::string &backend);

std::string get_default_backend();

bool get_debug_flag();

AsyncIO *create_asyncio(unsigned int n_entries, std::string backend);
