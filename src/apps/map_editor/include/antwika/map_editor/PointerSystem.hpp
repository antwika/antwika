#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class PointerSystem final : public ISystem
    {
    public:
        PointerSystem(
            EditorStore &store, const gfx::ViewportRenderer &view);

        PointerSystem(const PointerSystem &) = delete;
        PointerSystem(PointerSystem &&) = delete;

        PointerSystem &operator=(const PointerSystem &) = delete;
        PointerSystem &operator=(PointerSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        EditorStore &store;
        const gfx::ViewportRenderer &view;
    };

}
