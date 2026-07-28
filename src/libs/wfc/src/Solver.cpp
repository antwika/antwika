#include "antwika/wfc/Solver.hpp"

#include <cstdint>
#include <numeric>
#include <optional>
#include <span>

#include "antwika/wfc/WfcError.hpp"
#include "EntropyIndex.hpp"
#include "Trail.hpp"

namespace antwika::wfc
{

    namespace
    {
        std::vector<std::size_t> extractAssignment(
            const std::vector<Domain> &wave)
        {
            std::vector<std::size_t> assignment;
            assignment.reserve(wave.size());
            for (const Domain &domain : wave)
            {
                assignment.push_back(domain.singleValue());
            }
            return assignment;
        }
    } // namespace

    Solver::Solver(
        std::vector<Domain> initialWave,
        std::vector<std::reference_wrapper<const IConstraint>> constraints,
        std::vector<double> valueWeights,
        SolverLimits limits)
        : initialWave(std::move(initialWave)),
          constraints(std::move(constraints)),
          valueWeights(std::move(valueWeights)),
          limits(limits)
    {
        if (!this->initialWave.empty())
        {
            const std::size_t alphabetSize =
                this->initialWave.front().alphabetSize();
            for (const Domain &domain : this->initialWave)
            {
                if (domain.alphabetSize() != alphabetSize)
                {
                    throw WfcError(
                        "Domain alphabet size mismatch in initial wave");
                }
            }
        }

        cellToConstraints.assign(this->initialWave.size(), {});
        for (std::size_t i = 0; i < this->constraints.size(); ++i)
        {
            for (const std::size_t cell : this->constraints[i].get().cells())
            {
                if (cell >= this->initialWave.size())
                {
                    throw WfcError(
                        "Constraint references an out-of-range cell "
                        "index");
                }
                cellToConstraints[cell].push_back(i);
            }
        }
    }

    SolveResult Solver::solve() const
    {
        std::vector<Domain> wave = initialWave;
        detail::Trail trail;
        detail::EntropyIndex entropyIndex(wave, valueWeights);
        std::uint64_t steps = 0;

        auto restrictAndRecord = [&](std::size_t cell, std::size_t value)
        {
            Domain &domain = wave[cell];
            const std::vector<std::size_t> before(
                domain.begin(), domain.end());
            domain.restrictTo(value);
            for (const std::size_t v : before)
            {
                if (v != value)
                {
                    trail.record(cell, v);
                }
            }
            entropyIndex.update(cell, domain);
        };

        auto propagate =
            [&](const std::vector<std::size_t> &startingWorklist) -> bool
        {
            std::vector<std::size_t> worklist(
                startingWorklist.begin(), startingWorklist.end());
            std::vector<bool> queued(constraints.size(), false);
            for (const std::size_t index : worklist)
            {
                queued[index] = true;
            }

            std::size_t head = 0;
            while (head < worklist.size())
            {
                const std::size_t constraintIndex = worklist[head];
                ++head;
                queued[constraintIndex] = false;

                const IConstraint &constraint =
                    constraints[constraintIndex].get();
                const std::span<const std::size_t> cells =
                    constraint.cells();

                std::vector<Domain> before;
                before.reserve(cells.size());
                for (const std::size_t cell : cells)
                {
                    before.push_back(wave[cell]);
                }

                if (!constraint.prune(wave))
                {
                    return false;
                }

                for (std::size_t i = 0; i < cells.size(); ++i)
                {
                    const std::size_t cell = cells[i];
                    const Domain &prior = before[i];
                    const Domain &now = wave[cell];
                    if (now.count() >= prior.count())
                    {
                        continue;
                    }

                    for (const std::size_t v : prior)
                    {
                        if (!now.contains(v))
                        {
                            trail.record(cell, v);
                        }
                    }
                    entropyIndex.update(cell, now);

                    for (const std::size_t other :
                         cellToConstraints[cell])
                    {
                        if (!queued[other])
                        {
                            queued[other] = true;
                            worklist.push_back(other);
                        }
                    }
                }
            }
            return true;
        };

        std::vector<std::size_t> allConstraints(constraints.size());
        std::iota(allConstraints.begin(), allConstraints.end(), 0);

        if (!propagate(allConstraints))
        {
            return {SolveOutcome::Unsatisfiable, {}};
        }

        struct ChoicePoint
        {
            std::size_t cell;
            std::vector<std::size_t> candidates;
            std::size_t nextIndex = 0;
            std::size_t checkpoint;
        };
        std::vector<ChoicePoint> stack;

        auto pushChoicePoint = [&](std::size_t cell)
        {
            std::vector<std::size_t> candidates(
                wave[cell].begin(), wave[cell].end());
            stack.push_back(ChoicePoint{
                cell, std::move(candidates), 0, trail.checkpoint()});
        };

        // Deliberately restructured versus PLAN_WFC.md 3.9's literal
        // pseudocode: that version re-derives "which cell" via
        // entropyIndex.pickNext() at the very top of every loop
        // iteration, including right after a failed propagate(). A
        // failed propagate() can leave some *other* cell's domain
        // empty (a contradiction) without that cell ever appearing in
        // entropyIndex (which only tracks cells with count() > 1) --
        // so re-deriving "solved" purely from "pickNext() returns
        // nothing" at that point would misreport a contradiction as
        // Solved. Instead, a failed propagate() here falls straight
        // through to retry the *same* choice point's next candidate,
        // and pickNext() is only ever consulted right after a
        // *successful* propagate() (including the very first one) --
        // exactly when "no undetermined cell left" truly does mean
        // solved.
        {
            const std::optional<std::size_t> firstCell =
                entropyIndex.pickNext();
            if (!firstCell.has_value())
            {
                return {SolveOutcome::Solved, extractAssignment(wave)};
            }
            pushChoicePoint(*firstCell);
        }

        while (true)
        {
            ChoicePoint &top = stack.back();
            if (top.nextIndex >= top.candidates.size())
            {
                stack.pop_back();
                if (stack.empty())
                {
                    return {SolveOutcome::Unsatisfiable, {}};
                }
                trail.rewindTo(
                    stack.back().checkpoint, wave, entropyIndex);
                continue;
            }

            if (limits.maxSteps.has_value() && steps >= *limits.maxSteps)
            {
                return {SolveOutcome::LimitExceeded, {}};
            }
            ++steps;

            trail.rewindTo(top.checkpoint, wave, entropyIndex);
            const std::size_t cell = top.cell;
            const std::size_t value = top.candidates[top.nextIndex];
            ++top.nextIndex;

            restrictAndRecord(cell, value);

            if (!propagate(cellToConstraints[cell]))
            {
                continue;
            }

            const std::optional<std::size_t> nextCell =
                entropyIndex.pickNext();
            if (!nextCell.has_value())
            {
                return {SolveOutcome::Solved, extractAssignment(wave)};
            }
            pushChoicePoint(*nextCell);
        }
    }

} // namespace antwika::wfc
