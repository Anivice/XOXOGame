#include "log.hpp"

#include <vector>

static const std::vector<int> containers = {1, 2, 3, 4, 5, 6, 7, 8};

int main()
{
    debug::log(debug::error_log, containers, "\n");
}
