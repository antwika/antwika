#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::ui_demo
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    class DemoOverlay final
    {
    public:
        explicit DemoOverlay(Size canvas = {});

        [[nodiscard]] Size canvas() const noexcept;

        void set(DrawList picture);

        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

}
