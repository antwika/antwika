#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <string_view>

namespace antwika::app
{

    int runGuarded(
        std::string_view name,
        const std::function<void()> &body,
        std::ostream &errors = std::cerr);

}
