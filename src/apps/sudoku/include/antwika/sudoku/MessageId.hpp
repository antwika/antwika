#pragma once

#include <cstdint>

namespace antwika::sudoku
{

    /**
     * @brief Every string the sudoku board shows, as symbolic ids.
     *
     * They live here rather than in antwika::i18n because a library
     * that enumerated its consumers' strings would be a library naming
     * its consumers.
     * What keeps that safe is that the list of every id there is, both
     * catalogues and the completeness check over them are in this
     * module too: see the MessageSet concept in
     * <antwika/i18n/MessageSet.hpp> and the suite MessagesTest.cpp
     * instantiates.
     *
     * A MessageId is never persisted, so its numbering is free and
     * adding, reordering or removing one needs no migration.
     */
    enum class MessageId : std::uint16_t
    {
        /**
         * @brief The name over the grid.
         */
        Title,

        /**
         * @brief The button that finishes the grid.
         */
        SolveButton,

        /**
         * @brief How to play, shown until something happens.
         */
        Hint,

        /**
         * @brief The solver finished the grid.
         */
        Solved,

        /**
         * @brief Every square is filled and every rule holds.
         */
        Complete,

        /**
         * @brief No solution exists from where the grid is.
         */
        NoSolution,

        /**
         * @brief The solver gave up before deciding.
         */
        LimitExceeded,

        /**
         * @brief That square is one of the puzzle's clues.
         */
        GivenLocked,

        /**
         * @brief How many ids there are; not an id itself.
         *
         * Messages.cpp static_asserts its name table against this,
         * which is what makes an enumerator nobody listed a build
         * failure rather than a string that is silently in no
         * catalogue.
         */
        Count,
    };

} // namespace antwika::sudoku
