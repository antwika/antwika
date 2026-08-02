#pragma once

#include <cstdint>

namespace antwika::tower_defence
{

    /**
     * @brief Every string the score bar shows, as symbolic ids.
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
         * @brief Which level of the campaign, `{0}` of `{1}`.
         */
        Level,

        /**
         * @brief Which wave of the level, `{0}` of `{1}`.
         */
        Wave,

        /**
         * @brief Leaks the player can still afford, `{0}`.
         */
        Lives,

        /**
         * @brief The running score, `{0}`.
         */
        Score,

        /**
         * @brief The best score of any earlier run, `{0}`.
         */
        Best,

        /**
         * @brief The last wave of the last level is dead.
         */
        Cleared,

        /**
         * @brief The lives ran out.
         */
        Overrun,

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

} // namespace antwika::tower_defence
