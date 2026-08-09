#pragma once

#include <compare>
#include <string>

#include "antwika/network/Port.hpp"

namespace antwika::network
{

    struct Endpoint final
    {
        std::string host;

        Port port{};

        [[nodiscard]] bool operator==(const Endpoint &other) const
            = default;

        [[nodiscard]] std::strong_ordering operator<=>(
            const Endpoint &other) const = default;
    };

}
