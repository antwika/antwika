#pragma once

#include <optional>
#include <string>

#include <antwika/input/event/KeyPressed.hpp>
#include <antwika/input/event/PointerButtonPressed.hpp>
#include <antwika/input/event/PointerButtonReleased.hpp>
#include <antwika/input/event/PointerScrolled.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/view/ViewContext.hpp"

namespace antwika::editor
{

    class IEditorView
    {
    public:
        IEditorView() = default;

        IEditorView(const IEditorView &) = delete;
        IEditorView(IEditorView &&) = delete;

        IEditorView &operator=(const IEditorView &) = delete;
        IEditorView &operator=(IEditorView &&) = delete;

        virtual ~IEditorView() = default;

        [[nodiscard]] virtual bool claims(
            View shownView, bool playing) const noexcept
            = 0;

        [[nodiscard]] virtual std::string getStatusText(
            const ViewContext &viewContext) const
            = 0;

        virtual void draw(
            const ViewContext &viewContext, const ui::Frame &frame)
            = 0;

        [[nodiscard]] virtual bool layoutPanel(
            ui::Context &, const ViewContext &)
        {
            return false;
        }

        virtual void carryFrame(const ui::Frame &, const ViewContext &)
        {
        }

        virtual void drawOverlay(const ViewContext &)
        {
        }

        [[nodiscard]] virtual bool takesPaintKeys() const noexcept
        {
            return false;
        }

        [[nodiscard]] virtual bool offersPaint(
            const Paint paint) const noexcept
        {
            return paint == Paint::Brush || paint == Paint::Line
                   || paint == Paint::Fill;
        }

        [[nodiscard]] virtual bool takeWidgets(
            const ui::Interactions &,
            const ViewContext &,
            std::optional<std::string> &)
        {
            return false;
        }

        [[nodiscard]] virtual bool consumePress(
            const ViewContext &, const input::PointerButtonPressed &)
        {
            return false;
        }

        [[nodiscard]] virtual bool consumeRelease(
            const ViewContext &, const input::PointerButtonReleased &)
        {
            return false;
        }

        [[nodiscard]] virtual bool consumeScroll(
            const ViewContext &, const input::PointerScrolled &)
        {
            return false;
        }

        virtual void trackPointer(const ViewContext &)
        {
        }

        [[nodiscard]] virtual bool consumeKey(
            const ViewContext &, const input::KeyPressed &)
        {
            return false;
        }
    };

}
