#include "antwika/map_editor/Generate.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/ConstraintRefs.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/SolverLimits.hpp>

#include "antwika/map_editor/Commands.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::geometry::GridCell;
        using antwika::tilemap::TerrainClass;
        using antwika::wfc::AdjacencyConstraint;
        using antwika::wfc::CompatibilityTable;
        using antwika::wfc::Domain;
        using antwika::wfc::SolveOutcome;
        using antwika::wfc::Solver;

        constexpr std::size_t kAlphabet =
            enums::kCount<TerrainClass>;

        constexpr std::size_t kStair =
            static_cast<std::size_t>(TerrainClass::Stair);

        constexpr std::uint64_t kMaxSteps = 200000;

        constexpr std::uint32_t kSeedEvery = 6;

        constexpr std::uint32_t kGenerateFailedTicks = 180;

        [[nodiscard]] CompatibilityTable makeTable(
            const GenerationRules &rules)
        {
            CompatibilityTable table(kAlphabet);

            for (std::size_t a = 0; a < kAlphabet; ++a)
            {
                for (std::size_t b = 0; b < kAlphabet; ++b)
                {
                    if (!rules.allowed[a][b])
                    {
                        table.set(a, b, false);
                    }
                }
            }

            return table;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::uint32_t next(std::uint32_t &rng) noexcept
        {
            rng ^= rng << 13U;
            rng ^= rng >> 17U;
            rng ^= rng << 5U;
            return rng;
        }

        /**
         * @brief Spreads a counter seed into an xorshift32 state.
         *
         * Ensures: the result is never zero, and every distinct
         *          seed below the zero remap's single collision
         *          yields a distinct state, so consecutive seeds
         *          never share a stream.
         */
        [[nodiscard]] std::uint32_t scrambled(
            const std::uint32_t seed) noexcept
        {
            auto value = seed + 0x9E3779B9U;

            value ^= value >> 16U;
            value *= 0x21F0AAADU;
            value ^= value >> 15U;
            value *= 0x735A2D97U;
            value ^= value >> 15U;

            return value == 0 ? 1U : value;
        }

        [[nodiscard]] std::size_t weightedPick(
            const Domain &domain,
            const GenerationRules &rules,
            std::uint32_t &rng)
        {
            double total = 0.0;

            for (const auto value : domain)
            {
                total += rules.weights[value];
            }

            auto roll = static_cast<double>(next(rng) % 1024U)
                        / 1024.0 * total;

            for (const auto value : domain) // GCOVR_EXCL_LINE
            {
                roll -= rules.weights[value];

                if (roll <= 0.0) // GCOVR_EXCL_LINE
                {
                    return value;
                }
            }

            return domain.singleValue(); // GCOVR_EXCL_LINE
        }

        struct PinnedField final
        {
            std::vector<std::optional<std::size_t>> fixed{};
            std::vector<bool> pinnedVoid{};
        };

        [[nodiscard]] PinnedField pinnedValues(
            const EditorState &state)
        {
            const auto columns = state.map.columns();
            const auto rows = state.map.rows();
            const auto cells =
                static_cast<std::size_t>(columns) * rows;

            PinnedField field;
            field.fixed.assign(cells, std::nullopt);
            field.pinnedVoid.assign(cells, false);

            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns;
                     ++column)
                {
                    const auto cell =
                        GridCell{.column = column, .row = row};
                    const auto index = pinIndex(state.map, cell);

                    if (!state.pinned[index])
                    {
                        continue;
                    }

                    const auto *slab = state.map.at(cell).slabAt(
                        state.activeLevel);

                    if (slab == nullptr)
                    {
                        field.pinnedVoid[index] = true;
                        continue;
                    }

                    field.fixed[index] =
                        static_cast<std::size_t>(slab->terrain);
                }
            }

            return field;
        } // GCOVR_EXCL_LINE

        void dropIncompatible(
            Domain &domain,
            const std::optional<std::size_t> neighbour,
            const CompatibilityTable &table)
        {
            if (!neighbour.has_value())
            {
                return;
            }

            for (std::size_t value = 0; value < kAlphabet; ++value)
            {
                if (domain.contains(value)
                    && !table.compatible(value, *neighbour))
                {
                    domain.remove(value);
                }
            }
        }

        [[nodiscard]] std::vector<Domain> makeWave(
            const EditorState &state,
            const CompatibilityTable &table,
            const bool seeded,
            const std::uint32_t seed,
            std::vector<std::optional<std::size_t>> fixed)
        {
            const auto columns = state.map.columns();
            const auto rows = state.map.rows();
            auto rng = scrambled(seed);

            std::vector<Domain> wave;
            wave.reserve(
                static_cast<std::size_t>(columns) * rows);

            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns;
                     ++column)
                {
                    const auto index =
                        static_cast<std::size_t>(row) * columns
                        + column;

                    if (fixed[index].has_value())
                    {
                        wave.push_back(Domain::singleton(
                            *fixed[index], kAlphabet));
                        continue;
                    }

                    Domain domain(kAlphabet);

                    domain.remove(kStair);

                    if (seeded && next(rng) % kSeedEvery == 0)
                    {
                        Domain safe = domain;

                        if (column > 0)
                        {
                            dropIncompatible(
                                safe, fixed[index - 1], table);
                        }

                        if (column + 1 < columns)
                        {
                            dropIncompatible(
                                safe, fixed[index + 1], table);
                        }

                        if (row > 0)
                        {
                            dropIncompatible(
                                safe, fixed[index - columns], table);
                        }

                        if (row + 1 < rows)
                        {
                            dropIncompatible(
                                safe, fixed[index + columns], table);
                        }

                        if (!safe.isEmpty())
                        {
                            const auto value = weightedPick(
                                safe, state.rules, rng);

                            domain.restrictTo(value);
                            fixed[index] = value;
                        }
                    }

                    wave.push_back(std::move(domain));
                }
            }

            return wave;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<AdjacencyConstraint>
        makeConstraints(
            const EditorState &state,
            const CompatibilityTable &table,
            const std::vector<bool> &pinnedVoid)
        {
            const auto columns = state.map.columns();
            const auto rows = state.map.rows();

            std::vector<AdjacencyConstraint> constraints;

            for (std::uint32_t row = 0; row < rows; ++row)
            {
                for (std::uint32_t column = 0; column < columns;
                     ++column)
                {
                    const auto at =
                        static_cast<std::size_t>(row) * columns
                        + column;

                    if (pinnedVoid[at])
                    {
                        continue;
                    }

                    if (column + 1 < columns
                        && !pinnedVoid[at + 1])
                    {
                        constraints.emplace_back(at, at + 1, table);
                    }

                    if (row + 1 < rows
                        && !pinnedVoid[at + columns])
                    {
                        constraints.emplace_back(
                            at, at + columns, table);
                    }
                }
            }

            return constraints;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::optional<std::vector<TerrainClass>> solve(
            const EditorState &state,
            const bool seeded,
            const std::uint32_t seed)
        {
            const auto table = makeTable(state.rules);
            auto field = pinnedValues(state);
            const auto constraints =
                makeConstraints(state, table, field.pinnedVoid);
            const auto refs = antwika::wfc::referencesTo(constraints);

            const Solver solver(
                makeWave(
                    state,
                    table,
                    seeded,
                    seed,
                    std::move(field.fixed)),
                refs,
                {state.rules.weights.begin(),
                 state.rules.weights.end()},
                {.maxSteps = kMaxSteps});

            const auto result = solver.solve();

            if (result.outcome != SolveOutcome::Solved)
            {
                return std::nullopt;
            }

            std::vector<TerrainClass> terrains;
            terrains.reserve(result.assignment.size());

            for (const auto value : result.assignment)
            {
                terrains.push_back(static_cast<TerrainClass>(
                    value % kAlphabet));
            }

            return terrains;
        }
    }

    std::optional<std::vector<TerrainClass>> generateTerrains(
        const EditorState &state, const std::uint32_t seed)
    {
        if (const auto seededResult = solve(state, true, seed))
        {
            return seededResult;
        }

        return solve(state, false, seed);
    }

    void generate(EditorState &state, log::ILogger &logger)
    {
        reconcilePins(state);

        const auto seed = state.generateSeed++;
        const auto terrains = generateTerrains(state, seed);

        if (!terrains.has_value())
        {
            logger.log(
                log::Level::Warning,
                "generate found no solution for seed "
                    + std::to_string(seed));
            state.generateFailedTicks = kGenerateFailedTicks;
            return;
        }

        applyGenerated(state, *terrains);
        logger.log(
            log::Level::Info,
            "generated with seed " + std::to_string(seed));
    }

}
