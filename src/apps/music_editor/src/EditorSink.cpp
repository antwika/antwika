#include "antwika/music_editor/EditorSink.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
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
            // The lines are re-read from what the input just did.
            // Only then is the sound advanced.
            // A note decided now reads the line as it now stands.
            score.update(state.lines);
            playback.step(state.paused);

            refreshAndAct(false, ui::Keyboard{});

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
            const auto meaning = uiKeyFor(key->key, key->modifiers.shift);

            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            const char typed =
                typedCharacterFor(key->key, key->modifiers.shift);

            if (typed != '\0')
            {
                characters.push_back(typed);
            }
        }

        keyboard.typed = characters;

        refreshAndAct(isLeftPress(*decoded), keyboard);
    }

    ui::Pointer EditorSink::pointerNow(const bool pressed) const
    {
        const auto &mouse = folded.mouse();

        return ui::Pointer{
            .position = located
                ? std::optional<Point>{antwika::app::asPoint(
                      mouse.position())}
                : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void EditorSink::refreshAndAct(
        const bool pressed, const ui::Keyboard &keyboard)
    {
        const PlaybackStatus status{
            .started = playback.started(),
            .voices = playback.voices(),
            .cycles = playback.playedTicks()};

        const auto frame = scene.describe(
            state, score, status, canvas, pointerNow(pressed), keyboard);

        picture = frame.commands;

        const auto &acted = frame.interactions;

        // Tab moved the focus, and antwika::ui worked out where to.
        // No guard for kNoWidget is needed here.
        // focusWidget ignores anything that is not a field.
        (void)focusWidget(state, acted.focused);

        if (acted.activated == kPlayButton)
        {
            state.paused = !state.paused;
        }
        else if (acted.activated == kPanicButton)
        {
            playback.silence();
        }
        else
        {
            (void)focusWidget(state, acted.activated);
        }

        if (!acted.edit.has_value())
        {
            return;
        }

        // Enter is the only key here that is not a character.
        // It is the one thing a line hears that is not more music.
        if (acted.edit->submitted)
        {
            state.paused = !state.paused;
        }

        applyEdit(state, *acted.edit);
    }

    const ui::DrawList &EditorSink::commands() const noexcept
    {
        return picture;
    }

} // namespace antwika::music_editor
