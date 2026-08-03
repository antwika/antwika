#include "antwika/atlas_editor/EditorSink.hpp"

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/OpeningSheet.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/input/Key.hpp>
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
    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerScrolled;
    using antwika::ui::kNoWidget;

    namespace
    {
        // Read off the event itself rather than off the folded state.
        // Mouse::wasPressed() answers for the whole tick.
        // Several events of one tick would each read as the press.
        [[nodiscard]] bool isPressOf(
            const InputEvent &event, const MouseButton button) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr && pressed->button == button;
        }

        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            return isPressOf(event, MouseButton::Left);
        }

        [[nodiscard]] bool isLeftRelease(const InputEvent &event) noexcept
        {
            const auto *released =
                std::get_if<PointerButtonReleased>(&event);

            return released != nullptr
                   && released->button == MouseButton::Left;
        }

        // A repeat is the window system saying a key is still held.
        // Cut, copy and paste are things one does once per press.
        // So a held Ctrl+V pastes once rather than once a frame.
        [[nodiscard]] const KeyPressed *freshPress(
            const InputEvent &event) noexcept
        {
            const auto *pressed = std::get_if<KeyPressed>(&event);

            if (pressed == nullptr || pressed->repeat)
            {
                return nullptr;
            }

            return pressed;
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
          translator(translator),
          // The sheet as this run opened it, before any edit lands.
          // What a recorded announcement is checked against.
          expectedOpening(openingSheetEvent(state.image()).payload)
    {
    }

    void EditorSink::handle(const TickEvent &event)
    {
        // The sheet the recording was drawn on, checked out loud.
        // Every Pick lifts a colour off the sheet.
        // A replay against a changed --image diverged in silence.
        if (event.event.name == events::kOpeningSheet)
        {
            if (event.event.payload != expectedOpening)
            {
                throw AtlasEditorError(
                    "atlas_editor: this replay was recorded against "
                    "another opening sheet (" + event.event.payload
                    + ", and this run opened " + expectedOpening
                    + "); replaying it here would repaint different "
                      "pixels in silence");
            }

            return;
        }

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

        // A key reaches the sheet wherever the pointer happens to be.
        // There is no text field here for one to have gone into instead.
        applyToKeyboard(*decoded);

        // The bar is resolved first.
        // A press on it is already known to be the bar's.
        if (!overlay.pointerOverUi())
        {
            applyToSheet(*decoded, was, at);
        }
    }

    void EditorSink::applyToKeyboard(const InputEvent &event)
    {
        const KeyPressed *pressed = freshPress(event);

        if (pressed == nullptr || !pressed->modifiers.control)
        {
            return;
        }

        if (pressed->key == Key::C)
        {
            state.copySelection();
        }
        else if (pressed->key == Key::X)
        {
            state.cutSelection();
        }
        else if (pressed->key == Key::V)
        {
            state.pasteClipboard();
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
        else if (activated == widgets::kGuides)
        {
            state.toggleGuides();
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

    // Bresenham over the whole segment, both ends included.
    // Integer throughout, which is a rule rather than a preference.
    // Which pixel a press means is simulation state.
    // A float's last bit is not the same on every toolchain.
    // Painting `from` again costs nothing.
    // The last event already put it down.
    // And a write of the colour already there is no change at all.
    // So an edit is never counted twice.
    void EditorSink::strokeAlong(
        const Point from, const Point to, const Brush brush)
    {
        const std::int32_t stepX = to.x < from.x ? -1 : 1;
        const std::int32_t stepY = to.y < from.y ? -1 : 1;

        // Distances as magnitudes, the vertical one negated.
        // That is the form the two comparisons below are written for.
        const std::int32_t spanX = (to.x - from.x) * stepX;
        const std::int32_t spanY = (from.y - to.y) * stepY;

        std::int32_t error = spanX + spanY;
        Point walked = from;

        for (;;)
        {
            (state.*brush)(walked);

            if (walked.x == to.x && walked.y == to.y)
            {
                return;
            }

            const std::int32_t doubled = 2 * error;

            if (doubled >= spanY)
            {
                error += spanY;
                walked.x += stepX;
            }

            if (doubled <= spanX)
            {
                error += spanX;
                walked.y += stepY;
            }
        }
    }

    void EditorSink::applyToSheet(
        const InputEvent &event, const Point was, const Point at)
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

        // The right button's first job is dropping the rectangle.
        // Whichever tool is in hand.
        // A rectangle marked with Select outlives a trip to the palette.
        if (isPressOf(event, MouseButton::Right))
        {
            state.clearSelection();
        }

        // Panning is the middle button's whatever else is going on.
        if (folded.mouse().isDown(MouseButton::Middle))
        {
            state.panBy(Point{.x = at.x - was.x, .y = at.y - was.y});
            return;
        }

        // Select's buttons are a gesture rather than a brush.
        // So it takes the pointer and leaves before the strokes below.
        // Which is what keeps its right button to clearing alone.
        if (state.tool() == Tool::Select)
        {
            applySelection(event, at);
            return;
        }

        // Held rather than pressed.
        // A drag then paints every pixel it crosses.
        // Which is the segment from where the pointer was, not one dot.
        // A window system reports a fast stroke as a few long jumps.
        // A dot per event would leave the gaps between them bare.
        if (folded.mouse().isDown(MouseButton::Left))
        {
            strokeAlong(was, at, &EditorState::applyAt);
        }
        else if (folded.mouse().isDown(MouseButton::Right))
        {
            strokeAlong(was, at, &EditorState::eraseAt);
        }
    }

    // Down, along and up, in that order and from the events themselves.
    // A release is not a held button, so it cannot be read off the fold.
    void EditorSink::applySelection(
        const InputEvent &event, const Point at)
    {
        if (isLeftPress(event))
        {
            state.beginSelecting(at);
        }
        else if (isLeftRelease(event))
        {
            state.finishSelecting(at);
        }
        else if (folded.mouse().isDown(MouseButton::Left))
        {
            state.dragSelectionTo(at);
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
