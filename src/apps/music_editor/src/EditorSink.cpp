#include "antwika/music_editor/EditorSink.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/music_editor/EditorKeys.hpp"

namespace antwika::music_editor
{

    using antwika::app::locates;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerScrolled;

    namespace
    {
        // How far one notch of the wheel moves the pane.
        // Three lines, which is what everything else moves.
        constexpr std::size_t kLinesPerNotch = 3;

        [[nodiscard]] const PointerButtonPressed *leftPress(
            const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            if (pressed == nullptr
                || pressed->button != MouseButton::Left)
            {
                return nullptr;
            }

            return pressed;
        }

        [[nodiscard]] bool isLeftRelease(const InputEvent &event) noexcept
        {
            const auto *released =
                std::get_if<PointerButtonReleased>(&event);

            return released != nullptr
                   && released->button == MouseButton::Left;
        }
    } // namespace

    EditorSink::EditorSink(
        EditorState &state,
        Score &score,
        Playback &playback,
        const IInputEventCodec &codec,
        const EditorScene &scene,
        const Size canvas)
        : state(state),
          score(score),
          playback(playback),
          codec(codec),
          scene(scene),
          canvas(canvas)
    {
    }

    void EditorSink::handle(const TickEvent &event)
    {
        // A tick's edges are cleared on the next tick's first event.
        // Clearing at the end of a tick would need this sink last.
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // The document is re-read from what the input just did.
            // Only then is the sound advanced.
            // A note decided now reads the score as it now stands.
            score.read(state.source);
            playback.step(state.paused);

            refreshAndAct(PointerEdge{}, ui::Keyboard{});

            return;
        }

        const auto decoded = codec.decode(event.event);

        if (!decoded.has_value())
        {
            return;
        }

        located = located || locates(*decoded);
        folded.apply(*decoded);

        // The characters live here for as long as the Context does.
        // ui::Keyboard borrows them rather than owning them.
        std::string characters;
        ui::Keyboard keyboard;

        if (const auto *key = std::get_if<KeyPressed>(&*decoded))
        {
            // Escape is this application's alone.
            // antwika::ui is never told about it.
            // A field that gave up would throw the score away.
            if (key->key == antwika::input::Key::Escape)
            {
                state.paused = !state.paused;
            }

            const auto meaning = uiKeyFor(key->key, key->modifiers);

            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            characters =
                typedTextFor(key->key, key->modifiers, state.layout);

            // A paste is the clipboard typed.
            // Which is all a paste can be here.
            // antwika::ui keeps nothing between frames.
            // So it has nowhere to have put what was cut.
            // This editor holds it, off recorded key presses.
            if (key->modifiers.control
                && key->key == antwika::input::Key::V)
            {
                characters = state.clipboard;
            }

            // One edge per character.
            // That is what orders them against everything else.
            for (std::size_t at = 0; at < characters.size(); ++at)
            {
                keyboard.keys.push_back(ui::Key::Character);
            }
        }

        if (const auto *wheel = std::get_if<PointerScrolled>(&*decoded))
        {
            scrollBy(wheel->vertical);
        }

        if (isLeftRelease(*decoded))
        {
            state.dragging = false;
        }

        keyboard.typed = characters;

        const auto *press = leftPress(*decoded);

        const bool moved =
            std::holds_alternative<antwika::input::PointerMoved>(
                *decoded);

        refreshAndAct(
            PointerEdge{
                .pressed = press != nullptr,
                // Shift makes a press carry a selection on.
                // A move under a held press is what drags one out.
                // Nothing else does.
                // So typing with a button held moves no caret.
                .extends = press != nullptr
                    ? press->modifiers.shift
                    : moved && state.dragging},
            keyboard);
    }

    ui::Pointer EditorSink::pointerNow(const PointerEdge edge) const
    {
        const auto &mouse = folded.mouse();

        return ui::Pointer{
            .position = located
                ? std::optional<Point>{antwika::app::asPoint(
                      mouse.position())}
                : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = edge.pressed,
            .extends = edge.extends};
    }

    ui::Frame EditorSink::frameFor(
        const PointerEdge edge, const ui::Keyboard &keyboard) const
    {
        const PlaybackStatus status{
            .started = playback.started(),
            .voices = playback.voices(),
            .cycles = playback.playedTicks(),
            .lines = playback.sounding()};

        return scene.describe(
            state, score, status, canvas, pointerNow(edge), keyboard);
    }

    void EditorSink::scrollBy(const std::int32_t notches)
    {
        // Away from the user is up the document.
        // How far down it can usefully go is antwika::ui's answer.
        // One too far comes back clamped on the very next frame.
        const auto from = static_cast<std::int64_t>(state.scroll);
        const auto moved =
            from
            - static_cast<std::int64_t>(notches)
                  * static_cast<std::int64_t>(kLinesPerNotch);

        state.scroll =
            moved > 0 ? static_cast<std::size_t>(moved) : std::size_t{0};
    }

    void EditorSink::refreshAndAct(
        const PointerEdge edge, const ui::Keyboard &keyboard)
    {
        const auto frame = frameFor(edge, keyboard);

        // The focus never moves.
        // There is one thing to type into.
        // The scene names it to every frame's Context.
        const auto &acted = frame.interactions;

        bool changed = false;

        if (acted.activated == kPlayButton)
        {
            state.paused = !state.paused;
            changed = true;
        }
        else if (acted.activated == kPanicButton)
        {
            playback.silence();
        }
        else if (acted.activated == kLayoutBox)
        {
            state.layoutOpen = !state.layoutOpen;
            changed = true;
        }

        // This scene declares one list, so a choice is that list's.
        if (acted.chosen.has_value())
        {
            state.layout = static_cast<KeyLayout>(acted.chosen->index);
            state.layoutOpen = false;
            changed = true;
        }

        if (acted.scrolled.has_value())
        {
            applyScroll(state, *acted.scrolled);
            changed = true;
        }

        if (acted.edit.has_value())
        {
            applyEdit(state, *acted.edit);
            changed = true;
        }

        // A press inside the pane is what a later move drags from.
        if (edge.pressed)
        {
            state.dragging = acted.hovered == kCodeField;
        }

        if (!changed)
        {
            picture = frame.commands;

            return;
        }

        // Described again.
        // The picture above predates the changes just made.
        // With no keys and no press, so nothing happens twice.
        picture = frameFor(PointerEdge{}, ui::Keyboard{}).commands;
    }

    const ui::DrawList &EditorSink::commands() const noexcept
    {
        return picture;
    }

} // namespace antwika::music_editor
