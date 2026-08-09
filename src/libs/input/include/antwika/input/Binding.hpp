#pragma once

#include <variant>

#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"

namespace antwika::input
{

    using Binding = std::variant<Key, MouseButton>;

}
