#pragma once

#include <cstddef>

#include "antwika/animation/Progress.hpp"

namespace antwika::animation
{

    struct Frame final
    {
        std::size_t index{0};

        Progress progress{};

        bool finished{false};

        [[nodiscard]] bool operator==(
            const Frame &other) const noexcept = default;
    };

}
