#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class KeyboardSystem final : public ISystem
    {
    public:
        KeyboardSystem(
            EditorStore &store,
            gfx::IWindow &window,
            log::ILogger &logger);

        KeyboardSystem(const KeyboardSystem &) = delete;
        KeyboardSystem(KeyboardSystem &&) = delete;

        KeyboardSystem &operator=(const KeyboardSystem &) = delete;
        KeyboardSystem &operator=(KeyboardSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void handleFastPath(input::Key key);

        void perform(HotkeyAction action);

        void keysDialogKey(input::Key key);

        EditorStore &store;
        gfx::IWindow &window;
        log::ILogger &logger;
    };

}
