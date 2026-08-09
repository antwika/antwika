#pragma once

#include <cstdint>

namespace antwika::task_worker
{

    struct TaskWorkerConfig final
    {
        std::uint32_t workerCount = 2;
        std::int32_t tickIntervalMs = 400;
    };

}
