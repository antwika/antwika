#pragma once

#include "antwika/ecs/World.hpp"

namespace antwika::ecs
{

    class OpenPhase final
    {
    public:
        explicit OpenPhase(World &world) noexcept;

        ~OpenPhase();

        OpenPhase(const OpenPhase &) = delete;
        OpenPhase(OpenPhase &&) = delete;

        OpenPhase &operator=(const OpenPhase &) = delete;
        OpenPhase &operator=(OpenPhase &&) = delete;

    private:
        World *world;
    };

}
