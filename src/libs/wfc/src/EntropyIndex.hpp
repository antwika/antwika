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

    // Private, incrementally maintained lowest-entropy-cell picker.
    //
    // Keyed on (sortKey, cellIndex) in a std::set, ordered ascending.
    // pickNext() is then a single begin() lookup.
    // update() is a single erase+insert.
    // Neither rescans the whole wave.
    // Ordering by the pair gives ties-by-lowest-index for free.
    // It comes from std::pair's lexicographic operator<.
    // No separate tie-break step is needed.
    // Only cells with count() > 1 are kept in the set.
    class EntropyIndex final
    {
    public:
        EntropyIndex(
            const std::vector<Domain> &wave,
            std::vector<double> valueWeights);

        // Notify the index that a cell's domain changed.
        // This covers a propagation shrink or a Trail-rewind regrow.
        void update(std::size_t cell, const Domain &domain);

        // Next cell to collapse, per the entropy + lowest-index rule.
        // Returns std::nullopt if every cell is singleton or empty.
        [[nodiscard]] std::optional<std::size_t> pickNext() const;

    private:
        std::vector<double> valueWeights;
        std::set<std::pair<double, std::size_t>> keysByEntropy;
        std::vector<std::optional<std::pair<double, std::size_t>>>
            cellKey;

        // Orders cells the way entropy would.
        // It does not inherit the last ULP of this build's libm.
        // Uniform weights key on the candidate count itself.
        // Weighted ones key on the entropy in whole 1e-9 steps.
        [[nodiscard]] double sortKey(const Domain &domain) const;
    };

} // namespace antwika::wfc::detail
