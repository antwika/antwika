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
    // Keyed on (entropy, cellIndex) in a std::set, ordered ascending, so
    // pickNext() is a single begin() lookup and update() is a single
    // erase+insert -- neither rescans the whole wave. Ordering by the
    // pair directly makes "ties by lowest cell index" fall out of
    // std::pair's lexicographic operator< for free, with no separate
    // tie-break step. Only cells with count() > 1 are kept in the set.
    class EntropyIndex
    {
    public:
        EntropyIndex(
            const std::vector<Domain> &wave,
            std::vector<double> valueWeights);

        // Notify the index that a cell's domain changed (shrunk during
        // propagation, or grew back during a Trail rewind).
        void update(std::size_t cell, const Domain &domain);

        // Next cell to collapse, per the entropy + lowest-index rule, or
        // std::nullopt if every cell is already singleton/empty.
        [[nodiscard]] std::optional<std::size_t> pickNext() const;

    private:
        std::vector<double> valueWeights;
        std::set<std::pair<double, std::size_t>> keysByEntropy;
        std::vector<std::optional<std::pair<double, std::size_t>>>
            cellKey;

        [[nodiscard]] double computeEntropy(const Domain &domain) const;
    };

} // namespace antwika::wfc::detail
