#include "antwika/game/UiSink.hpp"

#include <cstddef>
#include <optional>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::ui::kNoWidget;
    using antwika::ui::WidgetId;

    namespace
    {
        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr
                   && pressed->button == MouseButton::Left;
        }
    } // namespace

    UiSink::UiSink(
        Camera &camera,
        UiOverlay &overlay,
        const InputFold &input,
        const Toolbar &toolbar,
        Camera home)
        : camera(camera),
          overlay(overlay),
          input(input),
          toolbar(toolbar),
          home(home)
    {
    }

    void UiSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            // What it paints then shows the state this tick ends with.
            refreshAndAct(false);
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        refreshAndAct(isLeftPress(*decoded));
    }

    Pointer UiSink::pointerNow(bool pressed) const
    {
        const auto &mouse = input.state().mouse();

        return Pointer{
            .position = input.located()
                            ? std::optional<Point>{input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void UiSink::refreshAndAct(bool pressed)
    {
        auto frame = toolbar.describe(
            overlay.canvas(), pointerNow(pressed), camera, overlay.tool());
        const auto activated = frame.interactions.activated;

        if (activated == widgets::kZoomIn)
        {
            camera.zoomIn();
        }
        else if (activated == widgets::kZoomOut)
        {
            camera.zoomOut();
        }
        else if (activated == widgets::kResetView)
        {
            camera = home;
        }
        else
        {
            selectFrom(activated);
        }

        // The zoom the bar reports has just changed.
        // So it is described once more.
        // Otherwise it would show the level it was pressed at.
        if (activated != kNoWidget)
        {
            frame = toolbar.describe(
                overlay.canvas(),
                pointerNow(pressed),
                camera,
                overlay.tool());
        }

        overlay.set(
            std::move(frame.commands), frame.interactions.pointerOverUi);
    }

    void UiSink::selectFrom(WidgetId activated)
    {
        // Searched rather than subtracted from kFirstTool.
        // A widget this bar lacks cannot become a tool it has.
        for (std::size_t index = 0; index < kBuildToolCount; ++index)
        {
            const auto tool = static_cast<BuildTool>(index);

            if (activated == widgets::toolWidget(tool))
            {
                overlay.select(tool);
                return;
            }
        }
    }

} // namespace antwika::game
