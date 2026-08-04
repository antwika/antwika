#pragma once

#include <cstdint>

namespace antwika::task_worker
{

    /**
     * @brief The numbers this application reads off config.json
     * beside its assets at startup.
     *
     * Every field defaults to the value it externalizes, so a
     * default-constructed one is the shipped application and a
     * missing file changes nothing. The format's mechanics live in
     * antwika::config; what may not move here is anything a recorded
     * click's meaning depends on, for the reasons apps/game's page
     * gives.
     */
    struct TaskWorkerConfig
    {
        /** @brief How many workers pull from the queue. */
        std::uint32_t workerCount = 2;
        /** @brief Milliseconds one tick takes on the wall clock. */
        std::int32_t tickIntervalMs = 400;
    };

} // namespace antwika::task_worker
