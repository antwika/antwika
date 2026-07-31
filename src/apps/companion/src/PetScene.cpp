#include "antwika/companion/PetScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/animation/Playback.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

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
        // The picture is laid out on this many whole units a side.
        // Whole units rather than fractions of the canvas is the point.
        // It keeps every rectangle the same integer on every toolchain.
        constexpr std::uint32_t kGridUnits = 32;

        // How wide a gauge is, leaving a unit of margin either side.
        constexpr std::uint32_t kBarUnits = kGridUnits - 2;

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
            .happinessFill = {.red = 118, .green = 210, .blue = 138}};

        constexpr Palette kNight{
            .sky = {.red = 24, .green = 30, .blue = 62},
            .ground = {.red = 32, .green = 62, .blue = 44},
            .orb = {.red = 214, .green = 220, .blue = 240},
            .fur = {.red = 142, .green = 100, .blue = 60},
            .detail = {.red = 104, .green = 68, .blue = 36},
            .eye = {.red = 24, .green = 22, .blue = 30},
            .gauge = {.red = 30, .green = 34, .blue = 44},
            .hungerFill = {.red = 150, .green = 78, .blue = 54},
            .happinessFill = {.red = 78, .green = 140, .blue = 96}};

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
            .happinessFill = {.red = 96, .green = 96, .blue = 102}};

        [[nodiscard]] std::optional<Layout> layoutFor(const Size canvas)
        {
            const auto byWidth = canvas.width / kGridUnits;
            const auto byHeight = canvas.height / kGridUnits;
            const auto unit = byWidth < byHeight ? byWidth : byHeight;

            if (unit == 0)
            {
                return std::nullopt;
            }

            const auto used = unit * kGridUnits;

            return Layout{
                .unit = unit,
                .origin = {
                    .x = static_cast<std::int32_t>(
                        (canvas.width - used) / 2),
                    .y = static_cast<std::int32_t>(
                        (canvas.height - used) / 2)}};
        }

        [[nodiscard]] Rect box(
            const Layout &layout,
            const std::int32_t x,
            const std::int32_t y,
            const std::uint32_t width,
            const std::uint32_t height)
        {
            const auto unit = static_cast<std::int32_t>(layout.unit);

            return Rect{
                .origin = {
                    .x = layout.origin.x + x * unit,
                    .y = layout.origin.y + y * unit},
                .size = {
                    .width = width * layout.unit,
                    .height = height * layout.unit}};
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

        renderer.drawRect(box(*layout, 0, 24, kGridUnits, 8),
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
            return;
        }

        // Every moving part resolves from the tick count carried here.
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
            return;
        }

        if (snapshot.hungry)
        {
            drawBowl(renderer, *layout, palette);
        }
    }

} // namespace antwika::companion
