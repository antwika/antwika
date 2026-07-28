#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    /**
     * @brief Requires every covered cell to end up with a different
     * value, enforced via naked-single elimination.
     *
     * For every cell in cellIndices currently a singleton with value v,
     * removes v from every other cell in cellIndices. This is not full
     * hyper-arc-consistency; completeness comes from Solver's
     * backtracking search, not from how aggressively this prunes.
     */
    class AllDifferentConstraint final : public IConstraint
    {
    public:
        /**
         * @brief Construct the constraint over a set of cell indices.
         * @param cellIndices The cells that must all differ.
         */
        explicit AllDifferentConstraint(
            std::vector<std::size_t> cellIndices);

        [[nodiscard]] std::span<const std::size_t> cells() const override;
        bool prune(std::vector<Domain> &wave) const override;

    private:
        std::vector<std::size_t> cellIndices;
    };

} // namespace antwika::wfc
