#include "antwika/ecs/SystemScheduler.hpp"

#include "antwika/ecs/OpenPhase.hpp"

namespace antwika::ecs
{

    namespace
    {
        [[nodiscard]] std::string phaseNamesOf(
            const std::vector<std::string> &names)
        {
            if (names.empty())
            {
                return "no phase has been created";
            }

            std::string knownNames = "known phases are ";

            for (std::size_t index = 0; index < names.size(); ++index)
            {
                knownNames += index == 0 ? "" : ", ";
                knownNames += names[index];
            }

            return knownNames;
        }

    }

    PhaseId SystemScheduler::createPhase(std::string_view name)
    {
        phases.push_back(PhaseEntry{std::string(name), {}}); // GCOVR_EXCL_LINE
        return static_cast<PhaseId>(phases.size() - 1);
    }

    void SystemScheduler::addSystem(PhaseId phase, ISystem &system)
    {
        if (getRawValue(phase) >= phases.size())
        {
            std::vector<std::string> names;
            names.reserve(phases.size());

            for (const auto &entry : phases)
            {
                names.push_back(entry.name);
            }

            throw EcsError(
                "SystemScheduler: unknown phase, " + phaseNamesOf(names));
        }

        phases[getRawValue(phase)].systems.push_back(&system);
    }

    void SystemScheduler::run(World &world, antwika::time::Tick tick)
    {
        for (const auto &phase : phases)
        {
            const OpenPhase openPhase(world);

            for (auto *system : phase.systems)
            {
                system->update(world, tick);
            }
        }
    }

}
