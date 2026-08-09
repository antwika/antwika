#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace modalWidgets
    {
        inline constexpr WidgetId kMainMenu{201};

        inline constexpr WidgetId kResume{202};
    }

    class MenuModalScene final
    {
    public:
        explicit MenuModalScene(const Translator &translator);

        [[nodiscard]] Frame describe(Size canvas, Pointer pointer) const;

    private:
        const Translator &translator;
    };

}
