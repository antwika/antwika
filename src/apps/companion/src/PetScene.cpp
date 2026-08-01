#include "antwika/companion/PetScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/animation/Playback.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/i18n/MessageId.hpp>

#include "antwika/companion/Saying.hpp"

namespace antwika::companion
{

    using antwika::animation::Clip;
    using antwika::animation::KeyFrame;
    using antwika::animation::LoopMode;
    using antwika::animation::resolve;
    using antwika::animation::uniformClip;
    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::i18n::MessageId;

    namespace
    {
        // How wide a gauge is, leaving a unit of margin either side.
        constexpr std::uint32_t kBarUnits = kSceneUnits - 2;

        // How many glyph pixels one layout unit is worth.
        // So the readout grows with the window rather than beside it.
        // A unit worth fewer than this still gets the smallest text.
        constexpr std::uint32_t kGlyphPixelsPerUnit = 4;

        // What the readout says, in the order it says it.
        constexpr std::size_t kReadoutLines = 3;

        // The one fact no gauge holds, as the id it is said with.
        // An interrupted night is worth saying apart from a quiet one.
        // It is the rest of that night's recovery already forfeited.
        constexpr MessageId kAwakeWord = MessageId::CompanionAwake;
        constexpr MessageId kHungryWord =
            MessageId::CompanionAwakeHungry;
        constexpr MessageId kAsleepWord = MessageId::CompanionAsleep;
        constexpr MessageId kWokenWord =
            MessageId::CompanionAsleepWoken;
        constexpr MessageId kGoneWord = MessageId::CompanionGone;

        // What the one button says, in this file with the other ids.
        constexpr MessageId kReviveWords = MessageId::CompanionNewPet;

        // How grown up it is, in LifeStage's own order.


        // Every line the companion may say, in Saying's own order.
        // One table in one place, and this is the place.
        // An id is presentation, where a Saying is what is decided.
        // Nothing here is the words themselves any more.
        // Those live in one catalogue per language, and Pet sees none.
        // Which is what keeps the active language out of the state.
        constexpr std::array<MessageId, 16> kSayingWords{
            MessageId::CompanionSayHello,
            MessageId::CompanionSayHello,
            MessageId::CompanionSayBored,
            MessageId::CompanionSayNiceDay,
            MessageId::CompanionSayLaLaLa,
            MessageId::CompanionSayFeedMe,
            MessageId::CompanionSayYumYum,
            MessageId::CompanionSayFull,
            MessageId::CompanionSayShhh,
            MessageId::CompanionSayZzz,
            MessageId::CompanionSayPlay,
            MessageId::CompanionSayWheee,
            MessageId::CompanionSayTooTired,
            MessageId::CompanionSayNotSleepy,
            MessageId::CompanionSayYawn,
            MessageId::CompanionSayPoked};

        // Every life stage and every day mood, in their own order.
        // An ordinary day says nothing, so it has no id of its own.
        constexpr std::array<MessageId, 5> kStageWords{
            MessageId::CompanionStageEgg,
            MessageId::CompanionStageChild,
            MessageId::CompanionStageTeen,
            MessageId::CompanionStageAdult,
            MessageId::CompanionStageElder};

        constexpr std::array<MessageId, 3> kMoodWords{
            MessageId::CompanionMoodHungry,
            MessageId::CompanionMoodRestless,
            MessageId::CompanionMoodHeavy};

        static_assert(
            kSayingWords.size()
            == static_cast<std::size_t>(Saying::Poked) + 1);

        // Saying::None draws no bubble at all, so slot zero is unread.
        // It repeats the next entry rather than naming an empty id.
        // A catalogue entry that resolved to nothing would be a gap.
        // And a gap is what the exclamation marks exist to show.
        static_assert(kSayingWords[0] == kSayingWords[1]);

        // Where the bubble sits, in the layout's own units.
        // Left of the animal, under the gauges and over the props.
        // So it covers nothing that says anything.
        // It does overlap PetLayout's kButton box, deliberately.
        // The two are never up at once, so the room is spent twice.
        // A perished companion is silent: perish() clears the bubble.
        // Pet::requireLivable() refuses a saved one that is not.
        constexpr std::int32_t kBubbleX = 1;
        constexpr std::int32_t kBubbleY = 8;
        constexpr std::uint32_t kBubbleUnitsWide = 11;
        constexpr std::uint32_t kBubbleUnitsHigh = 5;
        constexpr std::uint32_t kBubbleTailUnits = 2;
        constexpr std::uint32_t kBubblePadUnits = 1;

