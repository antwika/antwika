#include "antwika/music_editor/EditorScene.hpp"

#include <cstddef>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Scope.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::music_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::ui::ButtonSpec;
        using antwika::ui::Context;
        using antwika::ui::kGrow;
        using antwika::ui::TextAreaSpec;
        using antwika::ui::Theme;

        constexpr Color kBackdrop{
            .red = 18, .green = 18, .blue = 24, .alpha = 255};

        constexpr Color kTrouble{
            .red = 235, .green = 110, .blue = 110, .alpha = 255};

        constexpr Color kCalm{
            .red = 130, .green = 205, .blue = 140, .alpha = 255};

        // How many refusals are shown at once.
        // A document with more says so.
        // Every one of them would push the code off the window.
        constexpr std::size_t kProblemsShown = 3;

        [[nodiscard]] std::string statusLine(
            const EditorState &state, const PlaybackStatus &status)
        {
            return std::string(state.paused ? "paused" : "playing")
                + "  cycle " + std::to_string(status.cycles)
                + "  lines " + std::to_string(status.lines)
                + "  voices " + std::to_string(status.voices)
                + "  notes " + std::to_string(status.started);
        }

        [[nodiscard]] std::string problemLine(const Problem &problem)
        {
            return "line " + std::to_string(problem.line) + ": "
                + problem.message;
        }
    } // namespace

    antwika::ui::Theme editorTheme() noexcept
    {
        Theme theme;

        // The text alone, rather than scaledTheme()'s every metric.
        // A doubled inset would spend the window on its own margins.
        theme.textScale = kTextScale;

        return theme;
    }

    Frame EditorScene::describe(
        const EditorState &state,
        const Score &score,
        const PlaybackStatus &status,
        const Size canvas,
        const Pointer pointer,
        const Keyboard &keyboard) const
    {
        // There is one thing to type into, and it always has the focus.
        // So no press moves it and no key has to.
        Context ui(
            canvas, editorTheme(), pointer, keyboard, kCodeField);

        // Every Scope is closed before finish().
        // It refuses to lay out a half-built tree.
        {
            const auto page = ui.panel();

            ui.label("antwika music editor");
            ui.label(
                statusLine(state, status),
                state.paused ? kTrouble : kCalm);
            ui.label(
                "esc pauses, enter is a new line, tab indents");

            ui.textArea(
                TextAreaSpec{
                    .id = kCodeField,
                    .width = kGrow,
                    .height = kGrow,
                    .text = state.source,
                    .placeholder = "$: bass.n(\"0 ~ 0 [~ 3]\")",
                    .cursor = state.cursor,
                    .focused = true});

            const auto &problems = score.problems();

            for (std::size_t shown = 0;
                 shown < problems.size() && shown < kProblemsShown;
                 ++shown)
            {
                // Named by line so a reader knows which one is refused.
                // Drawn in its own ink so it is not mistaken for music.
                ui.label(problemLine(problems[shown]), kTrouble);
            }

            if (problems.size() > kProblemsShown)
            {
                ui.label(
                    std::to_string(problems.size() - kProblemsShown)
                        + " more",
                    kTrouble);
            }

            const auto controls = ui.row();

            ui.button(
                state.paused ? "resume" : "pause",
                ButtonSpec{.id = kPlayButton});

            ui.button("silence", ButtonSpec{.id = kPanicButton});
        }

        return ui.finish();
    }

    void EditorScene::draw(
        IRenderer &renderer, const DrawList &picture) const
    {
        renderer.clear(kBackdrop);
        antwika::ui::paint(renderer, picture);
    }

} // namespace antwika::music_editor
