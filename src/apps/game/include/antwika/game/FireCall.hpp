#pragma once

#include <antwika/ecs/Entity.hpp>

namespace antwika::game
{

    struct FireCall final
    {
        antwika::ecs::Entity target = antwika::ecs::kNullEntity;

        [[nodiscard]] bool operator==(const FireCall &other) const
            = default;
    };

}
