#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::console
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    class ConsolePicture final
    {
    public:
        explicit ConsolePicture(Size canvas = {});

        [[nodiscard]] Size canvas() const noexcept;

        void set(DrawList picture);

        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

}
