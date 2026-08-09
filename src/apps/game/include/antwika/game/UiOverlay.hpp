#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/HoverTargets.hpp>

#include "antwika/game/BuildTool.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::HoverTargets;

    class UiOverlay final
    {
    public:
        explicit UiOverlay(Size canvas = {});

        [[nodiscard]] Size canvas() const noexcept;

        void set(
            DrawList picture, HoverTargets targets, bool covered);

        [[nodiscard]] const DrawList &commands() const noexcept;

        [[nodiscard]] const HoverTargets &hoverTargets() const noexcept;

        [[nodiscard]] bool pointerOverUi() const noexcept;

        [[nodiscard]] bool coversPoint(
            antwika::gfx::Point at) const noexcept;

        void select(BuildTool tool) noexcept;

        void clearTool() noexcept;

        [[nodiscard]] std::optional<BuildTool> tool() const noexcept;

    private:
        Size area;
        DrawList picture;
        HoverTargets targets;
        bool covered = false;
        std::optional<BuildTool> selected = BuildTool::Road;
    };

}
