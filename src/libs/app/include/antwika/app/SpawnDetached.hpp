#pragma once

#include <string>
#include <vector>

namespace antwika::app
{

    [[nodiscard]] bool isSpawnDetached(
        const std::string &program,
        const std::vector<std::string> &arguments);

}
