#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::scheduler::kLowPriority;
    using antwika::scheduler::kNormalPriority;
    using antwika::task_worker::PoolScene;
    using antwika::task_worker::PoolSnapshot;
    using antwika::task_worker::TaskView;
    using antwika::task_worker::WorkerStatus;
    using antwika::task_worker::WorkerView;

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

TEST(PoolPreviewTest, Draw_WritesAPoolAtWork)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "task-worker",
             .title = "Antwika Task Worker",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const antwika::task_worker::Translator translator{
                    antwika::i18n::kDefaultLocale};

                const PoolScene scene(translator);
                scene.draw(renderer, kCanvas, busyPool());
            })
            .empty());
}
