#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <string_view>

namespace antwika::app
{

    int runCatchingErrors(
        std::string_view name,
        const std::function<void()> &body,
        std::ostream &errors = std::cerr);

}