        // The longest line the catalogue holds for the table above.
        // The text is scaled to fit this rather than to fit each line.
        // So the bubble is one size and the words one height throughout.
        // Measured rather than written down.
        // A count taken off English would be wrong for every other.
        // A window smaller than main.cpp's cannot give it that room.
        // The longest lines overhang their bubble there.
        // Which is where the readout already overhangs the grid.
        // So neither is clamped, and both stay one arithmetic rule.
        // Where the ground begins.
        // The animal stands on it and the three props sit along it.
        constexpr std::int32_t kGroundY = 22;
        [[nodiscard]] std::size_t longestSaying(
            const Translator &translator)
        {
            std::size_t longest = 1;

            for (const MessageId id : kSayingWords)
            {
                const auto length = translator.text(id).size();

                if (length > longest)
                {
                    longest = length;
                }
            }

            return longest;
        }

        constexpr Tick kBreatheFrameTicks = kTicksPerSecond / 2;
        constexpr Tick kEyesOpenTicks = 3 * kTicksPerSecond;
        constexpr Tick kEyesShutTicks = kTicksPerSecond / 5;
        constexpr Tick kDrowseFrameTicks = 3 * kTicksPerSecond / 4;

        // How far up the animal sits on each frame of a breath.
        constexpr std::array<std::int32_t, 4> kBob{0, 1, 1, 0};

        // What each form does to the fur it is drawn in, as a fraction.
        // A factor rather than three more colours in every palette.
        // So the day, the night and the grave keep one fur to shade.
        constexpr std::array<std::uint32_t, 3> kFormNumerator{5, 1, 3};
        constexpr std::array<std::uint32_t, 3> kFormDenominator{4, 1, 4};

        static_assert(
            kFormNumerator.size()
            == static_cast<std::size_t>(PetForm::Scruffy) + 1);

        struct Palette
        {
            Color sky;
            Color ground;
            Color orb;
            Color fur;
            Color detail;
            Color eye;
            Color gauge;
            Color energyFill;
            Color hungerFill;
            Color funFill;
            Color happinessFill;
            Color text;
            Color bubble;
            Color bubbleText;
        };

        constexpr Palette kDay{
            .sky = {.red = 132, .green = 190, .blue = 226},
            .ground = {.red = 78, .green = 142, .blue = 82},
            .orb = {.red = 250, .green = 226, .blue = 130},
            .fur = {.red = 200, .green = 140, .blue = 78},
            .detail = {.red = 168, .green = 106, .blue = 52},
            .eye = {.red = 34, .green = 30, .blue = 40},
            .gauge = {.red = 42, .green = 46, .blue = 58},
            .energyFill = {.red = 238, .green = 206, .blue = 92},
            .hungerFill = {.red = 226, .green = 118, .blue = 78},
            .funFill = {.red = 132, .green = 168, .blue = 232},
            .happinessFill = {.red = 118, .green = 210, .blue = 138},
            .text = {.red = 246, .green = 250, .blue = 244},
            .bubble = {.red = 250, .green = 248, .blue = 238},
            .bubbleText = {.red = 42, .green = 46, .blue = 58}};

        constexpr Palette kNight{
            .sky = {.red = 24, .green = 30, .blue = 62},
            .ground = {.red = 32, .green = 62, .blue = 44},
            .orb = {.red = 214, .green = 220, .blue = 240},
            .fur = {.red = 130, .green = 92, .blue = 54},
            .detail = {.red = 104, .green = 68, .blue = 36},
            .eye = {.red = 24, .green = 22, .blue = 30},
            .gauge = {.red = 30, .green = 34, .blue = 44},
            .energyFill = {.red = 156, .green = 136, .blue = 62},
            .hungerFill = {.red = 150, .green = 78, .blue = 54},
            .funFill = {.red = 86, .green = 110, .blue = 154},
            .happinessFill = {.red = 78, .green = 140, .blue = 96},
            .text = {.red = 196, .green = 206, .blue = 226},
            .bubble = {.red = 206, .green = 212, .blue = 232},
            .bubbleText = {.red = 24, .green = 30, .blue = 62}};

