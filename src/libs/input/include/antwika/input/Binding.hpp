#pragma once

#include <variant>

#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"

namespace antwika::input
{

    /**
     * @brief An input an action can be bound to.
     *
     * A key or a pointer button, and deliberately nothing else. Scroll
     * notches and pointer movement are amounts rather than things that are
     * either down or not, so an action bound to one could answer isActive()
     * only by inventing a threshold.
     */
    using Binding = std::variant<Key, MouseButton>;

} // namespace antwika::input
