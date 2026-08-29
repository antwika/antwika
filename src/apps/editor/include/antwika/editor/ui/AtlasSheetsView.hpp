#pragma once

#include <string>

#include <antwika/gfx/PointF.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class AtlasSheetsView final : public IEditorView
    {
    public:
        [[nodiscard]] bool claims(
            View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        [[nodiscard]] bool consumePress(
            const ViewContext &viewContext,
            const input::PointerButtonPressed &downPressed) override;

        [[nodiscard]] bool consumeRelease(
            const ViewContext &viewContext,
            const input::PointerButtonReleased &upReleased) override;

        [[nodiscard]] bool consumeScroll(
            const ViewContext &viewContext,
            const input::PointerScrolled &rolledScrolled) override;

        void trackPointer(const ViewContext &viewContext) override;

        [[nodiscard]] bool takesPaintKeys() const noexcept override;

        [[nodiscard]] bool offersPaint(
            Paint paint) const noexcept override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        [[nodiscard]] bool blockedAsTransitionSlot(
            const ViewContext &viewContext);

    private:
        void finishShapedStroke(
            const ViewContext &viewContext,
            gfx::PointF releasedAtPoint);

        [[nodiscard]] bool paintedOnAtlasPixel(
            const ViewContext &viewContext);
    };

}
