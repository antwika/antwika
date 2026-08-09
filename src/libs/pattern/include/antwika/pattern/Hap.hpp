#pragma once

#include <optional>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    struct Hap final
    {
        std::optional<Span> whole;

        Span part;

        Controls value;

        [[nodiscard]] bool hasOnset() const noexcept;

        [[nodiscard]] bool operator==(const Hap &other) const = default;
    };

}
