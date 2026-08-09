#include "antwika/pattern/Span.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/PatternError.hpp"

namespace antwika::pattern
{

    namespace
    {
        [[nodiscard]] std::string describe(const Cycle &cycle)
        {
            return std::to_string(cycle.numerator()) + "/"
                + std::to_string(cycle.denominator());
        }
    }

    Span::Span(Cycle begin, Cycle end) : from(begin), to(end)
    {
        if (to <= from)
        {
            throw PatternError(
                "antwika::pattern: a span from " + describe(from) + " to "
                + describe(to) + " holds no time at all");
        }
    }

    const Cycle &Span::begin() const noexcept
    {
        return from;
    }

    const Cycle &Span::end() const noexcept
    {
        return to;
    }

    Cycle Span::length() const
    {
        return to - from;
    }

    std::optional<Span> Span::intersect(const Span &other) const
    {
        const auto begin = std::max(from, other.from);
        const auto end = std::min(to, other.to);

        if (end <= begin)
        {
            return std::nullopt;
        }

        return Span(begin, end);
    }

    std::vector<Span> Span::spanCycles() const
    {
        std::vector<Span> pieces;

        auto begin = from;

        while (begin < to)
        {
            const auto boundary = begin.nextSam();
            const auto end = std::min(boundary, to);

            pieces.emplace_back(begin, end);

            begin = end;
        }

        return pieces;

    } // GCOVR_EXCL_LINE

}
