#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace menuWidgets
    {
        inline constexpr WidgetId kNewGame{101};

        inline constexpr WidgetId kQuit{102};

        inline constexpr WidgetId kLoadGame{103};

        inline constexpr WidgetId kWorldMap{104};

        inline constexpr WidgetId kOptions{105};
    }

    class MainMenuScene final
    {
    public:
        explicit MainMenuScene(const Translator &translator);

        [[nodiscard]] Frame describe(Size canvas, Pointer pointer) const;

        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

}
