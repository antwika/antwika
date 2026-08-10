#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/TextEdit.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class EntityEditSystem final : public ISystem
    {
    public:
        explicit EntityEditSystem(EditorStore &store);

        EntityEditSystem(const EntityEditSystem &) = delete;
        EntityEditSystem(EntityEditSystem &&) = delete;

        EntityEditSystem &operator=(const EntityEditSystem &) = delete;
        EntityEditSystem &operator=(EntityEditSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void edit(const ui::TextEdit &change);

        void applyBuffers();

        EditorStore &store;
    };

}
