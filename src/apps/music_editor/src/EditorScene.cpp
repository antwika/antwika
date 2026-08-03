#include "antwika/music_editor/EditorScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
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

        // The speeds, as the box's option labels.
        // Read off the one table the sink acts on, so they agree.
        constexpr auto kSpeedLabels = []
        {
            std::array<std::string_view, kSpeeds.size()> labels{};

            for (std::size_t at = 0; at < kSpeeds.size(); ++at)
            {
                labels[at] = kSpeeds[at].label;
            }

            return labels;
        }();

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

        using antwika::gfx::Rect;
        using antwika::pattern::Cycle;
        using antwika::pattern::Hap;
        using antwika::pattern::Span;

        // The roll's own ground, darker than the pane it sits in.
        constexpr Color kRollBackdrop{
            .red = 12, .green = 13, .blue = 18, .alpha = 255};

        /**
         * @brief One note of a roll, before any pixel is known.
         *
         * The pitch in whole semitones, and where the query saw it
         * inside the cycle -- exact fractions, scaled to pixels only
         * once the band's rectangle is at hand.
         */
        struct RollNote
        {
            std::int64_t pitch = 0;
            Cycle begin{};
            Cycle end{};
        };

        [[nodiscard]] std::vector<RollNote> rollNotesOf(
            const antwika::pattern::Pattern &playing)
        {
            std::vector<Hap> haps;

            try
            {
                haps = playing.queryAll(
                    Span{Cycle(0), Cycle(1)});
            }
            // The excluded line is the handler's no-match edge.
            // Only some other exception type would take it.
            // See docs/confirming-unreachable-branches.md.
            catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
            {
                // A pattern can parse and still refuse a window.
                // The roll comes out empty, as the sound falls silent.
                return {};
            }

            std::vector<RollNote> notes;
            notes.reserve(haps.size());

            for (const auto &hap : haps)
            {
                // Whole semitones, floored, as the lanes are drawn.
                // The stored form is fixed point, so this is a shift.
                const auto note = hap.value.get(kNote)
                                      .value_or(pattern::ParamValue{});

                notes.push_back(RollNote{
                    .pitch = note.raw() >> pattern::kFractionBits,
                    .begin = hap.part.begin(),
                    .end = hap.part.end()});
            }

            return notes;
            // Only an unwind destroys notes at this brace.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Get how far along a rectangle a cycle position is.
         *
         * Exact fraction in, whole pixel out, in 64-bit arithmetic:
         * a position inside one cycle times a pane's width stays far
         * inside the range.
         *
         * @param at The position, inside cycle nought.
         * @param width What one whole cycle scales to.
         * @return The pixels, rounded down.
         */
        [[nodiscard]] std::int64_t acrossRoll(
            const Cycle &at, const std::uint32_t width) noexcept
        {
            return at.numerator()
                * static_cast<std::int64_t>(width)
                / at.denominator();
        }

        /**
         * @brief Append one pianoroll's picture over its band.
         *
         * A backdrop, and a cell per note: time runs across the band
         * as one cycle, and each distinct semitone is a lane, lowest
         * at the bottom.  Appended after the frame's own commands,
         * which paints it over the empty room the band held open.
         *
         * @param commands The picture to append to.
         * @param band Where the band ended up.
         * @param playing What the line plays.
         */
        void appendRoll(
            DrawList &commands,
            const Rect &band,
            const antwika::pattern::Pattern &playing)
        {
            commands.push_back(antwika::ui::FillRect{
                .rect = band, .color = kRollBackdrop});

            const auto notes = rollNotesOf(playing);

            if (notes.empty())
            {
                return;
            }

            auto lowest = notes.front().pitch;
            auto highest = notes.front().pitch;

            for (const auto &note : notes)
            {
                lowest = std::min(lowest, note.pitch);
                highest = std::max(highest, note.pitch);
            }

            // Never zero, and never wider than the band is tall.
            // A range of a thousand semitones gets one-pixel lanes.
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

                // Lanes past the band's top are not drawn.
                // A one-pixel lane cannot hold a thousand of them.
                const auto below = std::uint64_t{lane + 1} * laneHeight;

                if (below > band.size.height)
                {
                    continue;
                }

                const auto left =
                    acrossRoll(note.begin, band.size.width);
                const auto right =
                    acrossRoll(note.end, band.size.width);

                commands.push_back(antwika::ui::FillRect{
                    .rect = Rect{
                        .origin =
                            {.x = band.origin.x
                                  + static_cast<std::int32_t>(left),
                             .y = band.origin.y
                                  + static_cast<std::int32_t>(
                                      band.size.height - below)},
                        .size =
                            {.width = static_cast<std::uint32_t>(
                                 std::max<std::int64_t>(
                                     1, right - left)),
                             .height = laneHeight}},
                    .color = kCalm});
            }
        }

        using antwika::synth::Waveshape;

        // What a wave is drawn in, apart from the notes' own ink.
        constexpr Color kWaveInk{
            .red = 110, .green = 170, .blue = 235, .alpha = 255};

        // How many oscillations a note of pitch nought shows.
        constexpr std::int64_t kWaveformPeriods = 2;

        // How wide one drawn column of a wave is, in canvas pixels.
        constexpr std::int64_t kWaveformStep = 4;

        /**
         * @brief Get how many oscillations one note's wave shows.
         *
         * Doubled per octave up and halved per octave down, in whole
         * octaves, so the arithmetic stays exact and a run of notes
         * reads as higher-is-denser.  Saturated well inside the shift
         * width, since a notation may name any pitch it likes.
         *
         * @param pitch The note, in whole semitones.
         * @return The oscillations, at least one.
         */
        [[nodiscard]] std::int64_t wavePeriodsOf(
            const std::int64_t pitch) noexcept
        {
            if (pitch >= 0)
            {
                return kWaveformPeriods
                       << std::min<std::int64_t>(pitch / 12, 6);
            }

            return std::max<std::int64_t>(
                1,
                kWaveformPeriods
                    >> std::min<std::int64_t>(-pitch / 12, 6));
        }

        /**
         * @brief Get how far off the midline one wave column sits.
         *
         * The phase is an exact fraction num/den of one oscillation,
         * and every shape is drawn in integer arithmetic: the sine as
         * a parabolic arc per half period, peaking where the sine
         * peaks, so no transcendental is taken per column.
         *
         * @param shape What the oscillator traces.
         * @param num The phase's top, below den.
         * @param den The phase's bottom, above nought.
         * @param half The amplitude, in pixels off the midline.
         * @param column Which column of the band, for the noise hash.
         * @return The offset, positive above the midline.
         */
        [[nodiscard]] std::int64_t waveOffset(
            const Waveshape shape,
            const std::int64_t num,
            const std::int64_t den,
            const std::int64_t half,
            const std::int64_t column) noexcept
        {
            if (shape == Waveshape::Square)
            {
                return num * 2 < den ? half : -half;
            }

            if (shape == Waveshape::Saw)
            {
                return half * (2 * num - den) / den;
            }

            if (shape == Waveshape::Triangle)
            {
                return num * 2 < den
                           ? half * (4 * num - den) / den
                           : half * (3 * den - 4 * num) / den;
            }

            if (shape == Waveshape::Sine)
            {
                return num * 2 < den
                           ? half * 8 * num * (den - 2 * num)
                                 / (den * den)
                           : -half * 8 * (2 * num - den) * (den - num)
                                 / (den * den);
            }

            // Noise: a hash of the column, so the picture holds still.
            const auto bits =
                static_cast<std::uint64_t>(column) * 2654435761ULL;
            const auto mixed = bits ^ (bits >> 13);

            return static_cast<std::int64_t>(
                       mixed
                       % static_cast<std::uint64_t>(2 * half + 1))
                   - half;
        }

        /**
         * @brief Append one waveform's picture over its band.
         *
         * A backdrop, and a column of ink per few pixels of each
         * note: the column reaches from the midline to where the
         * oscillator's shape sits at that phase, scaled by the gain,
         * so a quiet line reads as a shallow wave and a rest as the
         * bare backdrop.
         *
         * @param commands The picture to append to.
         * @param band Where the band ended up.
         * @param wave What the line plays, and with what sound.
         */
        void appendWave(
            DrawList &commands, const Rect &band, const Waveform &wave)
        {
            commands.push_back(antwika::ui::FillRect{
                .rect = band, .color = kRollBackdrop});

            const auto half = static_cast<std::int64_t>(
                std::abs(wave.preset.gain)
                * static_cast<float>(band.size.height / 2));

            const auto mid =
                band.origin.y
                + static_cast<std::int32_t>(band.size.height / 2);

            for (const auto &note : rollNotesOf(wave.playing))
            {
                const auto left =
                    acrossRoll(note.begin, band.size.width);
                const auto right =
                    acrossRoll(note.end, band.size.width);
                const auto total = right - left;

                // A note thinner than a pixel has no column to hold.
                if (total <= 0)
                {
                    continue;
                }

                const auto periods = wavePeriodsOf(note.pitch);

                for (std::int64_t x = 0; x < total;
                     x += kWaveformStep)
                {
                    const auto width = std::min<std::int64_t>(
                        kWaveformStep, total - x);

                    const auto off = waveOffset(
                        wave.preset.shape,
                        x * periods % total,
                        total,
                        half,
                        left + x);

                    const auto up =
                        std::max<std::int64_t>(off, 0);
                    const auto down =
                        std::max<std::int64_t>(-off, 0);

                    commands.push_back(antwika::ui::FillRect{
                        .rect = Rect{
                            .origin =
                                {.x = band.origin.x
                                      + static_cast<std::int32_t>(
                                          left + x),
                                 .y = mid
                                      - static_cast<std::int32_t>(up)},
                            .size =
                                {.width =
                                     static_cast<std::uint32_t>(width),
                                 .height = static_cast<std::uint32_t>(
                                     std::max<std::int64_t>(
                                         1, up + down))}},
                        .color = kWaveInk});
                }
            }
        }

        /**
         * @brief Append every waveform the score asked for.
         *
         * On appendPianorolls' terms exactly, band for band.
         *
         * @param frame The finished frame, whose commands grow.
         * @param score Whose waves to paint.
         */
        void appendWaveforms(Frame &frame, const Score &score)
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

                appendWave(
                    frame.commands, *band, score.waveforms()[at]);
            }
        }

        /**
         * @brief Append every pianoroll the score asked for.
         *
         * Painted from the band rectangles the layout reported, so
         * the picture cannot drift from where the room was held; a
         * band scrolled off the top, or cut short by the pane's
         * bottom edge, draws nothing -- the pane shows whole rows
         * and no half ones, and a roll follows that rule.
         *
         * @param frame The finished frame, whose commands grow.
         * @param score Whose rolls to paint.
         */
        void appendPianorolls(Frame &frame, const Score &score)
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
                    score.pianorolls()[at].playing);
            }
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
        // The bands of room the rolls stand in, one per roll.
        // The Context borrows them until finish().
        // So they are declared before it, as the document's text is.
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

        // After the rolls.
        // A line asking for both stacks the roll over the wave.
        for (std::size_t at = 0; at < score.waveforms().size(); ++at)
        {
            bands.push_back(antwika::ui::LineBand{
                .line = score.waveforms()[at].line,
                .rows = kWaveformRows,
                .id = waveformBand(at)});
        }

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

                // How fast the musical clock runs.
                // A choice is a recorded click, so a replay keeps pace.
                const DropdownSpec speed{
                    .id = kSpeedBox,
                    .optionIdBase = kSpeedOptions,
                    .options = kSpeedLabels,
                    .selected = state.speed,
                    .open = state.speedOpen};

                ui.dropdown(speed);

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
                    // A pianoroll's room, held under the line asking.
                    .bands = bands,
                    .scrollbar = true,
                    .focused = true,
                    .dragging = state.dragging});

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

        // The rolls are painted over the room their bands held open.
        // Their places are the rectangles the layout reported.
        // So the picture cannot drift from where a click is read.
        auto frame = ui.finish();

        appendPianorolls(frame, score);
        appendWaveforms(frame, score);

        return frame;
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
