#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::scheduler::kLowPriority;
    using antwika::scheduler::kNormalPriority;
    using antwika::task_worker::PoolScene;
    using antwika::task_worker::PoolSnapshot;
    using antwika::task_worker::TaskView;
    using antwika::task_worker::WorkerStatus;
    using antwika::task_worker::WorkerView;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 960, .height = 600};

    [[nodiscard]] PoolSnapshot busyPool()
    {
        return PoolSnapshot{
            .tick = 42,
            .dispatch = {.budget = 2, .dispatched = 2},
            .workers =
                {WorkerView{WorkerStatus::Busy, 1, "Alpha", 4, 1},
                 WorkerView{WorkerStatus::Busy, 4, "Delta", 6, 3},
                 WorkerView{}},
            .queue =
                {TaskView{3, "Gamma", kLowPriority, 2, false, ""},
                 TaskView{5, "Epsilon", kNormalPriority, 1, true, "Delta"}},
            .completed = {
                TaskView{2, "Beta", kNormalPriority, 5, false, ""}}};
    }
}

TEST(PoolDrawTest, Draw_DrawsAPoolAtWork)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const antwika::task_worker::Translator translator{
        antwika::i18n::kDefaultLocale};

    const PoolScene scene(translator);

    scene.draw(renderer, kCanvas, busyPool());
}
