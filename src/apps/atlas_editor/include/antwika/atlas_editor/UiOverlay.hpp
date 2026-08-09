#pragma once

#include <optional>

#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::atlas_editor
{

    using antwika::gfx::Rect;
    using antwika::ui::DrawList;

    struct PaneRects final
    {
        Rect sheet{};

        std::optional<Rect> preview{};

        [[nodiscard]] bool operator==(const PaneRects &other) const =
            default;
    };

    class UiOverlay final
    {
    public:
        void set(DrawList picture, bool covered, PaneRects panes);

        [[nodiscard]] const DrawList &commands() const noexcept;

        [[nodiscard]] bool pointerOverUi() const noexcept;

        [[nodiscard]] const PaneRects &panes() const noexcept;

    private:
        DrawList picture;
        bool covered = false;
        PaneRects paneRects{};
    };

}
