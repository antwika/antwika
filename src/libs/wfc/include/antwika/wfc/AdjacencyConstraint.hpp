#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "antwika/wfc/CompatibilityTable.hpp"
#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    class AdjacencyConstraint final : public IConstraint
    {
    public:
        AdjacencyConstraint(
            std::size_t left, std::size_t right, CompatibilityTable table);

        AdjacencyConstraint(
            std::size_t left,
            std::size_t right,
            std::shared_ptr<const CompatibilityTable> table);

        [[nodiscard]] std::span<const std::size_t> cells() const override;
        bool prune(std::vector<Domain> &waveDomains) const override;

    private:
        std::array<std::size_t, 2> cellIndices;
        std::shared_ptr<const CompatibilityTable> table;
    };

}
