#pragma once

#include <string>

#include "antwika/holdem/HandValue.hpp"

namespace antwika::holdem
{

    [[nodiscard]] std::string describe(HandValue value);

}
