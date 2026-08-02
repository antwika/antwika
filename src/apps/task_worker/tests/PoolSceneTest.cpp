#include "antwika/task_worker/PoolScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::gfx::Color;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::i18n::kDefaultLocale;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::kMinCanvasHeight;
using antwika::task_worker::kMinCanvasWidth;
using antwika::task_worker::PoolScene;
using antwika::task_worker::PoolSnapshot;
using antwika::task_worker::TaskView;
using antwika::task_worker::Translator;
using antwika::task_worker::WorkerStatus;
using antwika::task_worker::WorkerView;
using ::testing::NiceMock;

namespace
{
    // The size main.cpp asks its window for.
    constexpr Size kCanvas{.width = 960, .height = 600};

    constexpr Color kBackground{.red = 14, .green = 16, .blue = 22};
    constexpr Color kBarTrack{.red = 48, .green = 54, .blue = 72};
    constexpr Color kBarFill{.red = 96, .green = 208, .blue = 144};

    struct DrawnText
    {
        Point origin{};
        std::string text;
        Color color{};
    };

    struct DrawnRect
    {
        Rect rect{};
        Color color{};
    };

    struct Drawn
    {
        Color cleared{};
        std::vector<DrawnRect> rects;
        std::vector<DrawnText> texts;
    };

    // One recorder rather than an expectation per call.
    // What is worth asserting is what ended up in the picture.
    // And in which order, rather than that one call happened.
    void record(NiceMock<MockRenderer> &renderer, Drawn &drawn)
    {
        ON_CALL(renderer, clear)
            .WillByDefault([&drawn](Color color)
                           { drawn.cleared = color; });
        ON_CALL(renderer, drawRect)
            .WillByDefault(
                [&drawn](Rect rect, Color color)
                { drawn.rects.push_back(DrawnRect{rect, color}); });
        ON_CALL(renderer, drawText)
            .WillByDefault(
                [&drawn](
                    Point origin,
                    std::string_view text,
                    std::uint32_t,
                    Color color)
                {
                    drawn.texts.push_back(
                        DrawnText{origin, std::string{text}, color});
                });
    }

    [[nodiscard]] std::vector<std::string> textsOf(const Drawn &drawn)
    {
        std::vector<std::string> lines;

        for (const auto &text : drawn.texts)
        {
            lines.push_back(text.text);
        }

        return lines;
    }

    [[nodiscard]] bool drew(const Drawn &drawn, std::string_view line)
    {
        const auto lines = textsOf(drawn);
        return std::find(lines.begin(), lines.end(), line) != lines.end();
    }

    [[nodiscard]] const DrawnText *lineSaying(
        const Drawn &drawn, std::string_view line)
    {
        const auto it = std::find_if(
            drawn.texts.begin(),
            drawn.texts.end(),
            [line](const DrawnText &text) { return text.text == line; });
        return it != drawn.texts.end() ? &(*it) : nullptr;
    }

    [[nodiscard]] PoolSnapshot demoSnapshot()
    {
        return PoolSnapshot{
            .tick = 4,
            .dispatch = {.budget = 1, .dispatched = 1},
            .workers =
                {WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 1},
                 WorkerView{}},
            .queue =
                {TaskView{3, "Gamma", kLowPriority, 2, false, ""},
                 TaskView{
                     5, "Epsilon", kNormalPriority, 1, true, "Delta"}},
            .completed = {
                TaskView{2, "Beta", kNormalPriority, 5, false, ""}}};
    }
} // namespace

TEST(PoolSceneTest, Draw_ClearsAndDrawsTheWholeReadout)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    scene.draw(renderer, kCanvas, demoSnapshot());

    EXPECT_EQ(drawn.cleared, kBackground);
    EXPECT_TRUE(drew(drawn, "tick 4"));
    EXPECT_TRUE(drew(drawn, "budget 1"));
    EXPECT_TRUE(drew(drawn, "started 1"));
    EXPECT_TRUE(drew(drawn, "workers"));
    EXPECT_TRUE(drew(drawn, "queue"));
    EXPECT_TRUE(drew(drawn, "completed"));
}

TEST(PoolSceneTest, Draw_SaysWhichTaskAWorkerHoldsAndHowFarThroughItIs)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    scene.draw(renderer, kCanvas, demoSnapshot());

    EXPECT_TRUE(drew(drawn, "worker 0 Alpha"));
    EXPECT_TRUE(drew(drawn, "1 of 4 ticks left"));
    EXPECT_TRUE(drew(drawn, "worker 1 idle"));
}

