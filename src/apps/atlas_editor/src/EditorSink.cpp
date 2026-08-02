#include "antwika/atlas_editor/EditorSink.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/StatusMessage.hpp"

namespace antwika::atlas_editor
{

    using antwika::app::asPoint;
    using antwika::app::locates;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerScrolled;
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

    EditorSink::EditorSink(
        EditorState &state,
        UiOverlay &overlay,
        IAtlasStore &store,
        const IInputEventCodec &codec,
        const Translator &translator)
        : state(state),
          overlay(overlay),
          store(store),
          codec(codec),
          translator(translator)
    {
    }

    void EditorSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            state.noteTick();

            // Described again here, for the renderer about to paint.
            // What it paints then shows the state this tick ends with.
            refreshAndAct(false);
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        // A tick's edges are cleared on the next tick's first event.
        // Clearing at the end of a tick would need this sink last.
        // Nothing can then read an edge its own tick has lost.
        // game::InputFold clears them the same way and says why.
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        folded.apply(*decoded);

        const Point at = asPoint(folded.mouse().position());

        // Where the pointer was one event ago, which a drag needs.
        // Folding the movement is what loses it.
        // The first event to carry a position moves nothing.
        // Before it there was no place to have moved from.
        const Point was = previous.value_or(at);

        if (locates(*decoded))
        {
            previous = at;
        }

        refreshAndAct(isLeftPress(*decoded));

        // The bar is resolved first.
        // A press on it is already known to be the bar's.
        if (!overlay.pointerOverUi())
        {
            applyToSheet(
                *decoded,
                at,
                Point{.x = at.x - was.x, .y = at.y - was.y});
        }
    }

    Pointer EditorSink::pointerNow(const bool pressed) const
    {
        return Pointer{
            .position = previous,
            .down = folded.mouse().isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void EditorSink::refreshAndAct(const bool pressed)
    {
        auto frame =
            describeEditor(state, pointerNow(pressed), translator);
        const auto activated = frame.interactions.activated;

        act(activated);

        // What the button did has just changed what the bar reports.
        // So it is described once more.
        // The frame drawn would otherwise predate the press.
        // See ui::Context::finish().
        if (activated != kNoWidget)
        {
            frame = describeEditor(
                state, pointerNow(pressed), translator);
        }

        overlay.set(
            std::move(frame.commands), frame.interactions.pointerOverUi);
    }

    void EditorSink::act(const WidgetId activated)
    {
        // A button zooms on the middle of the canvas.
        // The pointer is up on the button itself.
        // Zooming there walks the sheet off the bottom of the window.
        const Point middle{
            .x = static_cast<std::int32_t>(state.canvas().width / 2),
            .y = static_cast<std::int32_t>(state.canvas().height / 2)};

        if (activated == widgets::kZoomIn)
        {
            state.zoomIn(middle);
        }
        else if (activated == widgets::kZoomOut)
        {
            state.zoomOut(middle);
        }
        else if (activated == widgets::kResetView)
        {
            state.resetView();
        }
        else if (activated == widgets::kGrid)
        {
            state.toggleGrid();
        }
        else if (activated == widgets::kSave)
        {
            save();
        }
        else if (activated == widgets::kLoad)
        {
            load();
        }
        else
        {
            // Searched rather than subtracted from the first id.
            // A widget this bar lacks cannot become one it has.
            for (std::size_t index = 0; index < kToolCount; ++index)
            {
                if (activated == widgets::toolWidget(
                        static_cast<Tool>(index)))
                {
                    state.selectTool(static_cast<Tool>(index));
                    return;
                }
            }

            for (std::size_t index = 0;
                 index < defaultPalette().size();
                 ++index)
            {
                if (activated == widgets::swatchWidget(index))
                {
                    state.selectColor(index);
                    return;
                }
            }
        }
    }

    void EditorSink::applyToSheet(
        const InputEvent &event, const Point at, const Point moved)
    {
        if (const auto *scrolled =
                std::get_if<PointerScrolled>(&event);
            scrolled != nullptr)
        {
            // A notch away from the artist zooms in.
            // That is what every other tool they use does.
            if (scrolled->vertical > 0)
            {
                state.zoomIn(at);
            }
            else if (scrolled->vertical < 0)
            {
                state.zoomOut(at);
            }

            return;
        }

        if (!locates(event))
        {
            return;
        }

        state.moveTo(at);

        // Held rather than pressed.
        // A drag then paints every pixel it crosses.
        if (folded.mouse().isDown(MouseButton::Middle))
        {
            state.panBy(moved);
        }
        else if (folded.mouse().isDown(MouseButton::Left))
        {
            state.applyAt(at);
        }
        else if (folded.mouse().isDown(MouseButton::Right))
        {
            state.eraseAt(at);
        }
    }

    // Both catches say the same thing about a different failure.
    // One place rather than the aggregate written out twice.
    // Which also keeps a caught failure's own words off a branch.
    void EditorSink::report(const MessageId id, std::string detail)
    {
        state.setStatus({.id = id, .detail = std::move(detail)});
    }

    void EditorSink::save()
    {
        // A failed save must not end the session.
        // Whoever is editing still has every unsaved change.
        // Telling them beats a stack unwinding out of the tick loop.
        // AtlasEditorError and GfxError both derive from runtime_error.
        // What is shown is the message either way, so one catch does.
        try
        {
            store.save(state.image().bitmap());
            state.markSaved();
            state.setStatus(
                {.id = MessageId::Saved,
                 .detail = store.savePath()});
        }
        catch (const std::runtime_error &failed) // GCOVR_EXCL_LINE
        {
            report(MessageId::SaveFailed, failed.what());
        }
    }

    void EditorSink::load()
    {
        try
        {
            auto loaded = store.load();

            if (!loaded.has_value())
            {
                state.setStatus(
                    {.id = MessageId::NothingToLoad,
                     .detail = {}});
                return;
            }

            state.replace(Canvas(std::move(*loaded)));
            state.setStatus(
                {.id = MessageId::Loaded, .detail = {}});
        }
        catch (const std::runtime_error &failed) // GCOVR_EXCL_LINE
        {
            report(MessageId::LoadFailed, failed.what());
        }
    }

} // namespace antwika::atlas_editor
