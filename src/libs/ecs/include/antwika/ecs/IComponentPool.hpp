#pragma once

#include <span>

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs::detail
{

    class IComponentPool
    {
    public:
        IComponentPool() = default;

        virtual ~IComponentPool() = default;

        IComponentPool(const IComponentPool &) = delete;
        IComponentPool(IComponentPool &&) = delete;

        IComponentPool &operator=(const IComponentPool &) = delete;
        IComponentPool &operator=(IComponentPool &&) = delete;

        virtual void commit() = 0;

        virtual void removeAll(std::span<const Entity> batchEntities) = 0;
    };

}
