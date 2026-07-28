#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/wfc/Domain.hpp"

namespace antwika::wfc
{

    /**
     * @brief Anything that can look at, and narrow, the domains of the
     * cells it cares about.
     *
     * prune() doubles as the solver's only consistency check: collapsing
     * a cell and re-running prune() on every constraint touching it both
     * propagates the consequence and detects a violation. prune() itself
     * stays trail-unaware -- Solver alone knows about undo (see Trail).
     */
    class IConstraint
    {
    public:
        virtual ~IConstraint() = default;

        /**
         * @brief The cell indices this constraint reads and prunes.
         * @return A span of indices into the wave this constraint acts
         * on.
         */
        [[nodiscard]] virtual std::span<const std::size_t> cells() const = 0;

        /**
         * @brief Prune wave[cells()] in place.
         * @param wave The full wave; only cells() indices are touched.
         * @return False iff a domain became empty (a contradiction);
         * true otherwise, including "no change."
         */
        virtual bool prune(std::vector<Domain> &wave) const = 0;
    };

} // namespace antwika::wfc
