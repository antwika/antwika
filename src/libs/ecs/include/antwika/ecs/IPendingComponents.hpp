#pragma once

#include <antwika/log/ILogger.hpp>

#include "antwika/ecs/Entity.hpp"

namespace antwika::ecs::detail
{

    class IPendingComponents
    {
    public:
        IPendingComponents() = default;

        virtual ~IPendingComponents() = default;

        IPendingComponents(const IPendingComponents &) = delete;
        IPendingComponents(IPendingComponents &&) = delete;

        IPendingComponents &operator=(const IPendingComponents &)
            = delete;
        IPendingComponents &operator=(IPendingComponents &&) = delete;

        virtual void apply() = 0;

        virtual void clear() noexcept = 0;
    };

}
