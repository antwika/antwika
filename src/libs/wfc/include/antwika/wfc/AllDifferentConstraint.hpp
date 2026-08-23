#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"

namespace antwika::wfc
{

    class AllDifferentConstraint final : public IConstraint
    {
    public:
        explicit AllDifferentConstraint(
            std::vector<std::size_t> cellIndices);

        [[nodiscard]] std::span<const std::size_t> getCells() const override;
        bool prune(std::vector<Domain> &waveDomains) const override;

    private:
        std::vector<std::size_t> cellIndices;
    };

}
