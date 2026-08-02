#include "antwika/music_editor/EditorScene.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Scope.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::music_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::ui::ButtonSpec;
        using antwika::ui::Context;
        using antwika::ui::kGrow;
        using antwika::ui::TextFieldSpec;
        using antwika::ui::Theme;

        constexpr Color kBackdrop{
            .red = 18, .green = 18, .blue = 24, .alpha = 255};

        constexpr Color kTrouble{
            .red = 235, .green = 110, .blue = 110, .alpha = 255};

        constexpr Color kCalm{
            .red = 130, .green = 205, .blue = 140, .alpha = 255};

        constexpr std::array<std::string_view, kTrackCount> kNames{
            "bass", "lead", "bell", "drum"};

        [[nodiscard]] std::string statusLine(
            const EditorState &state, const PlaybackStatus &status)
        {
            return std::string(state.paused ? "paused" : "playing")
                + "  cycle " + std::to_string(status.cycles)
                + "  voices " + std::to_string(status.voices)
                + "  notes " + std::to_string(status.started);
        }
    } // namespace

    std::string_view trackName(const std::size_t track) noexcept
    {
        return kNames[track];
    }

    Frame EditorScene::describe(
        const EditorState &state,
        const Score &score,
        const PlaybackStatus &status,
        const Size canvas,
        const Pointer pointer,
        const Keyboard &keyboard) const
    {
        // The focused field is named to the Context.
        // The typing goes where the state says it does.
        Context ui(
            canvas,
            Theme{},
            pointer,
            keyboard,
            fieldFor(state.focused));

        // Every Scope is closed before finish().
        // It refuses to lay out a half-built tree.
        {
            const auto page = ui.panel();

            ui.label("antwika music editor");
            ui.label(
                statusLine(state, status),
                state.paused ? kTrouble : kCalm);
            ui.label(
                "tab moves between lines, enter pauses, type to edit");

            for (std::size_t track = 0; track < kTrackCount; ++track)
            {
                const auto line = ui.row();

                ui.label(trackName(track));

                ui.textField(
                    TextFieldSpec{
                        .id = fieldFor(track),
                        .width = kGrow,
                        .text = state.lines[track],
                        .placeholder = "silent",
                        .cursor = track == state.focused
                            ? state.cursor
                            : ui::kCaretAtEnd,
                        .focused = track == state.focused});
            }

            for (std::size_t track = 0; track < kTrackCount; ++track)
            {
                if (score.error(track).empty())
                {
                    continue;
                }

                // Named so a reader knows which line is refused.
                // Drawn in its own ink so it is not mistaken for music.
                ui.label(
                    std::string(trackName(track)) + ": "
                        + score.error(track),
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
