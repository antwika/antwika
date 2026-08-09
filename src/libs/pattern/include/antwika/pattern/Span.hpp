#pragma once

#include <optional>
#include <vector>

#include "antwika/pattern/Cycle.hpp"

namespace antwika::pattern
{

    class Span final
    {
    public:
        Span(Cycle begin, Cycle end);

        [[nodiscard]] const Cycle &begin() const noexcept;

        [[nodiscard]] const Cycle &end() const noexcept;

        [[nodiscard]] Cycle length() const;

        [[nodiscard]] std::optional<Span> intersect(
            const Span &other) const;

        [[nodiscard]] std::vector<Span> spanCycles() const;

        [[nodiscard]] bool operator==(const Span &other) const noexcept
            = default;

    private:
        Cycle from;
        Cycle to;
    };

}
