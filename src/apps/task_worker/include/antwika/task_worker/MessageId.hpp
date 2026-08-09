#pragma once

#include <cstdint>

namespace antwika::task_worker
{

    enum class MessageId : std::uint16_t
    {
        Tick,

        Budget,

        Started,

        Workers,

        WorkerIdle,

        WorkerBusy,

        TicksLeft,

        Queue,

        Queued,

        Blocked,

        Completed,

        Count,
    };

}
