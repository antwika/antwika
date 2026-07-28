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
    // Solver snapshots a constraint's cells() domains before calling
    // prune(), diffs after, and pushes one entry per value that
    // disappeared, regardless of whether it came from remove() or
    // restrictTo() clearing several bits at once. rewindTo() replays
    // entries after a checkpoint in reverse, restoring each one via
    // Domain::add() and notifying an EntropyIndex so it stays
    // consistent -- this is what replaces a per-branch wave copy:
    // undoing a failed branch costs only what that branch changed.
    class Trail
    {
    public:
        // Record that value was just removed from wave[cell].
        void record(std::size_t cell, std::size_t value);

        // Current length of the log, to later rewindTo().
        [[nodiscard]] std::size_t checkpoint() const;

        // Undo every entry recorded after checkpoint, in reverse order,
        // restoring wave[cell] via Domain::add() and notifying
        // entropyIndex.update() for each affected cell.
        void rewindTo(
            std::size_t checkpoint,
            std::vector<Domain> &wave,
            EntropyIndex &entropyIndex);

    private:
        std::vector<std::pair<std::size_t, std::size_t>> entries;
    };

} // namespace antwika::wfc::detail
