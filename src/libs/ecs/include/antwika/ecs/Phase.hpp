#pragma once

#include <cstdint>

namespace antwika::ecs
{

    /**
     * @brief Identifies a phase created via SystemScheduler::createPhase.
     *
     * Ordering comes entirely from creation order, not from this value —
     * it's just a handle for addSystem() to refer back to a phase by.
     */
    using PhaseId = std::uint32_t;

} // namespace antwika::ecs
