#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Messages.hpp"
#include "antwika/game/SaveLoadState.hpp"

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace saveWidgets
    {
        inline constexpr WidgetId kPicker{201};

        inline constexpr WidgetId kName{202};

        inline constexpr WidgetId kSave{203};

        inline constexpr WidgetId kLoad{204};

        inline constexpr WidgetId kBack{205};

        inline constexpr WidgetId kFirstOption{300};
    }

    class SaveLoadScene final
    {
    public:
        explicit SaveLoadScene(const Translator &translator);

        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const SaveLoadState &state) const;

        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

}
