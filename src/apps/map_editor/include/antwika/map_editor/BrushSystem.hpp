#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class BrushSystem final : public ISystem
    {
    public:
        explicit BrushSystem(EditorStore &store);

        BrushSystem(const BrushSystem &) = delete;
        BrushSystem(BrushSystem &&) = delete;

        BrushSystem &operator=(const BrushSystem &) = delete;
        BrushSystem &operator=(BrushSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void paintBeyond(SignedCell target);

        EditorStore &store;
    };

}
