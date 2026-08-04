#include "antwika/task_worker/RenderSystem.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Priority.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::gfx::Color;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::i18n::kDefaultLocale;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::JobId;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::PoolScene;
using antwika::task_worker::RenderSystem;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::Translator;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kConfigured{.width = 960, .height = 600};

    // Deliberately too small for the scene to draw anything into.
    // So a layout taken from it would be provably the wrong one.
    constexpr Size kReported{.width = 200, .height = 200};

    void seedOneBusyWorker(World &world, TaskRegistry &registry)
    {
        const auto entity = world.create();
        world.add<Worker>(
            entity, Worker{WorkerStatus::Busy, 2, 1, makeName("Alpha")});
        world.commit();

        registry.submit(1, "Alpha", kNormalPriority, 4);
        registry.markStarted(static_cast<JobId>(1));
        registry.noteDispatch(1, 1);
    }
} // namespace

TEST(RenderSystemTest, Update_DrawsThisTicksPoolAndPresents)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    TaskRegistry registry;
    seedOneBusyWorker(world, registry);

    std::vector<std::string> lines;
    NiceMock<MockRenderer> renderer;
    ON_CALL(renderer, drawText)
        .WillByDefault(
            [&lines](
                Point, std::string_view text, std::uint32_t, Color)
            { lines.push_back(std::string{text}); });

    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize()).WillByDefault(Return(kConfigured));
    ON_CALL(window, size()).WillByDefault(Return(kReported));
    EXPECT_CALL(renderer, present()).Times(1);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};
    RenderSystem system(window, scene, registry);

    system.update(world, 3);

    EXPECT_THAT(lines, ::testing::Contains("tick 3"));
    EXPECT_THAT(lines, ::testing::Contains("worker 0 Alpha"));
    EXPECT_THAT(lines, ::testing::Contains("2 of 4 ticks left"));
}

// The layout is the configured size's, never the reported one's.
// The reported size here is too small to draw into at all.
// So any text at all can only have come from the configured size.
TEST(RenderSystemTest, Update_LaysOutAgainstTheConfiguredWindowSize)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    TaskRegistry registry;
    seedOneBusyWorker(world, registry);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize()).WillByDefault(Return(kConfigured));
    ON_CALL(window, size()).WillByDefault(Return(kReported));
    EXPECT_CALL(renderer, drawText).Times(::testing::AtLeast(1));

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};
    RenderSystem system(window, scene, registry);

    system.update(world, 0);
}

// The console's picture is painted last, over the pool.
// Whatever the tick path described is what lands on screen.
TEST(RenderSystemTest, Update_PaintsTheConsoleOverlayWhenGivenOne)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    TaskRegistry registry;
    seedOneBusyWorker(world, registry);

    NiceMock<MockRenderer> renderer;
    NiceMock<MockWindow> window;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, configuredSize()).WillByDefault(Return(kConfigured));
    ON_CALL(window, size()).WillByDefault(Return(kReported));

    antwika::console::ConsolePicture overlay(kConfigured);
    const antwika::ui::FillRect sheet{
        .rect = {
            .origin = {.x = 0, .y = 0},
            .size = {.width = 960, .height = 300}},
        .color = {.red = 1, .green = 2, .blue = 3, .alpha = 4}};
    overlay.set(antwika::ui::DrawList{sheet});

    // The scene draws what it draws; the sheet must be among it all.
    EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());
    EXPECT_CALL(renderer, drawRect(sheet.rect, sheet.color)).Times(1);
    EXPECT_CALL(renderer, present()).Times(1);

    const Translator translator{kDefaultLocale};
    const PoolScene scene{translator};
    RenderSystem system(window, scene, registry, overlay);

    system.update(world, 0);
}
