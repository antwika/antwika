#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    [[nodiscard]] ui::Frame describePanel(
        const EditorStore &store,
        gfx::Size canvas,
        ui::Pointer pointer,
        const ui::Keyboard &keyboard);

}
