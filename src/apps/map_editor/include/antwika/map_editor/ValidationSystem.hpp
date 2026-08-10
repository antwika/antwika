#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class ValidationSystem final : public ISystem
    {
    public:
        explicit ValidationSystem(EditorStore &store);

        ValidationSystem(const ValidationSystem &) = delete;
        ValidationSystem(ValidationSystem &&) = delete;

        ValidationSystem &operator=(const ValidationSystem &) = delete;
        ValidationSystem &operator=(ValidationSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        EditorStore &store;
    };

}
