#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::sudoku
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    class BoardOverlay final
    {
    public:
        explicit BoardOverlay(Size canvas = {});

        [[nodiscard]] Size canvas() const noexcept;

        void set(DrawList picture);

        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

}
