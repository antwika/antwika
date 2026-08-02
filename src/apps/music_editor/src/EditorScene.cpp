#include "antwika/music_editor/EditorScene.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Scope.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::music_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::ui::Alignment;
        using antwika::ui::ButtonSpec;
        using antwika::ui::ContainerSpec;
        using antwika::ui::Context;
        using antwika::ui::DropdownSpec;
        using antwika::ui::fixedSize;
        using antwika::ui::kFit;
        using antwika::ui::kGrow;
        using antwika::ui::TextAreaSpec;
        using antwika::ui::TextFieldSpec;
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

        // In the order KeyLayout names them.
        // Which one is selected is that enumeration's own number.
        // The Context borrows these, so they outlive every frame.
        constexpr std::array<std::string_view, 2> kLayouts{
            "swedish", "english"};

        // In the order EditorSink acts on them, by index.
        // The Context borrows these too.
        constexpr std::array<std::string_view, 4> kMenuItems{
            "new", "save", "load", "quit"};

        // Dark and translucent, so the score reads as still there.
        // apps/game's modal makes the same argument about its city.
        constexpr Color kScrim{
            .red = 6, .green = 8, .blue = 12, .alpha = 190};

        // Wide enough for a name worth typing and a list worth reading.
        constexpr std::uint32_t kCardWidth = 500;

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
            // The whole window rather than what the score fills.
            // The pane inside it is what scrolls.
            // One growing with the document would never need to.
            const auto page =
                ui.panel(ContainerSpec{.width = kGrow, .height = kGrow});

            ui.label("antwika music editor");
            ui.label(
                statusLine(state, status),
                state.paused ? kTrouble : kCalm);

            {
                // The one thing here that is a setting.
                // Which board the characters are read off.
                const auto hints = ui.row();

                // The commands, left of everything.
                // Nothing is ever selected: the box always says menu.
                const DropdownSpec menu{
                    .id = kMenuBox,
                    .optionIdBase = kMenuOptions,
                    .options = kMenuItems,
                    .placeholder = "menu",
                    .open = state.menuOpen};

                ui.dropdown(menu);

                ui.label(
                    "esc pauses, enter is a new line, tab indents, "
                    "f10 fills the screen");

                ui.spacer(kGrow);

                // Named rather than written into the call.
                // A temporary of it carries a cleanup block.
                // Nothing ever enters that, and the gate counts it.
                const DropdownSpec box{
                    .id = kLayoutBox,
                    .optionIdBase = kLayoutOptions,
                    .options = kLayouts,
                    .selected = static_cast<std::size_t>(state.layout),
                    .open = state.layoutOpen};

                ui.dropdown(box);
            }

            ui.textArea(
                TextAreaSpec{
                    .id = kCodeField,
                    .width = kGrow,
                    .height = kGrow,
                    .text = state.source,
                    .placeholder = "$: bass.n(\"0 ~ 0 [~ 3]\")",
                    .cursor = state.cursor,
                    .anchor = state.anchor,
                    .scroll = state.scroll,
                    // The notes sounding right now, on their own ground.
                    .highlights = status.playing,
                    .scrollbar = true,
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

    Frame EditorScene::describeModal(
        const EditorState &state,
        const Size canvas,
        const Pointer pointer,
        const Keyboard &keyboard) const
    {
        // The save box types into its field, so that has the focus.
        // The load box is buttons alone and asks for none.
        Context ui(
            canvas,
            editorTheme(),
            pointer,
            keyboard,
            state.modal == Modal::Save ? kSaveNameField
                                       : antwika::ui::kNoWidget);

        {
            // The scrim is the whole canvas, and it is filled.
            // That fill is what keeps a press off the pane beneath.
            const auto screen = ui.panel(ContainerSpec{
                .width = kGrow,
                .height = kGrow,
                .cross = Alignment::Center,
                .background = kScrim});

            ui.spacer(kGrow);

            {
                const auto card = ui.panel(ContainerSpec{
                    .width = fixedSize(kCardWidth), .height = kFit});

                if (state.modal == Modal::Save)
                {
                    ui.label("save the score as");

                    ui.textField(TextFieldSpec{
                        .id = kSaveNameField,
                        .width = kGrow,
                        .text = state.fileName,
                        .placeholder = "score-name",
                        .cursor = state.fileCursor,
                        .focused = true});
                }
                else
                {
                    ui.label("load a score");

                    for (std::size_t at = 0; at < state.scores.size();
                         ++at)
                    {
                        ui.button(
                            state.scores[at],
                            ButtonSpec{
                                .id = loadOption(at), .width = kGrow});
                    }

                    if (state.scores.empty())
                    {
                        ui.label("nothing saved yet");
                    }
                }

                if (!state.notice.empty())
                {
                    // Its own ink, so it is not mistaken for a label.
                    ui.label(state.notice, kTrouble);
                }

                const auto controls = ui.row();

                if (state.modal == Modal::Save)
                {
                    ui.button("save", ButtonSpec{.id = kSaveConfirm});
                }

                ui.button("cancel", ButtonSpec{.id = kModalCancel});
            }

            ui.spacer(kGrow);
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