        // A perished companion keeps its own palette, not the night's.
        // So the picture still says what happened at noon the day after.
        constexpr Palette kGone{
            .sky = {.red = 46, .green = 46, .blue = 54},
            .ground = {.red = 58, .green = 58, .blue = 62},
            .orb = {.red = 92, .green = 92, .blue = 100},
            .fur = {.red = 130, .green = 130, .blue = 136},
            .detail = {.red = 96, .green = 96, .blue = 102},
            .eye = {.red = 40, .green = 40, .blue = 46},
            .gauge = {.red = 34, .green = 34, .blue = 40},
            .energyFill = {.red = 96, .green = 96, .blue = 102},
            .hungerFill = {.red = 96, .green = 96, .blue = 102},
            .funFill = {.red = 96, .green = 96, .blue = 102},
            .happinessFill = {.red = 96, .green = 96, .blue = 102},
            .text = {.red = 182, .green = 182, .blue = 188},
            .bubble = {.red = 150, .green = 150, .blue = 156},
            .bubbleText = {.red = 40, .green = 40, .blue = 46}};

        [[nodiscard]] std::uint8_t channel(
            const std::uint8_t value,
            const std::uint32_t numerator,
            const std::uint32_t denominator)
        {
            const auto raised = static_cast<std::uint32_t>(value)
                                * numerator / denominator;

            return static_cast<std::uint8_t>(raised > 255 ? 255 : raised);
        }

        // Whole-number arithmetic on each channel.
        // So a form's fur is the same colour on every toolchain.
        // That is the rule the rest of this application follows.
        // Kept even though nothing a renderer makes reaches a replay.
        [[nodiscard]] Color shade(
            const Color color,
            const std::uint32_t numerator,
            const std::uint32_t denominator)
        {
            return Color{
                .red = channel(color.red, numerator, denominator),
                .green = channel(color.green, numerator, denominator),
                .blue = channel(color.blue, numerator, denominator),
                .alpha = color.alpha};
        }

        [[nodiscard]] Color furFor(
            const Palette &palette, const PetForm form)
        {
            const auto index = static_cast<std::size_t>(form);

            return shade(
                palette.fur,
                kFormNumerator[index],
                kFormDenominator[index]);
        }

        // Called on a prop's box alone, which is six units by four.
        // So a unit off each side always leaves something to draw.
        // A guard against that would be a branch no canvas could take.
        [[nodiscard]] Rect inset(const Rect &area, const std::uint32_t by)
        {
            const auto twice = 2 * by;

            return Rect{
                .origin =
                    {.x = area.origin.x + static_cast<std::int32_t>(by),
                     .y = area.origin.y + static_cast<std::int32_t>(by)},
                .size = {
                    .width = area.size.width - twice,
                    .height = area.size.height - twice}};
        }

        // Four glyph pixels to a unit, so the readout grows with it.
        // A unit too small for even that still gets the smallest text.
        [[nodiscard]] std::uint32_t textScale(const SceneLayout &layout)
        {
            const auto scale = layout.unit / kGlyphPixelsPerUnit;

            if (scale == 0)
            {
                return 1;
            }

            return scale;
        }


        [[nodiscard]] MessageId stateWord(const PetSnapshot &snapshot)
        {
            if (snapshot.state == PetState::Perished)
            {
                return kGoneWord;
            }

            if (snapshot.asleep)
            {
                return kAsleepWord;
            }

            if (snapshot.disturbed)
            {
                return kWokenWord;
            }

            return snapshot.hungry ? kHungryWord : kAwakeWord;
        }

