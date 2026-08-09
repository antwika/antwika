#pragma once

namespace antwika::game
{

    struct Path final
    {
        [[nodiscard]] bool operator==(const Path &other) const = default;
    };

}
