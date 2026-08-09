#pragma once

#include <cstddef>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/console/ConsoleState.hpp"

namespace antwika::console
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    namespace consoleWidgets
    {
        inline constexpr WidgetId kSheet{401};

        inline constexpr WidgetId kInput{402};
    }

    inline constexpr std::size_t kConsoleHistoryShown = 8;

    class ConsoleScene final
    {
    public:
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const ConsoleState &state) const;
    };

}
