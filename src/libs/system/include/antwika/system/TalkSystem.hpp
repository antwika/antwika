#pragma once

#include <cstddef>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    inline constexpr float kTalkRadius = 1.6F;

    class TalkSystem final : public ecs::ISystem
    {
    public:
        void setRosterCount(std::size_t rosterCount) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        std::size_t rosterCount = 0;
    };

}