// The bar is the one measurement in the picture, in whole pixels.
// Three quarters through a four-tick task fills three quarters of it.
TEST(PoolSceneTest, Draw_FillsABusyWorkersBarInProportionToItsProgress)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    scene.draw(renderer, kCanvas, demoSnapshot());

    const auto track = std::find_if(
        drawn.rects.begin(),
        drawn.rects.end(),
        [](const DrawnRect &rect) { return rect.color == kBarTrack; });
    const auto fill = std::find_if(
        drawn.rects.begin(),
        drawn.rects.end(),
        [](const DrawnRect &rect) { return rect.color == kBarFill; });

    ASSERT_NE(track, drawn.rects.end());
    ASSERT_NE(fill, drawn.rects.end());
    EXPECT_EQ(fill->rect.origin, track->rect.origin);
    EXPECT_EQ(fill->rect.size.width, track->rect.size.width * 3 / 4);
}

TEST(PoolSceneTest, Draw_LeavesTheBarEmptyForATaskThatHasNotMovedYet)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    auto snapshot = demoSnapshot();
    snapshot.workers[0] =
        WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 4};

    scene.draw(renderer, kCanvas, snapshot);

    const auto fill = std::find_if(
        drawn.rects.begin(),
        drawn.rects.end(),
        [](const DrawnRect &rect) { return rect.color == kBarFill; });

    ASSERT_NE(fill, drawn.rects.end());
    EXPECT_EQ(fill->rect.size.width, 0U);
}

TEST(PoolSceneTest, Draw_LeavesTheBarEmptyForATaskWithNoDurationOnRecord)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    auto snapshot = demoSnapshot();
    snapshot.workers[0] =
        WorkerView{WorkerStatus::Busy, 1, "Ghost", 0, 2};

    scene.draw(renderer, kCanvas, snapshot);

    const auto fill = std::find_if(
        drawn.rects.begin(),
        drawn.rects.end(),
        [](const DrawnRect &rect) { return rect.color == kBarFill; });

    ASSERT_NE(fill, drawn.rects.end());
    EXPECT_EQ(fill->rect.size.width, 0U);
}

// The queue is drawn top to bottom in the order the snapshot holds.
// That is the order antwika::scheduler will pull them in.
// A blocked task says what it waits for instead of a priority.
// Its priority is not what is keeping it out.
TEST(PoolSceneTest, Draw_ListsTheQueueDownwardsInPullOrder)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    scene.draw(renderer, kCanvas, demoSnapshot());

    const auto *first = lineSaying(drawn, "Gamma priority 0");
    const auto *second = lineSaying(drawn, "Epsilon waits for Delta");

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first->origin.x, second->origin.x);
    EXPECT_LT(first->origin.y, second->origin.y);
    EXPECT_NE(first->color, second->color);
}

TEST(PoolSceneTest, Draw_ListsAFinishedTaskUnderItsOwnHeading)
{
    NiceMock<MockRenderer> renderer;
    Drawn drawn;
    record(renderer, drawn);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    scene.draw(renderer, kCanvas, demoSnapshot());

    const auto *heading = lineSaying(drawn, "completed");
    const auto *finished = lineSaying(drawn, "Beta");

    ASSERT_NE(heading, nullptr);
    ASSERT_NE(finished, nullptr);
    EXPECT_LT(heading->origin.y, finished->origin.y);
}

TEST(PoolSceneTest, Draw_LeavesACanvasWithNoRoomAtTheBackgroundAlone)
{
    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    for (const Size canvas :
         {Size{.width = kMinCanvasWidth - 1, .height = kCanvas.height},
          Size{.width = kCanvas.width, .height = kMinCanvasHeight - 1}})
    {
        NiceMock<MockRenderer> renderer;
        Drawn drawn;
        record(renderer, drawn);

        scene.draw(renderer, canvas, demoSnapshot());

        EXPECT_EQ(drawn.cleared, kBackground);
        EXPECT_TRUE(drawn.rects.empty());
        EXPECT_TRUE(drawn.texts.empty());
    }
}

// The same snapshot draws the same picture twice.
// Which is what lets a frame be asserted call by call.
TEST(PoolSceneTest, Draw_IsTheSamePictureForTheSameSnapshot)
{
    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};

    NiceMock<MockRenderer> firstRenderer;
    Drawn first;
    record(firstRenderer, first);
    scene.draw(firstRenderer, kCanvas, demoSnapshot());

    NiceMock<MockRenderer> secondRenderer;
    Drawn second;
    record(secondRenderer, second);
    scene.draw(secondRenderer, kCanvas, demoSnapshot());

    ASSERT_EQ(first.rects.size(), second.rects.size());
    ASSERT_EQ(first.texts.size(), second.texts.size());

    for (std::size_t index = 0; index < first.rects.size(); ++index)
    {
        EXPECT_EQ(first.rects[index].rect, second.rects[index].rect);
        EXPECT_EQ(first.rects[index].color, second.rects[index].color);
    }

    EXPECT_EQ(textsOf(first), textsOf(second));
}
