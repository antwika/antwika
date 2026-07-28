#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "antwika/wfc/CompatibilityTable.hpp"
#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    /**
     * @brief Binary arc-consistency between two cells via a
     * CompatibilityTable -- the classic WFC tile-adjacency rule.
     *
     * prune() removes any value from left's domain with no compatible
     * value left in right's domain, and symmetrically for right (AC-3's
     * arc revision).
     */
    class AdjacencyConstraint final : public IConstraint
    {
    public:
        /**
         * @brief Construct the constraint between two cell indices.
         * @param left The left-hand cell index.
         * @param right The right-hand cell index.
         * @param table Symbol-pair compatibility between left and
         * right.
         */
        AdjacencyConstraint(
            std::size_t left, std::size_t right, CompatibilityTable table);

        [[nodiscard]] std::span<const std::size_t> cells() const override;
        bool prune(std::vector<Domain> &wave) const override;

    private:
        std::array<std::size_t, 2> cellIndices;
        CompatibilityTable table;
    };

} // namespace antwika::wfc
