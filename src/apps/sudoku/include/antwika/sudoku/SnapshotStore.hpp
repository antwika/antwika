#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/console/JsonSnapshotStore.hpp>
#include <antwika/console/SnapshotError.hpp>

#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/Status.hpp"

namespace antwika::sudoku
{

    /**
     * @brief What names this application's dump file as its own.
     *
     * Checked before any state is looked at, so another application's
     * dump refuses on this rather than on some later member.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-sudoku-state-dump";

    /**
     * @brief The dump-document version this build writes.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief Get a status's stable, persisted name.
     *
     * What a dump document holds, so these are part of the format and
     * may not change once written -- the same rule
     * game::toolName() is held to.
     *
     * @param status The status to name.
     * @return Its name, e.g. "playing".
     */
    [[nodiscard]] std::string_view statusName(Status status) noexcept;

    /**
     * @brief Get the status a persisted name refers to.
     * @param name The name to look up, as statusName() made it.
     * @return The status, or nothing for a name no status goes by.
     */
    [[nodiscard]] std::optional<Status> statusFromName(
        std::string_view name) noexcept;

    /**
     * @brief Encode a session's whole state as a dump's state member.
     * @param state The session to encode.
     * @return The encoded object: both boards in Board::format()'s
     * flat form, the picked square if any, and the status by name.
     */
    [[nodiscard]] nlohmann::json puzzleStateToJson(
        const PuzzleState &state);

    /**
     * @brief Decode a dump's state member and apply it to a session.
     * @param state The object to read, validated before it is.
     * @param into The session to restore into, wholesale.
     * @throws antwika::console::SnapshotError If the object is not
     * the shape a dump writes, a board string does not parse, or the
     * status name is one this build does not know.
     */
    void puzzleStateFromJson(
        const nlohmann::json &state, PuzzleState &into);

    /**
     * @brief The console's way in and out of this application's state.
     *
     * The application half of dump_state and load_state -- see
     * console::ISnapshotStore.
     * The whole session is one PuzzleState, so this store is that and
     * the envelope, nothing more.
     *
     * puzzleStateFromJson() already refuses with the seam's error, so
     * that is what this names as its own category and the seam's
     * rewrapping is the identity.
     */
    class SudokuSnapshotStore final
        : public antwika::console::JsonSnapshotStore<
              antwika::console::SnapshotError>
    {
    public:
        /**
         * @brief Construct the store over the session it snapshots.
         * @param state The session dumped and restored. Must outlive
         * this store.
         */
        explicit SudokuSnapshotStore(PuzzleState &state) noexcept;

        SudokuSnapshotStore(const SudokuSnapshotStore &) = delete;
        SudokuSnapshotStore(SudokuSnapshotStore &&) = delete;

        SudokuSnapshotStore &operator=(const SudokuSnapshotStore &)
            = delete;
        SudokuSnapshotStore &operator=(SudokuSnapshotStore &&) = delete;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &dumped) override;

        PuzzleState &state;
    };

} // namespace antwika::sudoku
