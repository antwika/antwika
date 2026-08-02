#pragma once

#include <cstdint>

namespace antwika::task_worker
{

    /**
     * @brief Every string the worker pool shows, as symbolic ids.
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
         * @brief Which tick the picture is of, `{0}`.
         */
        Tick,

        /**
         * @brief How many jobs this tick may start, `{0}`.
         */
        Budget,

        /**
         * @brief How many jobs this tick did start, `{0}`.
         */
        Started,

        /**
         * @brief The heading over the pool.
         */
        Workers,

        /**
         * @brief Worker `{0}` is holding nothing.
         */
        WorkerIdle,

        /**
         * @brief Worker `{0}` is holding task `{1}`.
         */
        WorkerBusy,

        /**
         * @brief `{0}` of a task's `{1}` ticks are left.
         */
        TicksLeft,

        /**
         * @brief The heading over the pending queue.
         */
        Queue,

        /**
         * @brief Pending task `{0}` at priority `{1}`.
         */
        Queued,

        /**
         * @brief Pending task `{0}` cannot run until task `{1}` has.
         */
        Blocked,

        /**
         * @brief The heading over the finished tasks.
         */
        Completed,

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

} // namespace antwika::task_worker