        // How the day is going, in the words the tables above name.
        // An ordinary day contributes nothing at all.
        // So its mood has no id, and the table is three rather than four.
        [[nodiscard]] std::string dayLine(
            const PetSnapshot &snapshot, const Translator &translator)
        {
            const std::string day = std::to_string(snapshot.day);
            const std::string stage = translator.text(
                kStageWords[static_cast<std::size_t>(snapshot.stage)]);
            const std::string mood =
                snapshot.mood == DayMood::Ordinary
                    ? std::string()
                    : translator.text(
                          kMoodWords
                              [static_cast<std::size_t>(snapshot.mood)
                               - 1]);
            const std::array<std::string_view, 3> args{day, stage, mood};

            return translator.formatted(MessageId::CompanionDay, args);
        }

        [[nodiscard]] std::string lineageLine(
            const PetSnapshot &snapshot, const Translator &translator)
        {
            const std::string generation =
                std::to_string(snapshot.lineage.generation);
            const std::string best =
                std::to_string(snapshot.lineage.bestTicks);
            const std::array<std::string_view, 2> args{generation, best};

            return translator.formatted(
                MessageId::CompanionLineage, args);
        }

        // Anchored to the bottom of the grid rather than to a row.
        // So three lines fit whatever a unit turned out to be worth.
        void drawReadout(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const PetSnapshot &snapshot,
            const Translator &translator)
        {
            const auto scale = textScale(layout);
            const auto step = static_cast<std::int32_t>(
                antwika::gfx::kGlyphLineHeight * scale);
            const Point floorLine =
                point(layout, 1, static_cast<std::int32_t>(kSceneUnits));
            const auto top =
                floorLine.y
                - static_cast<std::int32_t>(kReadoutLines) * step;

            // A line at a time rather than a collection of them.
            // A part-built collection has an unwinding no test reaches.
            // Which is a branch the coverage gate would then refuse.
            renderer.drawText(
                Point{.x = floorLine.x, .y = top},
                translator.text(stateWord(snapshot)),
                scale,
                palette.text);
            renderer.drawText(
                Point{.x = floorLine.x, .y = top + step},
                dayLine(snapshot, translator),
                scale,
                palette.text);
            renderer.drawText(
                Point{.x = floorLine.x, .y = top + 2 * step},
                lineageLine(snapshot, translator),
                scale,
                palette.text);
        }

        // Scaled to the longest line rather than to the one being said.
        // A unit too small for even the smallest glyphs still gets them.
        [[nodiscard]] std::uint32_t bubbleScale(
            const SceneLayout &layout, const Translator &translator)
        {
            const auto room =
                (kBubbleUnitsWide - 2 * kBubblePadUnits) * layout.unit;
            const auto widest = static_cast<std::uint32_t>(
                longestSaying(translator));
            const auto scale =
                room / (widest * antwika::gfx::kGlyphAdvance);

            if (scale == 0)
            {
                return 1;
            }

            return scale;
        }

        // Two rectangles and a line: the bubble and its tail.
        // The tail points at the animal standing to the right of it.
        // What it says arrives as a Saying rather than as words.
        // So deciding to speak and saying something are one decision.
        void drawBubble(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Saying saying,
            const Translator &translator)
        {
            renderer.drawRect(
                box(
                    layout,
                    kBubbleX,
                    kBubbleY,
                    kBubbleUnitsWide,
                    kBubbleUnitsHigh),
                palette.bubble);
            renderer.drawRect(
                box(
                    layout,
                    kBubbleX
                        + static_cast<std::int32_t>(
                            kBubbleUnitsWide - kBubbleTailUnits),
                    kBubbleY
                        + static_cast<std::int32_t>(kBubbleUnitsHigh),
                    kBubbleTailUnits,
                    1),
                palette.bubble);

            const std::string words = translator.text(
                kSayingWords[static_cast<std::size_t>(saying)]);
            const auto scale = bubbleScale(layout, translator);
            const auto text = antwika::gfx::textSize(words, scale);
            const Point corner = point(layout, kBubbleX, kBubbleY);
            const auto width = static_cast<std::int32_t>(
                kBubbleUnitsWide * layout.unit);
            const auto height = static_cast<std::int32_t>(
                kBubbleUnitsHigh * layout.unit);

            renderer.drawText(
                Point{
                    .x = corner.x
                         + (width
                            - static_cast<std::int32_t>(text.width))
                               / 2,
                    .y = corner.y
                         + (height
                            - static_cast<std::int32_t>(text.height))
                               / 2},
                words,
                scale,
                palette.bubbleText);
        }

