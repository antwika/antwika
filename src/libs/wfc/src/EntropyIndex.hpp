#pragma once

#include <cstddef>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "antwika/wfc/Domain.hpp"

namespace antwika::wfc::detail
{

    using antwika::wfc::Domain;

    class EntropyIndex final
    {
    public:
        EntropyIndex(
            const std::vector<Domain> &waveDomains,
            std::vector<double> valueWeights);

        void update(std::size_t cell, const Domain &domain);

        [[nodiscard]] std::optional<std::size_t> getPickNext() const;

    private:
        std::vector<double> valueWeights;
        std::set<std::pair<double, std::size_t>> keysByEntropy;
        std::vector<std::optional<std::pair<double, std::size_t>>>
            cellKey;

        [[nodiscard]] double getSortKey(const Domain &domain) const;
    };

}
