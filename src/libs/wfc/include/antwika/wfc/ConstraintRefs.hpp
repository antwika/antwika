#pragma once

#include <functional>
#include <ranges>
#include <vector>

#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    template <std::ranges::input_range Constraints>
    [[nodiscard]] std::vector<std::reference_wrapper<const IConstraint>>
    referencesTo(const Constraints &constraints)
    {
        return std::vector<std::reference_wrapper<const IConstraint>>(
            std::ranges::begin(constraints), std::ranges::end(constraints));
    }

}
