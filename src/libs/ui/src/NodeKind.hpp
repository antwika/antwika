#pragma once

#include <cstdint>

namespace antwika::ui::detail
{

    enum class NodeKind : std::uint8_t
    {
        Container = 0,
        Text,
        Image,
    };

}
