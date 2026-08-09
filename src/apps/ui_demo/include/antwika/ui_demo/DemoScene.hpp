#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Messages.hpp"

namespace antwika::ui_demo
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    class DemoScene final
    {
    public:
        explicit DemoScene(const Translator &translator);

        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const DemoState &state) const;

        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

}
