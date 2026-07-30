#pragma once

#include <stdexcept>

namespace antwika::ecs
{

    /**
     * @brief Thrown for an ECS misuse: accessing a dead or unknown
     * entity, a component an entity doesn't have, or an exhausted
     * entity-index space.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::replay::ReplayFormatError, rather than a vague
     * std::runtime_error. Index exhaustion is logged at Level::Fatal
     * before being thrown — see EntityManager — since a caller isn't
     * expected to recover from it, only to unwind cleanly.
     */
    class EcsError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::ecs
