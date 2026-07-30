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
     * the C++ call stack.
     */
    class Solver final
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
         * cell; empty means uniform (plain MRV). Every weight must be
         * strictly positive.
         * @param limits Optional step budget; default is unlimited.
         * @throws WfcError if initialWave's domains don't all share the
         * same alphabet size, a constraint references a cell index out
         * of range for initialWave, valueWeights is non-empty and its
         * size doesn't match the wave's alphabet size, or any weight is
         * not strictly positive.
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
         * @throws WfcError if a wave about to be reported Solved still
         * holds an undetermined cell, which would be a bug here rather
         * than anything a caller can provoke.
         */
        [[nodiscard]] SolveResult solve() const;

    private:
        std::vector<Domain> initialWave;
        std::vector<std::reference_wrapper<const IConstraint>> constraints;
        std::vector<double> valueWeights;
        SolverLimits limits;

        // cellToConstraints[c] lists every constraint touching cell c.
        // Built once at construction; wakes only touched constraints.
        std::vector<std::vector<std::size_t>> cellToConstraints;
    };

} // namespace antwika::wfc
