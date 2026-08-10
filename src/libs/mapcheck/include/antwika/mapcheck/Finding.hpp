#pragma once

#include <optional>
#include <string>

#include <antwika/geometry/Grid.hpp>

namespace antwika::mapcheck
{

    struct Finding final
    {
        std::string map{};
        std::string message{};
        std::optional<geometry::GridCell> at{};

        [[nodiscard]] bool operator==(const Finding &other) const = default;
    };

}
