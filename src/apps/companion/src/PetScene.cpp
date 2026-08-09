#include "antwika/companion/PetScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <antwika/animation/Frame.hpp>
#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/animation/Playback.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/tween/Easing.hpp>
#include <antwika/tween/Tween.hpp>

#include "antwika/companion/MessageId.hpp"
#include "antwika/companion/Messages.hpp"
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

    namespace
    {
        constexpr std::uint32_t kBarUnits = kSceneUnits - 2;

        constexpr std::uint32_t kGlyphPixelsPerUnit = 4;

        constexpr std::size_t kReadoutLines = 3;

        constexpr MessageId kAwakeWord = MessageId::Awake;
        constexpr MessageId kHungryWord =
            MessageId::AwakeHungry;
        constexpr MessageId kAsleepWord = MessageId::Asleep;
        constexpr MessageId kWokenWord =
            MessageId::AsleepWoken;
        constexpr MessageId kGoneWord = MessageId::Gone;

        constexpr MessageId kReviveWords = MessageId::NewPet;

        constexpr std::array<MessageId, 3> kPropWords{
            MessageId::PropFeed,
            MessageId::PropPlay,
            MessageId::PropSleep};

        static_assert(
            kPropWords.size() == static_cast<std::size_t>(Prop::Nest) + 1);

        constexpr std::array<MessageId, 16> kSayingWords{
            MessageId::SayHello,
            MessageId::SayHello,
            MessageId::SayBored,
            MessageId::SayNiceDay,
            MessageId::SayLaLaLa,
            MessageId::SayFeedMe,
            MessageId::SayYumYum,
            MessageId::SayFull,
            MessageId::SayShhh,
            MessageId::SayZzz,
            MessageId::SayPlay,
            MessageId::SayWheee,
            MessageId::SayTooTired,
            MessageId::SayNotSleepy,
            MessageId::SayYawn,
            MessageId::SayPoked};

        constexpr std::array<MessageId, 5> kStageWords{
            MessageId::StageEgg,
            MessageId::StageChild,
            MessageId::StageTeen,
            MessageId::StageAdult,
            MessageId::StageElder};

        constexpr std::array<MessageId, 3> kMoodWords{
            MessageId::MoodHungry,
            MessageId::MoodRestless,
            MessageId::MoodHeavy};

        static_assert(
            kSayingWords.size()
            == static_cast<std::size_t>(Saying::Poked) + 1);

        static_assert(kSayingWords[0] == kSayingWords[1]);

        constexpr std::int32_t kBubbleX = 1;
        constexpr std::int32_t kBubbleY = 8;
        constexpr std::uint32_t kBubbleUnitsWide = 11;
        constexpr std::uint32_t kBubbleUnitsHigh = 5;
        constexpr std::uint32_t kBubbleTailUnits = 2;
        constexpr std::uint32_t kBubblePadUnits = 1;

        constexpr std::int32_t kGroundY = 22;

        [[nodiscard]] std::size_t longestOf(
            const std::span<const MessageId> ids,
            const Translator &translator)
        {
            std::size_t longest = 1;

            for (const MessageId id : ids)
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

        constexpr std::array<std::int32_t, 4> kBob{0, 1, 1, 0};

        constexpr antwika::tween::Easing kBreatheEasing =
            antwika::tween::Easing::QuadInOut;

        [[nodiscard]] std::int32_t breathLift(
            const antwika::animation::Frame &breath, std::uint32_t unit)
        {
            const auto next = (breath.index + 1) % kBob.size();

            return static_cast<std::int32_t>(antwika::tween::tweenBetween(
                kBob[breath.index] * static_cast<std::int32_t>(unit),
                kBob[next] * static_cast<std::int32_t>(unit),
                kBreatheEasing,
                breath.progress));
        }

        constexpr std::array<std::uint32_t, 3> kFormNumerator{5, 1, 3};
        constexpr std::array<std::uint32_t, 3> kFormDenominator{4, 1, 4};

        static_assert(
            kFormNumerator.size()
            == static_cast<std::size_t>(PetForm::Scruffy) + 1);

        struct Palette final
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

            return translator.formatted(MessageId::Day, args);
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
                MessageId::Lineage, args);
        }

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

        [[nodiscard]] std::uint32_t bubbleScale(
            const SceneLayout &layout, const Translator &translator)
        {
            const auto room =
                (kBubbleUnitsWide - 2 * kBubblePadUnits) * layout.unit;
            const auto widest = static_cast<std::uint32_t>(
                longestOf(kSayingWords, translator));
            const auto scale =
                room / (widest * antwika::gfx::kGlyphAdvance);

            if (scale == 0)
            {
                return 1;
            }

            return scale;
        }

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

        [[nodiscard]] Rect raised(Rect rect, const std::int32_t lift)
        {
            rect.origin.y -= lift;

            return rect;
        }

        void drawAnimal(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Color fur,
            const std::int32_t lift,
            const bool eyesShut)
        {

            renderer.drawRect(
                raised(box(layout, 11, 20, 3, 2), lift), palette.detail);
            renderer.drawRect(
                raised(box(layout, 18, 20, 3, 2), lift), palette.detail);
            renderer.drawRect(
                raised(box(layout, 21, 15, 4, 2), lift), palette.detail);
            renderer.drawRect(raised(box(layout, 10, 13, 12, 8), lift), fur);
            renderer.drawRect(
                raised(box(layout, 11, 8, 2, 2), lift), palette.detail);
            renderer.drawRect(
                raised(box(layout, 19, 8, 2, 2), lift), palette.detail);
            renderer.drawRect(raised(box(layout, 11, 10, 10, 6), lift), fur);
            renderer.drawRect(
                raised(box(layout, 15, 14, 2, 1), lift), palette.detail);

            if (eyesShut)
            {
                renderer.drawRect(
                    raised(box(layout, 13, 13, 2, 1), lift), palette.eye);
                renderer.drawRect(
                    raised(box(layout, 17, 13, 2, 1), lift), palette.eye);
                return;
            }

            renderer.drawRect(
                raised(box(layout, 13, 12, 2, 2), lift), palette.eye);
            renderer.drawRect(
                raised(box(layout, 17, 12, 2, 2), lift), palette.eye);
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

        [[nodiscard]] std::uint32_t labelScale(
            const Rect &label, const Translator &translator)
        {
            const auto widest = static_cast<std::uint32_t>(
                longestOf(kPropWords, translator));
            const auto byHeight =
                label.size.height / antwika::gfx::kGlyphLineHeight;
            const auto byWidth =
                label.size.width / (widest * antwika::gfx::kGlyphAdvance);
            const auto scale = byHeight < byWidth ? byHeight : byWidth;

            if (scale == 0)
            {
                return 1;
            }

            return scale;
        }

        void drawProp(
            IRenderer &renderer,
            const SceneLayout &layout,
            const Palette &palette,
            const Prop prop,
            const bool wanted,
            const Translator &translator)
        {
            renderer.drawRect(propBox(layout, prop), palette.detail);
            renderer.drawRect(
                inset(propArtBox(layout, prop), layout.unit),
                wanted ? palette.orb : palette.eye);

            const Rect label = propLabelBox(layout, prop);
            const std::string words = translator.text(
                kPropWords[static_cast<std::size_t>(prop)]);
            const auto scale = labelScale(label, translator);
            const auto text = antwika::gfx::textSize(words, scale);

            renderer.drawText(
                Point{
                    .x = label.origin.x
                         + (static_cast<std::int32_t>(label.size.width)
                            - static_cast<std::int32_t>(text.width))
                               / 2,
                    .y = label.origin.y
                         + (static_cast<std::int32_t>(label.size.height)
                            - static_cast<std::int32_t>(text.height))
                               / 2},
                words,
                scale,
                palette.text);
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
    }

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

            drawReviveButton(
                renderer, *layout, palette, translator);
        }
        else
        {
            drawProp(
                renderer,
                *layout,
                palette,
                Prop::Bowl,
                snapshot.hungry,
                translator);
            drawProp(
                renderer,
                *layout,
                palette,
                Prop::Ball,
                snapshot.bored,
                translator);
            drawProp(
                renderer,
                *layout,
                palette,
                Prop::Nest,
                snapshot.tired,
                translator);

            const auto breath = resolve(breathe, snapshot.ticks);
            const bool eyesShut =
                snapshot.asleep
                || resolve(blink, snapshot.ticks).index == 1;

            drawAnimal(
                renderer,
                *layout,
                palette,
                furFor(palette, snapshot.form),
                breathLift(breath, layout->unit),
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

        if (snapshot.saying != Saying::None)
        {
            drawBubble(
                renderer, *layout, palette, snapshot.saying, translator);
        }

        drawReadout(
            renderer, *layout, palette, snapshot, translator);
    }

}
