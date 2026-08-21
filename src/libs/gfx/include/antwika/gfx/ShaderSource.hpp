#pragma once

#include <string>

namespace antwika::gfx
{

    struct ShaderSource final
    {
        std::string vertex;

        std::string fragment;

        [[nodiscard]] bool isComplete() const;

        [[nodiscard]] bool operator==(
            const ShaderSource &other) const = default;
    };

}
