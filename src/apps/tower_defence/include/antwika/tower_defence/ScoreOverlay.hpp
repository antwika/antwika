#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::tower_defence
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    class ScoreOverlay final
    {
    public:
        explicit ScoreOverlay(Size canvas = {});

        [[nodiscard]] Size canvas() const noexcept;

        void set(DrawList picture);

        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

}
