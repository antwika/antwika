#pragma once

#include <string>

#include <antwika/map/Settings.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class AtlasSheetsView final : public IEditorView
    {
    public:
        [[nodiscard]] bool claims(
            map::View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        [[nodiscard]] bool consumePress(
            const ViewContext &viewContext,
            const input::PointerButtonPressed &downPressed) override;

        [[nodiscard]] bool takesPaintKeys() const noexcept override;

        [[nodiscard]] bool offersPaint(
            map::Paint paint) const noexcept override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        [[nodiscard]] bool blockedAsTransitionSlot(
            const ViewContext &viewContext);

    private:
        [[nodiscard]] bool paintedOnAtlasPixel(
            const ViewContext &viewContext);
    };

}
