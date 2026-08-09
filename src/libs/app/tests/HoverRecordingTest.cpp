#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/input/fakes/FakeInputBackend.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Hover.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/app/PointerReading.hpp"

using antwika::app::hoverFrom;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::input::InputPipelineOptions;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerHintChannel;
using antwika::input::PointerMoved;
using antwika::input::Position;
using antwika::input::fakes::FakeInputBackend;
using antwika::replay::ReplaySource;
using antwika::time::Tick;
using antwika::ui::applyHover;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Size kCanvas{.width = 100, .height = 50};

    constexpr WidgetId kLeft{1};
    constexpr WidgetId kRight{2};

    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};

    Theme plainTheme()
    {
        return Theme{
            .buttonIdle = kIdle,
            .buttonHovered = kHovered,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] std::vector<std::vector<InputEvent>> script()
    {
        std::vector<std::vector<InputEvent>> rounds;

        for (std::int32_t x = 0; x < 24; x += 4)
        {
            rounds.push_back(
                {PointerMoved{.position = Position{.x = x, .y = 4}}});
        }

        rounds.push_back({PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{.x = 20, .y = 4}}});

        return rounds;
    }

    [[nodiscard]] antwika::ui::Frame twoButtons()
    {
        Context ui{kCanvas, plainTheme()};

        {
            const auto actions = ui.row();

            ui.button("ab", {.id = kLeft});
            ui.button("cd", {.id = kRight});
        }

        return ui.finish();
    }

    [[nodiscard]] std::vector<Color> fillsOf(const DrawList &commands)
    {
        std::vector<Color> colors;

        for (const auto &command : commands)
        {
            if (const auto *fill = std::get_if<FillRect>(&command))
            {
                colors.push_back(fill->color);
            }
        }

        return colors;
    }

    struct Run final
    {
        std::vector<TickEvent> recorded;
        std::vector<std::vector<Color>> pictures;
    };

    [[nodiscard]] Run drive(bool withHover)
    {
        const InputEventCodec codec;
        ReplaySource inner({});
        FakeInputBackend backend(script());
        PointerHintChannel hints;

        InputPipelineOptions options{
            .readsDevice = true,
            .coalescePointerMotion = true,
            .thinIdleMotion = true};

        if (withHover)
        {
            options.pointerHint = std::ref(hints);
        }

        InputPipeline pipeline(inner, backend, codec, options);

        Run run;

        for (Tick tick = 0; tick < 8; ++tick)
        {
            for (auto &event : pipeline.eventsFor(tick))
            {
                run.recorded.push_back(
                    TickEvent{.tick = tick, .event = std::move(event)});
            }

            auto frame = twoButtons();

            if (withHover)
            {
                applyHover(
                    frame.commands,
                    frame.hoverTargets,
                    hoverFrom(hints.forRenderingOnly()));
            }

            run.pictures.push_back(fillsOf(frame.commands));
        }

        return run;
    }

    [[nodiscard]] std::size_t distinctPictures(const Run &run)
    {
        std::size_t distinct = 0;

        for (std::size_t index = 0; index < run.pictures.size(); ++index)
        {
            if (index == 0
                || run.pictures[index] != run.pictures[index - 1])
            {
                ++distinct;
            }
        }

        return distinct;
    }
}

TEST(HoverRecordingTest, Recording_IsIdenticalWithAndWithoutHover)
{
    const auto plain = drive(false);
    const auto hovering = drive(true);

    ASSERT_EQ(2U, plain.recorded.size());
    EXPECT_EQ(plain.recorded, hovering.recorded);
}

TEST(HoverRecordingTest, Recording_HoldsNoneOfTheMotionTheHoverFollowed)
{
    const auto hovering = drive(true);

    ASSERT_EQ(2U, hovering.recorded.size());

    EXPECT_EQ("input.pointer_move", hovering.recorded.at(0).event.name);
    EXPECT_EQ("input.pointer_down", hovering.recorded.at(1).event.name);
    EXPECT_EQ(Tick{6}, hovering.recorded.at(0).tick);
    EXPECT_EQ(Tick{6}, hovering.recorded.at(1).tick);
}

TEST(HoverRecordingTest, Picture_FollowsThePointerTheRecordingNeverSaw)
{
    const auto plain = drive(false);
    const auto hovering = drive(true);

    EXPECT_EQ(1U, distinctPictures(plain));

    EXPECT_LT(1U, distinctPictures(hovering));
}
