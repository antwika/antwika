#include "antwika/music_editor/EditorScene.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Hap.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Span.hpp>
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

#include "antwika/music_editor/TrackPreset.hpp"

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

        constexpr std::size_t kProblemsShown = 3;

        constexpr std::array<std::string_view, 2> kLayouts{
            "swedish", "english"};

        constexpr std::array<std::string_view, 4> kMenuItems{
            "new", "save", "load", "quit"};

        constexpr auto kSpeedLabels = []
        {
            std::array<std::string_view, kSpeeds.size()> labels{};

            for (std::size_t at = 0; at < kSpeeds.size(); ++at)
            {
                labels[at] = kSpeeds[at].label;
            }

            return labels;
        }();

        constexpr Color kScrim{
            .red = 6, .green = 8, .blue = 12, .alpha = 190};

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

        using antwika::gfx::Rect;
        using antwika::pattern::Cycle;
        using antwika::pattern::Hap;
        using antwika::pattern::Span;

        constexpr Color kRollBackdrop{
            .red = 12, .green = 13, .blue = 18, .alpha = 255};

        constexpr Color kRollPlaying{
            .red = 235, .green = 245, .blue = 160, .alpha = 255};

        constexpr Color kRollNow{
            .red = 225, .green = 230, .blue = 235, .alpha = 255};

        struct RollNote final
        {
            std::int64_t pitch = 0;

            Cycle begin{};

            Cycle length{};
        };

        [[nodiscard]] std::vector<RollNote> rollNotesOf(
            const antwika::pattern::Pattern &playing, const Cycle &from)
        {
            std::vector<RollNote> notes;

            try
            {
                for (const auto &hap : playing.queryAll(
                         Span{from - Cycle(1), from + Cycle(1)}))
                {
                    if (!hap.hasOnset())
                    {
                        continue;
                    }

                    const auto note =
                        hap.value.get(kNote)
                            .value_or(pattern::ParamValue{});

                    const auto span = hap.whole.value_or(hap.part);

                    notes.push_back(RollNote{
                        .pitch =
                            note.raw() >> pattern::kFractionBits,
                        .begin = span.begin(),
                        .length = span.length()});
                }
            }
            catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
            {
                notes.clear();
            }

            return notes;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::int64_t acrossRoll(
            const Cycle &at, const std::uint32_t width) noexcept
        {
            return at.numerator()
                * static_cast<std::int64_t>(width)
                / at.denominator();
        }

        void appendRoll(
            DrawList &commands,
            const Rect &band,
            const Pianoroll &roll,
            const PlaybackStatus &status)
        {
            const auto width =
                std::min(band.size.width, kPianorollWidth);

            commands.push_back(antwika::ui::FillRect{
                .rect = Rect{
                    .origin = band.origin,
                    .size = {
                        .width = width,
                        .height = band.size.height}},
                .color = kRollBackdrop});

            const Cycle lookback{1, 4};

            const auto from = status.position - lookback;

            const auto now = acrossRoll(lookback, width);

            const auto notes = rollNotesOf(roll.playing, from);

            if (!notes.empty())
            {
                auto lowest = notes.front().pitch;
                auto highest = notes.front().pitch;

                for (const auto &note : notes)
                {
                    lowest = std::min(lowest, note.pitch);
                    highest = std::max(highest, note.pitch);
                }

                const auto lanes = static_cast<std::uint64_t>(
                    highest - lowest + 1);

                const auto laneHeight = std::max<std::uint32_t>(
                    1,
                    static_cast<std::uint32_t>(
                        std::uint64_t{band.size.height} / lanes));

                for (const auto &note : notes)
                {
                    const auto lane = static_cast<std::uint32_t>(
                        note.pitch - lowest);

                    const auto below =
                        std::uint64_t{lane + 1} * laneHeight;

                    if (below > band.size.height)
                    {
                        continue;
                    }

                    const auto begun =
                        acrossRoll(note.begin - from, width);

                    auto over = acrossRoll(
                        note.begin + note.length - from, width);

                    if (status.rate > 0 && status.cycleFrames > 0)
                    {
                        const auto length = note.length.numerator()
                            * status.cycleFrames
                            / note.length.denominator();

                        const auto held = std::min<std::int64_t>(
                            length,
                            static_cast<std::int64_t>(
                                roll.preset.maxHoldMs)
                                * status.rate / 1000);

                        const auto rings = held
                            + static_cast<std::int64_t>(
                                  roll.preset.releaseMs)
                                * status.rate / 1000;

                        over = begun
                            + rings * width / status.cycleFrames;
                    }

                    if (over <= 0)
                    {
                        continue;
                    }

                    const bool playing = begun <= now && now < over;

                    const auto left =
                        std::max<std::int64_t>(begun, 0);
                    const auto right =
                        std::min<std::int64_t>(over, width);

                    commands.push_back(antwika::ui::FillRect{
                        .rect = Rect{
                            .origin =
                                {.x = band.origin.x
                                      + static_cast<std::int32_t>(
                                          left),
                                 .y = band.origin.y
                                      + static_cast<std::int32_t>(
                                          band.size.height - below)},
                            .size =
                                {.width = static_cast<std::uint32_t>(
                                     std::max<std::int64_t>(
                                         1, right - left)),
                                 .height = laneHeight}},
                        .color = playing ? kRollPlaying : kCalm});
                }
            }

            commands.push_back(antwika::ui::FillRect{
                .rect = Rect{
                    .origin =
                        {.x = band.origin.x
                              + static_cast<std::int32_t>(now),
                         .y = band.origin.y},
                    .size = {
                        .width = 2,
                        .height = band.size.height}},
                .color = kRollNow});
        }

        constexpr Color kWaveInk{
            .red = 110, .green = 170, .blue = 235, .alpha = 255};

        void appendWave(
            DrawList &commands, const Rect &band, const WaveImage &image)
        {
            const auto columns = image.low.size();

            if (columns == 0)
            {
                return;
            }

            const auto half =
                static_cast<std::int32_t>(band.size.height / 2);
            const auto mid = band.origin.y + half;

            for (std::uint32_t x = 0; x < band.size.width; ++x)
            {
                const auto column =
                    static_cast<std::size_t>(x) * columns
                    / band.size.width;

                const auto top = mid
                    - static_cast<std::int32_t>(
                        image.high[column]
                        * static_cast<float>(half));

                const auto bottom = mid
                    - static_cast<std::int32_t>(
                        image.low[column]
                        * static_cast<float>(half));

                commands.push_back(antwika::ui::FillRect{
                    .rect = Rect{
                        .origin =
                            {.x = band.origin.x
                                  + static_cast<std::int32_t>(x),
                             .y = top},
                        .size =
                            {.width = 1,
                             .height = static_cast<std::uint32_t>(
                                 std::max(1, bottom - top))}},
                    .color = kWaveInk});
            }
        }

        void appendWaveforms(
            Frame &frame,
            const Score &score,
            const PlaybackStatus &status)
        {
            const auto height =
                kWaveformRows * antwika::gfx::kGlyphLineHeight
                * kTextScale;

            for (std::size_t at = 0; at < score.waveforms().size();
                 ++at)
            {
                const auto band = frame.rects.find(waveformBand(at));

                if (!band.has_value() || band->size.height < height)
                {
                    continue;
                }

                frame.commands.push_back(antwika::ui::FillRect{
                    .rect = *band, .color = kRollBackdrop});

                if (at < status.waves.size())
                {
                    appendWave(
                        frame.commands, *band, status.waves[at]);
                }
            }
        }

        void appendPianorolls(
            Frame &frame,
            const Score &score,
            const PlaybackStatus &status)
        {
            const auto height =
                kPianorollRows * antwika::gfx::kGlyphLineHeight
                * kTextScale;

            for (std::size_t at = 0; at < score.pianorolls().size();
                 ++at)
            {
                const auto band = frame.rects.find(pianorollBand(at));

                if (!band.has_value() || band->size.height < height)
                {
                    continue;
                }

                appendRoll(
                    frame.commands,
                    *band,
                    score.pianorolls()[at],
                    status);
            }
        }
    }

    antwika::ui::Theme editorTheme() noexcept
    {
        Theme theme;

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
        std::vector<antwika::ui::LineBand> bands;
        bands.reserve(
            score.pianorolls().size() + score.waveforms().size());

        for (std::size_t at = 0; at < score.pianorolls().size(); ++at)
        {
            bands.push_back(antwika::ui::LineBand{
                .line = score.pianorolls()[at].line,
                .rows = kPianorollRows,
                .id = pianorollBand(at)});
        }

        for (std::size_t at = 0; at < score.waveforms().size(); ++at)
        {
            bands.push_back(antwika::ui::LineBand{
                .line = score.waveforms()[at].line,
                .rows = kWaveformRows,
                .id = waveformBand(at)});
        }

        Context ui(
            canvas, editorTheme(), pointer, keyboard, kCodeField);

        {
            const auto page =
                ui.panel(ContainerSpec{.width = kGrow, .height = kGrow});

            ui.label("antwika music editor");
            ui.label(
                statusLine(state, status),
                state.paused ? kTrouble : kCalm);

            {
                const auto hints = ui.row();

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

                const DropdownSpec speed{
                    .id = kSpeedBox,
                    .optionIdBase = kSpeedOptions,
                    .options = kSpeedLabels,
                    .selected = state.speed,
                    .open = state.speedOpen};

                ui.dropdown(speed);

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
                    .highlights = status.playing,
                    .bands = bands,
                    .scrollbar = true,
                    .focused = true,
                    .dragging = state.dragging});

            const auto &problems = score.problems();

            for (std::size_t shown = 0;
                 shown < problems.size() && shown < kProblemsShown;
                 ++shown)
            {
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

        auto frame = ui.finish();

        appendPianorolls(frame, score, status);
        appendWaveforms(frame, score, status);

        return frame;
    }

    Frame EditorScene::describeModal(
        const EditorState &state,
        const Size canvas,
        const Pointer pointer,
        const Keyboard &keyboard) const
    {
        Context ui(
            canvas,
            editorTheme(),
            pointer,
            keyboard,
            state.modal == Modal::Save ? kSaveNameField
                                       : antwika::ui::kNoWidget);

        {
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

}
