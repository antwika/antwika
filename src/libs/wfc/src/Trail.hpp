#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "EntropyIndex.hpp"

namespace antwika::wfc::detail
{

    using antwika::wfc::Domain;

    // Private undo log of individual (cellIndex, value) removals.
    //
    // Solver snapshots a constraint's domains before calling prune().
    // It diffs the result afterward and logs each value that vanished.
    // That covers both remove() and a multi-bit restrictTo().
    // rewindTo() replays logged entries after a checkpoint in reverse.
    // Each one is restored via Domain::add().
    // The affected cell's EntropyIndex entry is updated too.
    // This replaces a per-branch wave copy.
    // Undoing a failed branch only costs what that branch changed.
    class Trail
    {
    public:
        // Record that value was just removed from wave[cell].
        void record(std::size_t cell, std::size_t value);

        // Current length of the log, to later rewindTo().
        [[nodiscard]] std::size_t checkpoint() const;

        // Undo every entry recorded after checkpoint, in reverse.
        // Restores wave[cell] via Domain::add().
        // Notifies entropyIndex.update() for each affected cell.
        void rewindTo(
            std::size_t checkpoint,
            std::vector<Domain> &wave,
            EntropyIndex &entropyIndex);

    private:
        std::vector<std::pair<std::size_t, std::size_t>> entries;
    };

} // namespace antwika::wfc::detail
