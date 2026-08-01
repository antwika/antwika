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
        // How wide a gauge is, leaving a unit of margin either side.
        constexpr std::uint32_t kBarUnits = kSceneUnits - 2;

        // How many glyph pixels one layout unit is worth.
        // So the readout grows with the window rather than beside it.
        // A unit worth fewer than this still gets the smallest text.
        constexpr std::uint32_t kGlyphPixelsPerUnit = 4;

        // What the readout says, in the order it says it.
        constexpr std::size_t kReadoutLines = 3;

        // The one fact no gauge holds, in the words it is said in.
        // An interrupted night is worth saying apart from a quiet one.
        // It is the rest of that night's recovery already forfeited.
        constexpr std::string_view kAwakeWord = "awake";
        constexpr std::string_view kHungryWord = "awake, hungry";
        constexpr std::string_view kAsleepWord = "asleep";
        constexpr std::string_view kWokenWord = "asleep, woken";
        constexpr std::string_view kGoneWord = "gone";

        // Every line the companion may say, in Saying's own order.
        // One table in one place, and this is the place.
        // The words are presentation, where a Saying is what is decided.
        // antwika::i18n is deliberately not used here.
        // The readout below is English written into this same file.
        // A catalogue holding one and not the other translates by halves.
        // It would also leave two places to add a line to.
        // Moving both is a change worth making on its own.
        constexpr std::array<std::string_view, 10> kSayingWords{
            "",
            "hello!",
            "bored...",
            "nice day",
            "la la la",
            "feed me!",
            "yum yum!",
            "im full!",
            "shhh!",
            "zzz..."};

        static_assert(
            kSayingWords.size()
            == static_cast<std::size_t>(Saying::Zzz) + 1);

        // Where the bubble sits, in the layout's own units.
        // Left of the animal, under the gauges and over the bowl.
        // So it covers nothing that says anything.
        constexpr std::int32_t kBubbleX = 1;
        constexpr std::int32_t kBubbleY = 7;
        constexpr std::uint32_t kBubbleUnitsWide = 10;
        constexpr std::uint32_t kBubbleUnitsHigh = 5;
        constexpr std::uint32_t kBubbleTailUnits = 2;
        constexpr std::uint32_t kBubblePadUnits = 1;

        // The longest line the table above holds.
        // The text is scaled to fit this rather than to fit each line.
        // So the bubble is one size and the words one height throughout.
        // A window smaller than main.cpp's cannot give it that room.
        // The longest lines overhang their bubble there.
        // Which is where the readout already overhangs the grid.
        // So neither is clamped, and both stay one arithmetic rule.
        constexpr std::uint32_t kSayingChars = 8;

        constexpr Tick kBreatheFrameTicks = kTicksPerSecond / 2;
        constexpr Tick kEyesOpenTicks = 3 * kTicksPerSecond;
        constexpr Tick kEyesShutTicks = kTicksPerSecond / 5;
        constexpr Tick kDrowseFrameTicks = 3 * kTicksPerSecond / 4;

        // How far up the animal sits on each frame of a breath.
        constexpr std::array<std::int32_t, 4> kBob{0, 1, 1, 0};

        struct Layout
        {
            std::uint32_t unit = 0;
            Point origin{};
        };

        struct Palette
        {
            Color sky;
            Color ground;
            Color orb;
            Color fur;
            Color detail;
            Color eye;
            Color gauge;
            Color hungerFill;
            Color happinessFill;
            Color text;
            Color bubble;
            Color bubbleText;
        };

        constexpr Palette kDay{
            .sky = {.red = 132, .green = 190, .blue = 226},
            .ground = {.red = 78, .green = 142, .blue = 82},
            .orb = {.red = 250, .green = 226, .blue = 130},
            .fur = {.red = 226, .green = 158, .blue = 88},
            .detail = {.red = 168, .green = 106, .blue = 52},
            .eye = {.red = 34, .green = 30, .blue = 40},
            .gauge = {.red = 42, .green = 46, .blue = 58},
            .hungerFill = {.red = 226, .green = 118, .blue = 78},
            .happinessFill = {.red = 118, .green = 210, .blue = 138},
            .text = {.red = 246, .green = 250, .blue = 244},
            .bubble = {.red = 250, .green = 248, .blue = 238},
            .bubbleText = {.red = 42, .green = 46, .blue = 58}};

        constexpr Palette kNight{
            .sky = {.red = 24, .green = 30, .blue = 62},
            .ground = {.red = 32, .green = 62, .blue = 44},
            .orb = {.red = 214, .green = 220, .blue = 240},
            .fur = {.red = 142, .green = 100, .blue = 60},
            .detail = {.red = 104, .green = 68, .blue = 36},
            .eye = {.red = 24, .green = 22, .blue = 30},
            .gauge = {.red = 30, .green = 34, .blue = 44},
            .hungerFill = {.red = 150, .green = 78, .blue = 54},
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
            .hungerFill = {.red = 96, .green = 96, .blue = 102},
            .happinessFill = {.red = 96, .green = 96, .blue = 102},
            .text = {.red = 182, .green = 182, .blue = 188},
            .bubble = {.red = 150, .green = 150, .blue = 156},
            .bubbleText = {.red = 40, .green = 40, .blue = 46}};

        [[nodiscard]] std::optional<Layout> layoutFor(const Size canvas)
        {
            const auto byWidth = canvas.width / kSceneUnits;
            const auto byHeight = canvas.height / kSceneUnits;
            const auto unit = byWidth < byHeight ? byWidth : byHeight;

            if (unit == 0)
            {
                return std::nullopt;
            }

            const auto used = unit * kSceneUnits;

            return Layout{
                .unit = unit,
                .origin = {
                    .x = static_cast<std::int32_t>(
                        (canvas.width - used) / 2),
                    .y = static_cast<std::int32_t>(
                        (canvas.height - used) / 2)}};
        }

        [[nodiscard]] Point point(
            const Layout &layout,
            const std::int32_t x,
            const std::int32_t y)
        {
            const auto unit = static_cast<std::int32_t>(layout.unit);

            return Point{
                .x = layout.origin.x + x * unit,
                .y = layout.origin.y + y * unit};
        }

        [[nodiscard]] Rect box(
            const Layout &layout,
            const std::int32_t x,
            const std::int32_t y,
            const std::uint32_t width,
            const std::uint32_t height)
        {
            return Rect{
                .origin = point(layout, x, y),
                .size = {
                    .width = width * layout.unit,
                    .height = height * layout.unit}};
        }

        // Four glyph pixels to a unit, so the readout grows with it.
        // A unit too small for even that still gets the smallest text.
        [[nodiscard]] std::uint32_t textScale(const Layout &layout)
        {
            const auto scale = layout.unit / kGlyphPixelsPerUnit;

            if (scale == 0)
            {
                return 1;
            }

            return scale;
        }

        [[nodiscard]] std::string ratio(
            const std::uint32_t value, const std::uint32_t max)
        {
            return std::to_string(value) + "/" + std::to_string(max);
        }

        [[nodiscard]] std::string_view stateWord(
            const PetSnapshot &snapshot)
        {
            if (snapshot.state == PetState::Perished)
            {
                return kGoneWord;
            }

            if (snapshot.state == PetState::Asleep)
            {
                return snapshot.disturbed ? kWokenWord : kAsleepWord;
            }

            return snapshot.hungry ? kHungryWord : kAwakeWord;
        }

        // Anchored to the bottom of the grid rather than to a row.
        // A unit of margin sits under the last line, and no more.
        // So three lines fit whatever a unit turned out to be worth.
        void drawReadout(
            IRenderer &renderer,
            const Layout &layout,
            const Palette &palette,
            const PetSnapshot &snapshot)
        {
            const auto scale = textScale(layout);
            const auto step = static_cast<std::int32_t>(
                antwika::gfx::kGlyphLineHeight * scale);
            const Point floorLine = point(
                layout, 1, static_cast<std::int32_t>(kSceneUnits) - 1);
            const auto top =
                floorLine.y
                - static_cast<std::int32_t>(kReadoutLines) * step;

            // A line at a time rather than a collection of them.
            // A part-built collection has an unwinding no test reaches.
            // Which is a branch the coverage gate would then refuse.
            renderer.drawText(
                Point{.x = floorLine.x, .y = top},
                "hunger " + ratio(snapshot.hunger, snapshot.hungerMax),
                scale,
                palette.text);
            renderer.drawText(
                Point{.x = floorLine.x, .y = top + step},
                "happy "
                    + ratio(snapshot.happiness, snapshot.happinessMax),
                scale,
                palette.text);
            renderer.drawText(
                Point{.x = floorLine.x, .y = top + 2 * step},
                stateWord(snapshot),
                scale,
                palette.text);
        }

        // Scaled to the longest line rather than to the one being said.
        // A unit too small for even the smallest glyphs still gets them.
        [[nodiscard]] std::uint32_t bubbleScale(const Layout &layout)
        {
            const auto room =
                (kBubbleUnitsWide - 2 * kBubblePadUnits) * layout.unit;
            const auto scale =
                room / (kSayingChars * antwika::gfx::kGlyphAdvance);

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
            const Layout &layout,
            const Palette &palette,
            const Saying saying)
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
                    kBubbleY + static_cast<std::int32_t>(kBubbleUnitsHigh),
                    kBubbleTailUnits,
                    1),
                palette.bubble);

            const std::string_view words =
                kSayingWords[static_cast<std::size_t>(saying)];
            const auto scale = bubbleScale(layout);
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
            const Layout &layout,
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
            const Layout &layout,
            const Palette &palette,
            const std::int32_t bob,
            const bool eyesShut)
        {
            const std::int32_t lift = -bob;

            renderer.drawRect(
                box(layout, 11, 22 + lift, 3, 2), palette.detail);
            renderer.drawRect(
                box(layout, 18, 22 + lift, 3, 2), palette.detail);
            renderer.drawRect(
                box(layout, 21, 16 + lift, 4, 2), palette.detail);
            renderer.drawRect(
                box(layout, 10, 14 + lift, 12, 9), palette.fur);
            renderer.drawRect(
                box(layout, 11, 5 + lift, 2, 2), palette.detail);
            renderer.drawRect(
                box(layout, 19, 5 + lift, 2, 2), palette.detail);
            renderer.drawRect(
                box(layout, 11, 7 + lift, 10, 8), palette.fur);
            renderer.drawRect(
                box(layout, 15, 12 + lift, 2, 1), palette.detail);

            if (eyesShut)
            {
                renderer.drawRect(
                    box(layout, 13, 11 + lift, 2, 1), palette.eye);
                renderer.drawRect(
                    box(layout, 17, 11 + lift, 2, 1), palette.eye);
                return;
            }

            renderer.drawRect(
                box(layout, 13, 10 + lift, 2, 2), palette.eye);
            renderer.drawRect(
                box(layout, 17, 10 + lift, 2, 2), palette.eye);
        }

        void drawGrave(
            IRenderer &renderer,
            const Layout &layout,
            const Palette &palette)
        {
            renderer.drawRect(box(layout, 10, 22, 12, 2), palette.detail);
            renderer.drawRect(box(layout, 12, 11, 8, 12), palette.fur);
            renderer.drawRect(box(layout, 15, 13, 2, 7), palette.eye);
            renderer.drawRect(box(layout, 13, 15, 6, 2), palette.eye);
        }

        // The bowl says what to do about it, where a gauge only reports.
        // It is the one thing on screen that a tap is an answer to.
        void drawBowl(
            IRenderer &renderer,
            const Layout &layout,
            const Palette &palette)
        {
            renderer.drawRect(box(layout, 3, 21, 6, 3), palette.detail);
            renderer.drawRect(box(layout, 4, 20, 4, 1), palette.eye);
        }

        void drawSleepPuffs(
            IRenderer &renderer,
            const Layout &layout,
            const Palette &palette,
            const std::size_t count)
        {
            for (std::size_t index = 0; index <= count; ++index)
            {
                const auto step = static_cast<std::int32_t>(index);
                renderer.drawRect(
                    box(layout, 22 + step * 2, 4 - step * 2, 2, 2),
                    palette.orb);
            }
        }

        [[nodiscard]] const Palette &paletteFor(
            const PetSnapshot &snapshot)
        {
            if (snapshot.state == PetState::Perished)
            {
                return kGone;
            }

            return snapshot.night ? kNight : kDay;
        }
    } // namespace

    PetScene::PetScene()
        : breathe(uniformClip(0, kBob.size(), kBreatheFrameTicks)),
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

        renderer.drawRect(box(*layout, 0, 24, kSceneUnits, 8),
                          palette.ground);
        renderer.drawRect(box(*layout, 25, 8, 4, 4), palette.orb);

        drawGauge(
            renderer,
            *layout,
            1,
            snapshot.hunger,
            snapshot.hungerMax,
            palette,
            palette.hungerFill);
        drawGauge(
            renderer,
            *layout,
            4,
            snapshot.happiness,
            snapshot.happinessMax,
            palette,
            palette.happinessFill);

        if (snapshot.state == PetState::Perished)
        {
            drawGrave(renderer, *layout, palette);
        }
        else
        {
            // Every moving part resolves from the tick count here.
            // So drawing the same tick twice draws the same pixels.
            // And a replay draws exactly what the recorded run drew.
            const auto breath = resolve(breathe, snapshot.ticks);
            const bool asleep = snapshot.state == PetState::Asleep;
            const bool eyesShut =
                asleep || resolve(blink, snapshot.ticks).index == 1;

            drawAnimal(
                renderer, *layout, palette, kBob[breath.index], eyesShut);

            if (asleep)
            {
                drawSleepPuffs(
                    renderer,
                    *layout,
                    palette,
                    resolve(drowse, snapshot.ticks).index);
            }
            else if (snapshot.hungry)
            {
                drawBowl(renderer, *layout, palette);
            }
        }

        // Over the animal rather than under it.
        // A bubble it stands in front of is somebody else talking.
        if (snapshot.saying != Saying::None)
        {
            drawBubble(renderer, *layout, palette, snapshot.saying);
        }

        // Last, so anything drawn over the ground stays behind it.
        // A perished companion is reported too.
        // Which is why the grave no longer ends this function.
        drawReadout(renderer, *layout, palette, snapshot);
    }

} // namespace antwika::companion
