#include "antwika/wfc/Solver.hpp"

#include <algorithm>
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
            const std::vector<Domain> &waveDomains)
        {
            std::vector<std::size_t> assignment;
            assignment.reserve(waveDomains.size());
            for (const Domain &domain : waveDomains)
            {
                assignment.push_back(domain.singleValue());
            }
            return assignment;
        } // GCOVR_EXCL_LINE
    }

    Solver::Solver(
        std::vector<Domain> initialWaveDomains,
        std::vector<std::reference_wrapper<const IConstraint>> constraints,
        std::vector<double> valueWeights,
        SolverLimits limits,
        std::vector<std::optional<std::size_t>> preferences)
        : initialWave(std::move(initialWaveDomains)),
          constraints(std::move(constraints)),
          valueWeights(std::move(valueWeights)),
          limits(limits),
          preferences(std::move(preferences))
    {
        if (!this->preferences.empty()
            && this->preferences.size() != this->initialWave.size())
        {
            throw WfcError(
                "preferences size must match the wave's cell count");
        }

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

                if (domain.isEmpty())
                {
                    throw WfcError(
                        "Initial wave holds a cell with an empty "
                        "domain");
                }
            }

            if (!this->valueWeights.empty()
                && this->valueWeights.size() != alphabetSize)
            {
                throw WfcError(
                    "valueWeights size must match the wave's alphabet "
                    "size");
            }
        }

        for (const double weight : this->valueWeights)
        {
            if (!(weight > 0.0))
            {
                throw WfcError("valueWeights must be strictly positive");
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
        std::vector<Domain> waveDomains = initialWave;
        detail::Trail trail;
        detail::EntropyIndex entropyIndex(waveDomains, valueWeights);
        std::uint64_t steps = 0;

        auto restrictAndRecord = [&](std::size_t cell, std::size_t value)
        {
            Domain &domain = waveDomains[cell];
            const std::vector<std::size_t> beforeDomains(
                domain.begin(), domain.end());
            domain.restrictTo(value);
            for (const std::size_t v : beforeDomains)
            {
                if (v != value)
                {
                    trail.record(cell, v);
                }
            }
            entropyIndex.update(cell, domain);
        };

        std::vector<bool> queuedFlags(constraints.size(), false);

        std::vector<Domain> beforeDomains;

        auto propagate =
            [&](const std::vector<std::size_t> &startingWorklist) -> bool
        {
            std::vector<std::size_t> worklist(
                startingWorklist.begin(), startingWorklist.end());
            std::fill(queuedFlags.begin(), queuedFlags.end(), false);
            for (const std::size_t index : worklist)
            {
                queuedFlags[index] = true;
            }

            std::size_t head = 0;
            while (head < worklist.size())
            {
                const std::size_t constraintIndex = worklist[head];
                ++head;
                queuedFlags[constraintIndex] = false;

                const IConstraint &constraint =
                    constraints[constraintIndex].get();
                const std::span<const std::size_t> cells =
                    constraint.cells();

                std::size_t snapshot = 0;
                for (const std::size_t cell : cells)
                {
                    if (snapshot < beforeDomains.size())
                    {
                        beforeDomains[snapshot] = waveDomains[cell];
                    }
                    else
                    {
                        beforeDomains.push_back(waveDomains[cell]);
                    }
                    ++snapshot;
                }

                const bool pruned = constraint.prune(waveDomains);

                for (std::size_t i = 0; i < cells.size(); ++i)
                {
                    const std::size_t cell = cells[i];
                    const Domain &priorDomain = beforeDomains[i];
                    const Domain &nowDomain = waveDomains[cell];
                    if (nowDomain.count() >= priorDomain.count())
                    {
                        continue;
                    }

                    for (const std::size_t v : priorDomain)
                    {
                        if (!nowDomain.contains(v))
                        {
                            trail.record(cell, v);
                        }
                    }
                    entropyIndex.update(cell, nowDomain);

                    for (const std::size_t otherCell :
                         cellToConstraints[cell])
                    {
                        if (!queuedFlags[otherCell])
                        {
                            queuedFlags[otherCell] = true;
                            worklist.push_back(otherCell);
                        }
                    }
                }

                if (!pruned)
                {
                    return false;
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

        struct ChoicePoint final
        {
            std::size_t cell;
            std::vector<std::size_t> candidates;
            std::size_t nextIndex = 0;
            std::size_t checkpoint;
        };
        std::vector<ChoicePoint> stackPoints;

        auto pushChoicePoint = [&](std::size_t cell)
        {
            std::vector<std::size_t> candidates(
                waveDomains[cell].begin(), waveDomains[cell].end());

            if (cell < preferences.size()
                && preferences[cell].has_value())
            {
                const auto likedEntry = std::find(
                    candidates.begin(),
                    candidates.end(),
                    *preferences[cell]);

                if (likedEntry != candidates.end())
                {
                    std::iter_swap(candidates.begin(), likedEntry);
                }
            }

            stackPoints.push_back(ChoicePoint{ // GCOVR_EXCL_LINE
                cell, std::move(candidates), 0, trail.checkpoint()});
        };

        {
            const std::optional<std::size_t> firstCell =
                entropyIndex.pickNext();
            if (!firstCell.has_value())
            {
                return {SolveOutcome::Solved, extractAssignment(waveDomains)};
            }
            pushChoicePoint(*firstCell);
        }

        while (true)
        {
            ChoicePoint &topPoint = stackPoints.back();
            if (topPoint.nextIndex >= topPoint.candidates.size())
            {
                stackPoints.pop_back();
                if (stackPoints.empty())
                {
                    return {SolveOutcome::Unsatisfiable, {}};
                }
                trail.rewindTo(
                    stackPoints.back().checkpoint, waveDomains, entropyIndex);
                continue;
            }

            if (limits.maxSteps.has_value() && steps >= *limits.maxSteps)
            {
                return {SolveOutcome::LimitExceeded, {}};
            }
            ++steps;

            trail.rewindTo(topPoint.checkpoint, waveDomains, entropyIndex);
            const std::size_t cell = topPoint.cell;
            const std::size_t value = topPoint.candidates[topPoint.nextIndex];
            ++topPoint.nextIndex;

            restrictAndRecord(cell, value);

            if (!propagate(cellToConstraints[cell]))
            {
                continue;
            }

            const std::optional<std::size_t> nextCell =
                entropyIndex.pickNext();
            if (!nextCell.has_value())
            {
                return {SolveOutcome::Solved, extractAssignment(waveDomains)};
            }
            pushChoicePoint(*nextCell);
        }
    }

}
