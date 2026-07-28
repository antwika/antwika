#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "antwika/wfc/Domain.hpp"
#include "antwika/wfc/IConstraint.hpp"
#include "antwika/wfc/SolveResult.hpp"
#include "antwika/wfc/SolverLimits.hpp"

namespace antwika::wfc
{

    /**
     * @brief Deterministic, complete Wave Function Collapse solver.
     *
     * Repeatedly picks the lowest-entropy cell (EntropyIndex), collapses
     * it to one candidate value, propagates the consequences via a
     * worklist over the supplied constraints, and backtracks on
     * contradiction using an explicit choice-point stack and a Trail
     * undo log -- never recursion, so search depth is never bounded by
     * the C++ call stack. See PLAN_WFC.md 3.9 for the full design.
     */
    class Solver
    {
    public:
        /**
         * @brief Construct a solver over a wave and its constraints.
         * @param initialWave One Domain per cell; copied once by
         * solve(), never mutated by the constructor.
         * @param constraints Constraints referencing indices into
         * initialWave. Each referenced IConstraint must outlive this
         * Solver.
         * @param valueWeights Per-symbol weight, shared across every
         * cell; empty means uniform (plain MRV), per PLAN_WFC.md 3.7.
         * @param limits Optional step budget; default is unlimited.
         * @throws WfcError if initialWave's domains don't all share the
         * same alphabet size, or a constraint references a cell index
         * out of range for initialWave.
         */
        Solver(
            std::vector<Domain> initialWave,
            std::vector<std::reference_wrapper<const IConstraint>>
                constraints,
            std::vector<double> valueWeights = {},
            SolverLimits limits = {});

        /**
         * @brief Solve the wave.
         * @return Solved with a valid assignment, Unsatisfiable if
         * every branch was exhausted with no solution, or
         * LimitExceeded if limits.maxSteps was reached first.
         */
        [[nodiscard]] SolveResult solve() const;

    private:
        std::vector<Domain> initialWave;
        std::vector<std::reference_wrapper<const IConstraint>> constraints;
        std::vector<double> valueWeights;
        SolverLimits limits;

        // cellToConstraints[c] lists every constraint whose cells()
        // includes c -- built once at construction so propagation wakes
        // only constraints actually touched by a change.
        std::vector<std::vector<std::size_t>> cellToConstraints;
    };

} // namespace antwika::wfc
