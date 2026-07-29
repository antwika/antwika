#include "antwika/task_worker/Worker.hpp"

#include <antwika/ecs/World.hpp>

namespace antwika::task_worker
{

    std::vector<antwika::ecs::Entity> allWorkers(
        const antwika::ecs::World &world)
    {
        const auto view = world.view<Worker>();
        return std::vector<antwika::ecs::Entity>(view.begin(), view.end());
    }

} // namespace antwika::task_worker
