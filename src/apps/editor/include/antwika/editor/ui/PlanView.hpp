#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DoubleClick.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/editor/editor/state/FocusedField.hpp"
#include "antwika/editor/editor/state/PlanDrag.hpp"
#include "antwika/editor/plan/PlanBoard.hpp"
#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class PlanView final : public IEditorView
    {
    public:
        [[nodiscard]] bool claims(
            map::View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        [[nodiscard]] bool layoutPanel(
            ui::Context &context, const ViewContext &viewContext) override;

        void carryFrame(
            const ui::Frame &frame,
            const ViewContext &viewContext) override;

        void drawOverlay(const ViewContext &viewContext) override;

        [[nodiscard]] bool takeWidgets(
            const ui::Interactions &interactions,
            const ViewContext &viewContext,
            std::optional<std::string> &notice) override;

        void open(std::string path);

        [[nodiscard]] const Board &getBoard() const noexcept;

        [[nodiscard]] bool isUnsaved() const noexcept;

        [[nodiscard]] std::optional<std::string> save();

        void layout(ui::Context &context, FocusedField focusedField);

        void updateFrame(
            const ui::Frame &frame,
            std::optional<gfx::Point> pointer);

        void drawGhost(
            gfx::ViewportRenderer &viewportRenderer,
            std::optional<gfx::Point> pointer);

        [[nodiscard]] bool consumeWidgets(
            const ui::Interactions &interactions,
            std::optional<gfx::Point> pointer,
            FocusedField &focusedField,
            std::optional<std::string> &notice);

        void carryEdits(
            const ui::Frame &frame, FocusedField &focusedField);

        [[nodiscard]] bool isPicked() const noexcept;

        void carry(PlanDrag heldDrag);

        void draggedTo(gfx::Point pointer);

        [[nodiscard]] std::optional<std::string> letGo();

        void endDrag() noexcept;

        [[nodiscard]] bool isDragging() const noexcept;

    private:
        [[nodiscard]] std::optional<std::uint32_t> getCardWidth() const;

        Board planBoard;
        std::string planPath;
        bool planUnsaved = false;

        std::optional<std::pair<Column, std::size_t>> planPicked;
        std::optional<PlanDrag> planDrag;

        std::uint32_t planColumnWidth = 0;

        std::size_t planBodyCursor = 0;
        std::size_t planBodyAnchor = 0;
        std::size_t planBodyScroll = 0;
        ui::DragOrigin planBodyDrag = ui::DragOrigin::None;
    };

}
