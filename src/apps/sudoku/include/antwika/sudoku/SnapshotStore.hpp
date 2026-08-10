#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/console/IJsonSnapshotStore.hpp>
#include <antwika/console/SnapshotError.hpp>

#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/Status.hpp"

namespace antwika::sudoku
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-sudoku-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    [[nodiscard]] std::string_view statusName(Status status) noexcept;

    [[nodiscard]] std::optional<Status> statusFromName(
        std::string_view name) noexcept;

    [[nodiscard]] nlohmann::json puzzleStateToJson(
        const PuzzleState &state);

    void puzzleStateFromJson(
        const nlohmann::json &state, PuzzleState &into);

    class SudokuSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<
              antwika::console::SnapshotError>
    {
    public:
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

}