        void drawGauge(
            IRenderer &renderer,
            const SceneLayout &layout,
            const std::int32_t y,
            const std::uint32_t value,
            const std::uint32_t max,
            const Palette &palette,
            const Color fill)
        {
            renderer.drawRect(
                box(layout, 1, y, kBarUnits, 2), palette.gauge);

            if (max == 0)
            {
                return;
            }

            const auto shown = value < max ? value : max;
            const auto filled = kBarUnits * shown / max;

            if (filled == 0)
            {
                return;
            }

            renderer.drawRect(box(layout, 1, y, filled, 2), fill);
        }

        void drawAnimal(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Color fur,
            const std::int32_t bob,
            const bool eyesShut)
        {
            const std::int32_t lift = -bob;

            renderer.drawRect(
                box(layout, 11, 20 + lift, 3, 2), palette.detail);
            renderer.drawRect(
                box(layout, 18, 20 + lift, 3, 2), palette.detail);
            renderer.drawRect(
                box(layout, 21, 15 + lift, 4, 2), palette.detail);
            renderer.drawRect(box(layout, 10, 13 + lift, 12, 8), fur);
            renderer.drawRect(
                box(layout, 11, 8 + lift, 2, 2), palette.detail);
            renderer.drawRect(
                box(layout, 19, 8 + lift, 2, 2), palette.detail);
            renderer.drawRect(box(layout, 11, 10 + lift, 10, 6), fur);
            renderer.drawRect(
                box(layout, 15, 14 + lift, 2, 1), palette.detail);

            if (eyesShut)
            {
                renderer.drawRect(
                    box(layout, 13, 13 + lift, 2, 1), palette.eye);
                renderer.drawRect(
                    box(layout, 17, 13 + lift, 2, 1), palette.eye);
                return;
            }

            renderer.drawRect(
                box(layout, 13, 12 + lift, 2, 2), palette.eye);
            renderer.drawRect(
                box(layout, 17, 12 + lift, 2, 2), palette.eye);
        }

        void drawGrave(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette)
        {
            renderer.drawRect(box(layout, 10, 20, 12, 2), palette.detail);
            renderer.drawRect(box(layout, 12, 12, 8, 9), palette.fur);
            renderer.drawRect(box(layout, 15, 14, 2, 5), palette.eye);
            renderer.drawRect(box(layout, 13, 15, 6, 2), palette.eye);
        }

        // The one thing on screen that is pressed rather than read.
        // Where it is is reviveButtonBox()'s answer, not this one's.
        // ReviveSink tests a press against that very box.
        // So what is shown and what is hit are one rectangle.
        // It borrows the bubble's two colours rather than adding a pair.
        // Two of the three palettes could never draw it.
        // And a plate with dark words on it is what a bubble is.
        void drawReviveButton(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Translator &translator)
        {
            const Rect button = reviveButtonBox(layout);
            renderer.drawRect(button, palette.bubble);

            const auto scale = textScale(layout);
            const std::string words = translator.text(kReviveWords);
            const auto text = antwika::gfx::textSize(words, scale);

            renderer.drawText(
                Point{
                    .x = button.origin.x
                         + (static_cast<std::int32_t>(button.size.width)
                            - static_cast<std::int32_t>(text.width))
                               / 2,
                    .y = button.origin.y
                         + (static_cast<std::int32_t>(button.size.height)
                            - static_cast<std::int32_t>(text.height))
                               / 2},
                words,
                scale,
                palette.bubbleText);
        }

        // The three things a press can mean.
        // Drawn into the very boxes propAt() hit-tests against.
        // So aiming at one and hitting it are the same rectangle.
        //
        // The one the companion would like is lit rather than present.
        // Which is all this application offers instead of instructions.
        // What to press next is on the screen.
        void drawProp(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Prop prop,
            const bool wanted)
        {
            const Rect area = propBox(layout, prop);

            renderer.drawRect(area, palette.detail);
            renderer.drawRect(
                inset(area, layout.unit),
                wanted ? palette.orb : palette.eye);
        }

