#pragma once

#include <antwika/component/DirectionKeys.hpp>
#include <antwika/system/SimulationState.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::gameplay
{

    class ISimulation
    {
    public:
        ISimulation() = default;

        virtual ~ISimulation() = default;

        ISimulation(const ISimulation &) = delete;
        ISimulation(ISimulation &&) = delete;

        ISimulation &operator=(const ISimulation &) = delete;
        ISimulation &operator=(ISimulation &&) = delete;

        virtual void setWasdKeys(component::DirectionKeys keys) noexcept = 0;

        virtual void setArrowKeys(component::DirectionKeys keys) noexcept = 0;

        virtual void setSimulation(
            system::SimulationState state) noexcept = 0;

        virtual void forgetPatrols() = 0;

        virtual void clearSteering() noexcept = 0;

        virtual void run(time::Tick tick) = 0;
    };

}
