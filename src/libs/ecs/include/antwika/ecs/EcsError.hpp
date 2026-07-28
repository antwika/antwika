#pragma once

#include <stdexcept>

namespace antwika::ecs
{

    /**
     * @brief Thrown for a recoverable ECS misuse: accessing a dead or
     * unknown entity, or a component an entity doesn't have.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::replay::ReplayFormatError, rather than a vague
     * std::runtime_error. Entity-index exhaustion is not reported this
     * way — see EntityManager, which logs fatal and terminates instead,
     * since that condition isn't one an application could recover from.
     */
    class EcsError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::ecs
