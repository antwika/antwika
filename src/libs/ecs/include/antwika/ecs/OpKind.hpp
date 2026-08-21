#pragma once

#include <cstdint>
#include <antwika/log/ILogger.hpp>
#include "antwika/ecs/Component.hpp"
#include "antwika/ecs/ComponentKey.hpp"
#include "antwika/ecs/ComponentPool.hpp"
#include "antwika/ecs/ComponentStorage.hpp"
#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/View.hpp"

namespace antwika::ecs::detail
{

    enum class OpKind : std::uint8_t
    {
        Insert,
        Remove,
    };

}
