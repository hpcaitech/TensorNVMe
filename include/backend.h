#include "asyncio.h"
#include <string>
#include <unordered_set>
#include <cstdlib>
#include <iostream>

std::unordered_set<std::string> get_backends();

bool probe_backend(const std::string &backend);

std::string get_default_backend();

AsyncIO *create_asyncio(unsigned int n_entries, std::string backend);