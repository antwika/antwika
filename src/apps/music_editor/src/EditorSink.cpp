#include "antwika/music_editor/EditorSink.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/Frame.hpp>

#include "antwika/music_editor/EditorKeys.hpp"
#include "antwika/music_editor/Events.hpp"
#include "antwika/music_editor/ScoreFiles.hpp"

#include <antwika/engine/Events.hpp>

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
        const Size canvas,
        input::IClipboard *clipboard,
        ITickEventSink &stop,
        std::string scoresDirectory)
        : state(state),
          score(score),
          playback(playback),
          codec(codec),
          scene(scene),
          canvas(canvas),
          clipboard(clipboard),
          stop(stop),
          scoresDirectory(std::move(scoresDirectory))
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

        // A paste is the one event of this application's own.
        // The clipboard was read upstream; this only types the answer.
        if (event.event.name == events::kPaste)
        {
            ui::Keyboard keyboard;

            for (std::size_t at = 0; at < event.event.payload.size();
                 ++at)
            {
                keyboard.keys.push_back(ui::Key::Character);
            }

            keyboard.typed = event.event.payload;

            refreshAndAct(PointerEdge{}, keyboard);

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
            // Over a modal it is the modal's way out instead.
            if (key->key == antwika::input::Key::Escape)
            {
                if (state.modal != Modal::None)
                {
                    state.modal = Modal::None;
                    state.notice.clear();
                }
                else
                {
                    state.paused = !state.paused;
                }
            }

            const auto meaning = uiKeyFor(key->key, key->modifiers);

            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            characters =
                typedTextFor(key->key, key->modifiers, state.layout);

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
        // The sounding notes' spans, as the pane's own highlights.
        std::vector<ui::TextHighlight> playing;

        for (const auto &span : playback.highlights())
        {
            playing.push_back(
                ui::TextHighlight{.begin = span.begin, .end = span.end});
        }

        const PlaybackStatus status{
            .started = playback.started(),
            .voices = playback.voices(),
            .cycles = playback.playedTicks(),
            .lines = playback.sounding(),
            .playing = std::move(playing)};

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
        // A box over the editor takes the whole tick's input.
        if (state.modal != Modal::None)
        {
            modalRefreshAndAct(edge, keyboard);

            return;
        }

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
        else if (acted.activated == kMenuBox)
        {
            state.menuOpen = !state.menuOpen;
            changed = true;
        }

        // Two lists, told apart by the box a choice names.
        if (acted.chosen.has_value())
        {
            if (acted.chosen->dropdown == kMenuBox)
            {
                state.menuOpen = false;
                menuAction(acted.chosen->index);
            }
            else
            {
                state.layout =
                    static_cast<KeyLayout>(acted.chosen->index);
                state.layoutOpen = false;
            }

            changed = true;
        }

        // An option's press is the option's alone.
        // What fell through its overlay onto the pane is not applied.
        if (!acted.chosen.has_value() && acted.scrolled.has_value())
        {
            applyScroll(state, *acted.scrolled);
            changed = true;
        }

        if (!acted.chosen.has_value() && acted.edit.has_value())
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

        // A menu choice may have raised a box over the fresh page.
        if (state.modal != Modal::None)
        {
            const auto box = scene.describeModal(
                state, canvas, ui::Pointer{}, ui::Keyboard{});

            picture.insert(
                picture.end(),
                box.commands.begin(),
                box.commands.end());
        }

        mirrorClipboard();
    }

    void EditorSink::modalRefreshAndAct(
        const PointerEdge edge, const ui::Keyboard &keyboard)
    {
        // The editor below is described with no input at all.
        // So nothing under the box is typed into or pressed.
        const auto page = frameFor(PointerEdge{}, ui::Keyboard{});

        const auto frame = scene.describeModal(
            state, canvas, pointerNow(edge), keyboard);

        const auto &acted = frame.interactions;

        bool changed = false;

        // The name field is the one editable thing a box holds.
        // So an edit here can name nothing else.
        if (acted.edit.has_value())
        {
            state.fileName = acted.edit->text;
            state.fileCursor = acted.edit->cursor;
            changed = true;

            // Enter in the field is its submit, so it saves.
            if (acted.edit->submitted)
            {
                saveNow();
            }
        }

        if (acted.activated == kSaveConfirm)
        {
            saveNow();
            changed = true;
        }
        else if (acted.activated == kModalCancel)
        {
            state.modal = Modal::None;
            state.notice.clear();
            changed = true;
        }
        else
        {
            // The score buttons, compared one by one.
            // A loop rather than id arithmetic.
            // Every id the box reports is one this frame declared.
            for (std::size_t at = 0; at < state.scores.size(); ++at)
            {
                if (acted.activated == loadOption(at))
                {
                    loadNow(at);
                    changed = true;

                    break;
                }
            }
        }

        // A press outside the pane leaves a drag disarmed.
        if (edge.pressed)
        {
            state.dragging = false;
        }

        if (!changed)
        {
            picture = page.commands;
            picture.insert(
                picture.end(),
                frame.commands.begin(),
                frame.commands.end());

            return;
        }

        // Described again, page and box both.
        // A load rewrote the page, and a save may have closed the box.
        picture = frameFor(PointerEdge{}, ui::Keyboard{}).commands;

        if (state.modal != Modal::None)
        {
            const auto box = scene.describeModal(
                state, canvas, ui::Pointer{}, ui::Keyboard{});

            picture.insert(
                picture.end(),
                box.commands.begin(),
                box.commands.end());
        }
    }

    void EditorSink::menuAction(const std::size_t index)
    {
        if (index == 0)
        {
            // New: an empty page, still playing nothing until typed.
            state.source.clear();
            state.cursor = 0;
            state.anchor.reset();
            state.scroll = 0;
        }
        else if (index == 1)
        {
            state.modal = Modal::Save;
            state.notice.clear();
        }
        else if (index == 2)
        {
            state.modal = Modal::Load;
            state.notice.clear();
        }
        else
        {
            // The loop's own signal rather than an event on the wire.
            // The recording holds the click, and the stop follows.
            // apps/game's main menu quits the very same way.
            // The excluded line's remaining branches are allocator's.
            // The event's name is a literal too short to reach a heap.
            // The rest are the unwind edges beside that.
            stop.handle(TickEvent{ // GCOVR_EXCL_LINE
                .tick = foldedTick,
                .event = {.name = antwika::engine::events::kStop}});
        }
    }

    void EditorSink::saveNow()
    {
        const auto name = safeScoreName(state.fileName);

        if (name.empty())
        {
            state.notice = "name it first";

            return;
        }

        try
        {
            saveScore(scorePath(scoresDirectory, name), state.source);
        }
        // The excluded line is the catch chain's dispatch.
        // Its one unexercised edge is a second kind of exception.
        // Nothing under saveScore() throws anything else.
        catch (const ScoreFileError &failed) // GCOVR_EXCL_LINE
        {
            state.notice = failed.what();

            return;
        }

        // Added rather than re-listed.
        // A directory read inside the tick path would not replay.
        addScore(state, name);

        state.modal = Modal::None;
        state.notice.clear();
    }

    void EditorSink::loadNow(const std::size_t at)
    {
        std::string text;

        try
        {
            text = loadScore(
                scorePath(scoresDirectory, state.scores[at]));
        }
        // Likewise, and for the same reason.
        catch (const ScoreFileError &failed) // GCOVR_EXCL_LINE
        {
            state.notice = failed.what();

            return;
        }

        state.source = std::move(text);
        state.cursor = 0;
        state.anchor.reset();
        state.scroll = 0;
        state.modal = Modal::None;
        state.notice.clear();
    }

    // A copy is mirrored outward, so other programs can paste it.
    // On changes alone, or every tick would touch the clipboard.
    void EditorSink::mirrorClipboard()
    {
        if (clipboard != nullptr && state.clipboard != mirrored)
        {
            mirrored = state.clipboard;
            clipboard->setText(mirrored);
        }
    }

    const ui::DrawList &EditorSink::commands() const noexcept
    {
        return picture;
    }

} // namespace antwika::music_editor
