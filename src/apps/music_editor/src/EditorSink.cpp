#include "antwika/music_editor/EditorSink.hpp"

#include <algorithm>
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
#include <antwika/engine/Events.hpp>

#include "antwika/music_editor/EditorKeys.hpp"
#include "antwika/music_editor/Events.hpp"
#include "antwika/music_editor/ScoreFiles.hpp"

namespace antwika::music_editor
{

    using antwika::app::isLeftRelease;
    using antwika::app::leftPress;
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
        constexpr std::size_t kLinesPerNotch = 3;

    }

    EditorSink::EditorSink(
        EditorState &state,
        Score &score,
        Playback &playback,
        const IInputEventCodec &codec,
        const EditorScene &scene,
        const Size canvas,
        WaveRenderDesc waveRender,
        std::optional<std::reference_wrapper<input::IClipboard>>
            clipboard,
        ITickEventSink &stop,
        std::string scoresDirectory,
        const bool writesScores)
        : state(state),
          score(score),
          playback(playback),
          codec(codec),
          scene(scene),
          canvas(canvas),
          clipboard(std::move(clipboard)),
          stop(stop),
          scoresDirectory(std::move(scoresDirectory)),
          writesScores(writesScores),
          waveRender(waveRender),
          waveImages(std::move(waveRender))
    {
    }

    void EditorSink::handle(const TickEvent &event)
    {
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            score.read(state.source);
            playback.step(state.paused);

            refreshAndAct(PointerEdge{}, ui::Keyboard{});

            return;
        }

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

        std::string characters;
        ui::Keyboard keyboard;

        if (const auto *key = std::get_if<KeyPressed>(&*decoded))
        {
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
            state.dragging = ui::DragHome::None;
        }

        keyboard.typed = characters;

        const auto *press = leftPress(*decoded);

        const bool moved =
            std::holds_alternative<antwika::input::PointerMoved>(
                *decoded);

        refreshAndAct(
            PointerEdge{
                .pressed = press != nullptr,
                .extends = press != nullptr
                    ? press->modifiers.shift
                    : moved
                          && state.dragging == ui::DragHome::Text},
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
        std::vector<ui::TextHighlight> playing;

        for (const auto &span : playback.highlights())
        {
            playing.push_back(
                ui::TextHighlight{.begin = span.begin, .end = span.end});
        }

        const auto &pick =
            kSpeeds[std::min(state.speed, kSpeeds.size() - 1)];

        const auto pace = waveRender.framesPerCycle
            / sequencer::Rational{pick.numerator, pick.denominator};

        const PlaybackStatus status{
            .started = playback.started(),
            .voices = playback.voices(),
            .cycles = playback.playedTicks(),
            .lines = playback.sounding(),
            .playing = std::move(playing),
            .position = playback.position(),
            .rate = waveRender.rate,
            .cycleFrames = pace.numerator() / pace.denominator(),
            .waves = waveImages.refresh(
                score, state.speed)}; // GCOVR_EXCL_LINE

        return scene.describe(
            state, score, status, canvas, pointerNow(edge), keyboard);
    }

    void EditorSink::scrollBy(const std::int32_t notches)
    {
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
        if (state.modal != Modal::None)
        {
            modalRefreshAndAct(edge, keyboard);

            return;
        }

        const auto frame = frameFor(edge, keyboard);

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
        else if (acted.activated == kSpeedBox)
        {
            state.speedOpen = !state.speedOpen;
            changed = true;
        }

        if (acted.chosen.has_value())
        {
            if (acted.chosen->dropdown == kMenuBox)
            {
                state.menuOpen = false;
                menuAction(acted.chosen->index);
            }
            else if (acted.chosen->dropdown == kSpeedBox)
            {
                state.speedOpen = false;
                speedAction(acted.chosen->index);
            }
            else
            {
                state.layout =
                    static_cast<KeyLayout>(acted.chosen->index);
                state.layoutOpen = false;
            }

            changed = true;
        }

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

        if (edge.pressed)
        {
            state.dragging = acted.areaPress.has_value()
                                 ? acted.areaPress->home
                                 : ui::DragHome::None;
        }

        if (!changed)
        {
            picture = frame.commands;

            return;
        }

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

        mirrorClipboard();
    }

    void EditorSink::modalRefreshAndAct(
        const PointerEdge edge, const ui::Keyboard &keyboard)
    {
        const auto page = frameFor(PointerEdge{}, ui::Keyboard{});

        const auto frame = scene.describeModal(
            state, canvas, pointerNow(edge), keyboard);

        const auto &acted = frame.interactions;

        bool changed = false;

        if (acted.edit.has_value())
        {
            state.fileName = acted.edit->text;
            state.fileCursor = acted.edit->cursor;
            changed = true;

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

        if (edge.pressed)
        {
            state.dragging = ui::DragHome::None;
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
            stop.handle(TickEvent{ // GCOVR_EXCL_LINE
                .tick = foldedTick,
                .event = {.name = antwika::engine::events::kStop}});
        }
    }

    void EditorSink::speedAction(const std::size_t index)
    {
        if (index == state.speed)
        {
            return;
        }

        state.speed = index;

        const auto &pick = kSpeeds[index];

        playback.setSpeed(
            sequencer::Rational{pick.numerator, pick.denominator});
    }

    void EditorSink::saveNow()
    {
        const auto name = safeScoreName(state.fileName);

        if (name.empty())
        {
            state.notice = "name it first";

            return;
        }

        if (writesScores)
        {
            try
            {
                saveScore(
                    scorePath(scoresDirectory, name), state.source);
            }
            catch (const ScoreFileError &failed) // GCOVR_EXCL_LINE
            {
                state.notice = failed.what();

                return;
            }
        }

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

    void EditorSink::mirrorClipboard()
    {
        if (clipboard.has_value() && state.clipboard != mirrored)
        {
            mirrored = state.clipboard;
            clipboard->get().setText(mirrored);
        }
    }

    const ui::DrawList &EditorSink::commands() const noexcept
    {
        return picture;
    }

}
