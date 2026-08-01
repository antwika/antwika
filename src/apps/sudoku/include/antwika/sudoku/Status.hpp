#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/i18n/MessageId.hpp>

namespace antwika::sudoku
{

    /**
     * @brief What the session last had to say to whoever is playing.
     *
     * **An id rather than a sentence**, exactly as
     * atlas_editor::StatusMessage and ui_demo::DemoMessage are: this is
     * part of the state a replay regenerates from the recorded input,
     * and a translated string kept here would put the active language
     * inside the thing a replay reproduces.
     * SudokuScene is therefore the only place in this application that
     * has heard of a language.
     */
    enum class Status : std::uint8_t
    {
        /** @brief Nothing has happened yet; how to play. */
        Playing = 0,

        /** @brief The solver finished the grid. */
        Solved,

        /** @brief Every square is filled and every rule holds. */
        Complete,

        /** @brief No solution exists from where the grid is. */
        Unsolvable,

        /** @brief The solver ran out of the steps it was allowed. */
        LimitExceeded,

        /** @brief That square is one of the puzzle's clues. */
        GivenLocked,
    };

    /**
     * @brief How many things this session can say.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kStatusCount =
        static_cast<std::size_t>(Status::GivenLocked) + 1;

    /**
     * @brief Get which message words one status.
     *
     * An id rather than the words, following
     * ui_demo::showcaseNameId(): the picture is worded by whoever holds
     * the translator, and this header holds no language at all.
     *
     * @param status The status to word.
     * @return Its message id, or the first status's for a value that is
     * not one of the enumerators -- the same fall-back-to-the-first rule
     * atlas_editor::toolNameId() follows.
     */
    [[nodiscard]] constexpr antwika::i18n::MessageId statusNameId(
        const Status status) noexcept
    {
        constexpr std::array<antwika::i18n::MessageId, kStatusCount>
            ids{
                antwika::i18n::MessageId::SudokuHint,
                antwika::i18n::MessageId::SudokuSolved,
                antwika::i18n::MessageId::SudokuComplete,
                antwika::i18n::MessageId::SudokuNoSolution,
                antwika::i18n::MessageId::SudokuLimitExceeded,
                antwika::i18n::MessageId::SudokuGivenLocked};

        return ids[static_cast<std::size_t>(status) % kStatusCount];
    }

} // namespace antwika::sudoku
