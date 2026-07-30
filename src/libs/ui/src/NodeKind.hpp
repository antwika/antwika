#pragma once

#include <cstdint>

namespace antwika::ui::detail
{

    /**
     * @brief What one entry in the layout arena is.
     *
     * Two kinds and no more.
     * A panel is a container with a background, a button is a container
     * with a background and a text child, and a spacer is a container
     * with no children at all, so every widget the library offers is one
     * of these two arranged differently.
     */
    enum class NodeKind : std::uint8_t
    {
        Container = 0,
        Text,
    };

} // namespace antwika::ui::detail
