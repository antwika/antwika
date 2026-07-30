#include "antwika/game/UiSink.hpp"

#include <optional>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/PointerReading.hpp"

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::ui::kNoWidget;

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
        const IInputEventCodec &codec,
        const Toolbar &toolbar,
        Camera home)
        : camera(camera),
          overlay(overlay),
          codec(codec),
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
            state.beginTick();
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        located = located || locates(*decoded);
        state.apply(*decoded);

        refreshAndAct(isLeftPress(*decoded));
    }

    Pointer UiSink::pointerNow(bool pressed) const
    {
        const auto &mouse = state.mouse();

        return Pointer{
            .position = located
                            ? std::optional<Point>{asPoint(mouse.position())}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void UiSink::refreshAndAct(bool pressed)
    {
        auto frame = toolbar.describe(
            overlay.canvas(), pointerNow(pressed), camera);
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

        // The zoom the bar reports has just changed.
        // So it is described once more.
        // Otherwise it would show the level it was pressed at.
        if (activated != kNoWidget)
        {
            frame = toolbar.describe(
                overlay.canvas(), pointerNow(pressed), camera);
        }

        overlay.set(
            std::move(frame.commands), frame.interactions.pointerOverUi);
    }

} // namespace antwika::game
