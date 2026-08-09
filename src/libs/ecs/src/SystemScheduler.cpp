#include "antwika/ecs/SystemScheduler.hpp"

namespace antwika::ecs
{

    PhaseId SystemScheduler::createPhase(std::string_view name)
    {
        phases.push_back(Phase{std::string(name), {}}); // GCOVR_EXCL_LINE
        return static_cast<PhaseId>(phases.size() - 1);
    }

    void SystemScheduler::addSystem(PhaseId phase, ISystem &system)
    {
        if (phase >= phases.size())
        {
            throw EcsError("SystemScheduler: unknown phase");
        }

        phases[phase].systems.push_back(&system);
    }

    void SystemScheduler::run(World &world, antwika::time::Tick tick)
    {
        for (const auto &phase : phases)
        {
            for (auto *system : phase.systems)
            {
                system->update(world, tick);
            }

            world.commit();
        }
    }

}