        [[nodiscard]] const Palette &paletteFor(
            const PetSnapshot &snapshot)
        {
            if (snapshot.state == PetState::Perished)
            {
                return kGone;
            }

            return snapshot.asleep ? kNight : kDay;
        }

        void drawSleepPuffs(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const std::size_t count)
        {
            for (std::size_t index = 0; index <= count; ++index)
            {
                const auto step = static_cast<std::int32_t>(index);
                renderer.drawRect(
                    box(layout, 22 + step * 2, 7 - step * 2, 2, 2),
                    palette.orb);
            }
        }
    } // namespace

    PetScene::PetScene(const Translator &translator)
        : translator(translator),
          breathe(uniformClip(0, kBob.size(), kBreatheFrameTicks)),
          blink(Clip(
              {KeyFrame{.index = 0, .durationTicks = kEyesOpenTicks},
               KeyFrame{.index = 1, .durationTicks = kEyesShutTicks}},
              LoopMode::Loop)),
          drowse(uniformClip(0, 3, kDrowseFrameTicks))
    {
    }

    void PetScene::draw(
        IRenderer &renderer,
        const Size canvas,
        const PetSnapshot &snapshot) const
    {
        const Palette &palette = paletteFor(snapshot);

        renderer.clear(palette.sky);

        const auto layout = layoutFor(canvas);

        if (!layout)
        {
            return;
        }

        renderer.drawRect(
            box(*layout, 0, kGroundY, kSceneUnits, kSceneUnits),
            palette.ground);
        renderer.drawRect(box(*layout, 27, 9, 4, 4), palette.orb);

        // The life meter first, since it is the one that decides.
        // Its track is drawn to the ceiling the companion still has.
        // So a collapse shortens the bar and not merely what is in it.
        drawGauge(
            renderer,
            *layout,
            0,
            snapshot.energy,
            snapshot.energyCeiling,
            palette,
            palette.energyFill);
        drawGauge(
            renderer,
            *layout,
            2,
            snapshot.hunger,
            snapshot.hungerMax,
            palette,
            palette.hungerFill);
        drawGauge(
            renderer,
            *layout,
            4,
            snapshot.fun,
            snapshot.funMax,
            palette,
            palette.funFill);
        drawGauge(
            renderer,
            *layout,
            6,
            snapshot.happiness,
            snapshot.happinessMax,
            palette,
            palette.happinessFill);

        if (snapshot.state == PetState::Perished)
        {
            drawGrave(renderer, *layout, palette);

            // Drawn from the snapshot like everything else here.
            // Whether there is a button is the state and nothing more.
            // So no renderer holds a note of one being offered.
            drawReviveButton(
                renderer, *layout, palette, translator);
        }
        else
        {
            // The props stand behind the animal.
            // So an animal before a bowl is drawn in front of it.
            drawProp(
                renderer, *layout, palette, Prop::Bowl, snapshot.hungry);
            drawProp(
                renderer, *layout, palette, Prop::Ball, snapshot.bored);
            drawProp(
                renderer, *layout, palette, Prop::Nest, snapshot.tired);

            // Every moving part resolves from the tick count here.
            // So drawing the same tick twice draws the same pixels.
            // And a replay draws exactly what the recorded run drew.
            const auto breath = resolve(breathe, snapshot.ticks);
            const bool eyesShut =
                snapshot.asleep
                || resolve(blink, snapshot.ticks).index == 1;

            drawAnimal(
                renderer,
                *layout,
                palette,
                furFor(palette, snapshot.form),
                kBob[breath.index],
                eyesShut);

            if (snapshot.asleep)
            {
                drawSleepPuffs(
                    renderer,
                    *layout,
                    palette,
                    resolve(drowse, snapshot.ticks).index);
            }
        }

        // Over the animal rather than under it.
        // A bubble it stands in front of is somebody else talking.
        if (snapshot.saying != Saying::None)
        {
            drawBubble(
                renderer, *layout, palette, snapshot.saying, translator);
        }

        // Last, so anything drawn over the ground stays behind it.
        // A perished companion is reported too.
        // Which is why the grave no longer ends this function.
        drawReadout(
            renderer, *layout, palette, snapshot, translator);
    }

} // namespace antwika::companion
