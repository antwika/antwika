#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/Phase.hpp>
#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::gameplay
{

    enum class Phase : std::uint8_t
    {
        Sending,
        Walking,
        Orienting,

        Health,
    };

    [[nodiscard]] constexpr Phase lastEnumerator(Phase) noexcept
    {
        return Phase::Health;
    }

    inline constexpr std::size_t kPhaseCount = enums::kCount<Phase>;

    inline constexpr std::array<Phase, kPhaseCount> kAllPhases =
        enums::kAll<Phase>;

    class GameLoop final
    {
    public:
        explicit GameLoop(ecs::World &world);

        [[nodiscard]] ecs::World &world() noexcept;

        [[nodiscard]] const ecs::World &world() const noexcept;

        void addSystem(Phase phase, ecs::ISystem &system);

        void run(time::Tick tick);

    private:
        ecs::World *worldValue;
        ecs::SystemScheduler scheduler;
        std::array<ecs::PhaseId, kPhaseCount> phases{};
    };

    [[nodiscard]] std::string_view phaseName(Phase phase);

}
