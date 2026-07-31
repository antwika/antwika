#pragma once

#include <functional>
#include <ranges>
#include <vector>

#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    /**
     * @brief Take the reference vector Solver wants over a container of
     * constraints the caller owns.
     * @param constraints Any range of objects deriving from IConstraint,
     * which must outlive both the returned vector and the Solver built
     * from it.
     * @return One std::reference_wrapper per element, in the same order.
     *
     * Solver takes its constraints by reference because it does not own
     * them and will not copy them, and because one wave is usually
     * constrained by several unrelated concrete types.
     * The price is that a caller holding the common case -- one
     * std::vector of one concrete constraint type -- has to convert, and
     * every caller in this repo did, three of them by hand-writing a
     * reserve-and-push loop that this one line replaces.
     *
     * This adds no notion of geometry to the library, which is the point:
     * the constraints handed in already address cells by index, and this
     * neither reads those indices nor cares how they were arrived at.
     */
    template <std::ranges::input_range Constraints>
    [[nodiscard]] std::vector<std::reference_wrapper<const IConstraint>>
    referencesTo(const Constraints &constraints)
    {
        return std::vector<std::reference_wrapper<const IConstraint>>(
            std::ranges::begin(constraints), std::ranges::end(constraints));
    }

} // namespace antwika::wfc
