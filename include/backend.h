#include "asyncio.h"
#include <string>
#include <unordered_set>

std::unordered_set<std::string> get_backends();

bool probe_backend(const std::string &backend);

AsyncIO *create_asyncio(unsigned int n_entries, const std::string &backend);