#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/input/Position.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardLayout.hpp"

namespace antwika::life
{

    /**
     * @brief The error a dump document this build cannot read raises.
     */
    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    /**
     * @brief What every dump of this application says it is.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-life-state-dump";

    /**
     * @brief The dump revision this build writes.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief The running simulation, as the console's dump_state takes
     * it.
     *
     * **The board plus the drag around it.** The Board is every cell's
     * alive state, and the rest is what the meaning of the next pointer
     * event depends on: whether a drag is under way, which cells it has
     * already toggled, and where it last was.
     * Coming back to a dump therefore means coming back to the instant
     * it was taken, mid-drag and all.
     *
     * The console's own history rides in the envelope rather than in
     * here, since carrying the console is every application's dump
     * behaviour and written once -- see console::SnapshotFormat.
     *
     * What it deliberately does not carry: the engine's tick number,
     * which a recording forbids going backwards, and the cells the
     * current tick has staged, which the next engine.tick clears.
     */
    struct StateDump
    {
        /** @brief Every cell's alive state, row-major. */
        Board board;

        /** @brief Whether the board was being drawn on. */
        bool dragging = false;

        /** @brief The cells the drag under way had already toggled. */
        std::vector<CellCoordinate> visited;

        /** @brief Where the drag last was, in canvas pixels. */
        std::optional<antwika::input::Position> lastDrag = std::nullopt;

        /**
         * @brief Compare two dumps.
         * @param other The dump to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const StateDump &other) const = default;
    };

    /**
     * @brief Build the chain that brings an old dump document up.
     *
     * Empty: version 1 is the first shape this application has ever
     * written, so there is nothing to bring up yet.
     *
     * @return The chain.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as the envelope's opaque state object.
     *
     * Pure: no filesystem, no clock. The cells are one string of '0'
     * and '1' characters, row-major, exactly width * height long.
     *
     * @param dump The state to encode.
     * @return The state object, magic-free: the envelope stamps the
     * document -- see console::SnapshotFormat.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    /**
     * @brief Decode the envelope's state object, validating it first.
     * @param state The state object a snapshot carried.
     * @return The decoded state.
     * @throws StateDumpError If the object is not a state this build
     * can read: the wrong shape, a cell string of the wrong length or
     * with a character that is not '0' or '1', or a visited cell off
     * the board it came with.
     */
    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::life
