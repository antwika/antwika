#include "antwika/task_worker/Messages.hpp"

#include <antwika/i18n/MessageTable.hpp>

#include "antwika/task_worker/MessageId.hpp"

namespace antwika::task_worker
{

    constexpr i18n::MessageTable<MessageId> kMessageTable{
        .names{{
            {MessageId::Tick, "Tick"},
            {MessageId::Budget, "Budget"},
            {MessageId::Started, "Started"},
            {MessageId::Workers, "Workers"},
            {MessageId::WorkerIdle, "WorkerIdle"},
            {MessageId::WorkerBusy, "WorkerBusy"},
            {MessageId::TicksLeft, "TicksLeft"},
            {MessageId::Queue, "Queue"},
            {MessageId::Queued, "Queued"},
            {MessageId::Blocked, "Blocked"},
            {MessageId::Completed, "Completed"},
        }},
        .english{{
            {MessageId::Tick, "tick {0}"},
            {MessageId::Budget, "budget {0}"},
            {MessageId::Started, "started {0}"},
            {MessageId::Workers, "workers"},
            {MessageId::WorkerIdle, "worker {0} idle"},
            {MessageId::WorkerBusy, "worker {0} {1}"},
            {MessageId::TicksLeft, "{0} of {1} ticks left"},
            {MessageId::Queue, "queue"},
            {MessageId::Queued, "{0} priority {1}"},
            {MessageId::Blocked, "{0} waits for {1}"},
            {MessageId::Completed, "completed"},
        }},
        .swedish{{
            {MessageId::Tick, "tick {0}"},
            {MessageId::Budget, "budget {0}"},
            {MessageId::Started, "startade {0}"},
            {MessageId::Workers, "arbetare"},
            {MessageId::WorkerIdle, "arbetare {0} ledig"},
            {MessageId::WorkerBusy, "arbetare {0} {1}"},
            {MessageId::TicksLeft, "{0} av {1} tick kvar"},
            {MessageId::Queue, "kö"},
            {MessageId::Queued, "{0} prioritet {1}"},
            {MessageId::Blocked, "{0} väntar på {1}"},
            {MessageId::Completed, "klara"},
        }},
    };

    static_assert(
        i18n::isComplete(kMessageTable),
        "every MessageId needs a name and text in both locales");

}
