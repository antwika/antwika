#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/ITexture.hpp>

#include "antwika/editor/view/IEditorView.hpp"

namespace antwika::editor
{

    class GizmoView final : public IEditorView
    {
    public:
        [[nodiscard]] std::optional<std::size_t> getPickedIndex()
            const noexcept;

        void pick(std::optional<std::size_t> gizmoIndex) noexcept;

        void paint(
            const ViewContext &viewContext,
            geometry::GridCell pixelCell,
            bool erases);

        [[nodiscard]] bool claims(
            View shownView, bool playing) const noexcept override;

        [[nodiscard]] std::string getStatusText(
            const ViewContext &viewContext) const override;

        void draw(
            const ViewContext &viewContext,
            const ui::Frame &frame) override;

        [[nodiscard]] bool layoutRail(ui::Context &context);

        [[nodiscard]] bool takeWidgets(
            const ui::Interactions &interactions,
            const ViewContext &viewContext,
            std::optional<std::string> &notice) override;

        [[nodiscard]] bool consumePress(
            const ViewContext &viewContext,
            const input::PointerButtonPressed &downPressed) override;

        void trackPointer(const ViewContext &viewContext) override;

    private:
        void drawSheet(const ViewContext &viewContext);

        std::optional<std::size_t> gizmoPicked = 0;
        std::unique_ptr<gfx::ITexture> gizmoCheckerTexture;
    };

}
