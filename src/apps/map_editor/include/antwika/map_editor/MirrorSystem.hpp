#pragma once

#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class MirrorSystem final : public ISystem
    {
    public:
        explicit MirrorSystem(EditorStore &store);

        MirrorSystem(const MirrorSystem &) = delete;
        MirrorSystem(MirrorSystem &&) = delete;

        MirrorSystem &operator=(const MirrorSystem &) = delete;
        MirrorSystem &operator=(MirrorSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        EditorStore &store;
        std::vector<tilemap::Entity> synced{};
        std::vector<ecs::Entity> spawned{};
        bool fresh = true;
    };

}
