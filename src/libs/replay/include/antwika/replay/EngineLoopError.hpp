#pragma once

#include <stdexcept>

namespace antwika::replay
{

    /**
     * @brief Thrown by EngineLoop::run() when maxTicks is reached without
     * an engine.stop event having been dispatched.
     *
     * maxTicks is a safety valve, not a domain concept -- production runs
     * typically omit it and rely solely on an explicit stop. Tests should
     * always pass one, so a forgotten stop event fails loudly here instead
     * of hanging the run indefinitely. Mirrors antwika::ecs::EcsError and
     * antwika::replay::ReplayFormatError: a single, specific, catchable
     * type rather than a vague std::runtime_error.
     */
    class EngineLoopError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::replay
