#include "antwika/task_worker/PoolScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/MessageId.hpp"
#include "antwika/task_worker/Messages.hpp"

namespace antwika::task_worker
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    namespace
    {

        constexpr std::uint32_t kScale = 2;

        constexpr std::int32_t kLine = static_cast<std::int32_t>(
            antwika::gfx::kGlyphLineHeight * kScale);
        constexpr std::int32_t kMargin = kLine;
        constexpr std::int32_t kPad = kLine / 2;
        constexpr std::int32_t kBarHeight = kLine / 2;
        constexpr std::int32_t kCardHeight = 2 * kLine + kBarHeight
                                             + 3 * kPad;
        constexpr std::int32_t kCardGap = kPad;
        constexpr std::int32_t kRowHeight = kLine + kPad;
        constexpr std::int32_t kBodyTop = 2 * kMargin + kLine;

        constexpr std::uint32_t kPoolNumerator = 3;
        constexpr std::uint32_t kPoolDenominator = 5;

        constexpr Color kBackground{.red = 14, .green = 16, .blue = 22};
        constexpr Color kPanel{.red = 26, .green = 30, .blue = 42};
        constexpr Color kIdleCard{.red = 32, .green = 36, .blue = 50};
        constexpr Color kBusyCard{.red = 30, .green = 52, .blue = 48};
        constexpr Color kHeading{.red = 140, .green = 152, .blue = 178};
        constexpr Color kInk{.red = 224, .green = 230, .blue = 240};
        constexpr Color kMuted{.red = 132, .green = 142, .blue = 162};
        constexpr Color kBudgetInk{.red = 240, .green = 202, .blue = 112};
        constexpr Color kBarTrack{.red = 48, .green = 54, .blue = 72};
        constexpr Color kBarFill{.red = 96, .green = 208, .blue = 144};
        constexpr Color kBlockedInk{.red = 206, .green = 148, .blue = 88};
        constexpr Color kDoneInk{.red = 104, .green = 148, .blue = 122};

        struct Columns final
        {
            std::int32_t poolX{0};
            std::uint32_t poolWidth{0};
            std::int32_t queueX{0};
            std::uint32_t queueWidth{0};
        };

        [[nodiscard]] Columns columnsFor(Size canvas)
        {
            const auto usable =
                canvas.width - static_cast<std::uint32_t>(3 * kMargin);
            const auto poolWidth =
                usable * kPoolNumerator / kPoolDenominator;
            const auto queueX = 2 * kMargin
                                + static_cast<std::int32_t>(poolWidth);

            return Columns{
                .poolX = kMargin,
                .poolWidth = poolWidth,
                .queueX = queueX,
                .queueWidth = usable - poolWidth};
        }

        [[nodiscard]] std::string worded(
            const Translator &translator,
            MessageId id,
            std::string_view first)
        {
            const std::array<std::string_view, 1> args{first};
            return translator.formatted(id, args);
        }

        [[nodiscard]] std::string worded(
            const Translator &translator,
            MessageId id,
            std::string_view first,
            std::string_view second)
        {
            const std::array<std::string_view, 2> args{first, second};
            return translator.formatted(id, args);
        }

        [[nodiscard]] std::string priorityText(const TaskView &task)
        {
            return std::to_string(
                antwika::scheduler::rawValue(task.priority));
        }

        [[nodiscard]] std::uint32_t filledWidth(
            std::uint32_t track, const WorkerView &worker)
        {
            if (worker.durationTicks == 0
                || worker.remainingTicks >= worker.durationTicks)
            {
                return 0;
            }

            const auto done =
                worker.durationTicks - worker.remainingTicks;
            return static_cast<std::uint32_t>(
                track * done / worker.durationTicks);
        }

        void drawHeader(
            IRenderer &renderer,
            const Translator &translator,
            Size canvas,
            const PoolSnapshot &snapshot)
        {
            const auto column = static_cast<std::int32_t>(
                (canvas.width - static_cast<std::uint32_t>(2 * kMargin))
                / 3);

            renderer.drawText(
                Point{.x = kMargin, .y = kMargin},
                worded(
                    translator,
                    MessageId::Tick,
                    std::to_string(snapshot.tick)),
                kScale,
                kInk);
            renderer.drawText(
                Point{.x = kMargin + column, .y = kMargin},
                worded(
                    translator,
                    MessageId::Budget,
                    std::to_string(snapshot.dispatch.budget)),
                kScale,
                kBudgetInk);
            renderer.drawText(
                Point{.x = kMargin + 2 * column, .y = kMargin},
                worded(
                    translator,
                    MessageId::Started,
                    std::to_string(snapshot.dispatch.dispatched)),
                kScale,
                kMuted);
        }

        void drawWorkerCard(
            IRenderer &renderer,
            const Translator &translator,
            const Columns &columns,
            std::int32_t top,
            std::size_t index,
            const WorkerView &worker)
        {
            const bool busy = worker.status == WorkerStatus::Busy;

            renderer.drawRect(
                Rect{
                    .origin = {.x = columns.poolX, .y = top},
                    .size = {
                        .width = columns.poolWidth,
                        .height = static_cast<std::uint32_t>(
                            kCardHeight)}},
                busy ? kBusyCard : kIdleCard);

            if (!busy)
            {
                renderer.drawText(
                    Point{
                        .x = columns.poolX + kPad, .y = top + kPad},
                    worded(
                        translator,
                        MessageId::WorkerIdle,
                        std::to_string(index)),
                    kScale,
                    kMuted);
                return;
            }

            renderer.drawText(
                Point{.x = columns.poolX + kPad, .y = top + kPad},
                worded(
                    translator,
                    MessageId::WorkerBusy,
                    std::to_string(index),
                    worker.label),
                kScale,
                kInk);
            renderer.drawText(
                Point{
                    .x = columns.poolX + kPad,
                    .y = top + kPad + kLine},
                worded(
                    translator,
                    MessageId::TicksLeft,
                    std::to_string(worker.remainingTicks),
                    std::to_string(worker.durationTicks)),
                kScale,
                kMuted);

            const auto track =
                columns.poolWidth - static_cast<std::uint32_t>(2 * kPad);
            const auto barTop = top + 2 * kPad + 2 * kLine;

            renderer.drawRect(
                Rect{
                    .origin = {.x = columns.poolX + kPad, .y = barTop},
                    .size = {
                        .width = track,
                        .height =
                            static_cast<std::uint32_t>(kBarHeight)}},
                kBarTrack);
            renderer.drawRect(
                Rect{
                    .origin = {.x = columns.poolX + kPad, .y = barTop},
                    .size = {
                        .width = filledWidth(track, worker),
                        .height =
                            static_cast<std::uint32_t>(kBarHeight)}},
                kBarFill);
        }

        void drawPool(
            IRenderer &renderer,
            const Translator &translator,
            const Columns &columns,
            const PoolSnapshot &snapshot)
        {
            renderer.drawText(
                Point{.x = columns.poolX, .y = kBodyTop},
                translator.text(MessageId::Workers),
                kScale,
                kHeading);

            auto top = kBodyTop + kLine + kPad;

            for (std::size_t index = 0; index < snapshot.workers.size();
                 ++index)
            {
                drawWorkerCard(
                    renderer,
                    translator,
                    columns,
                    top,
                    index,
                    snapshot.workers[index]);
                top += kCardHeight + kCardGap;
            }
        }

        void drawQueue(
            IRenderer &renderer,
            const Translator &translator,
            const Columns &columns,
            Size canvas,
            const PoolSnapshot &snapshot)
        {
            renderer.drawRect(
                Rect{
                    .origin = {
                        .x = columns.queueX, .y = kBodyTop - kPad},
                    .size = {
                        .width = columns.queueWidth,
                        .height = canvas.height
                                  - static_cast<std::uint32_t>(
                                      kBodyTop - kPad + kMargin)}},
                kPanel);

            renderer.drawText(
                Point{.x = columns.queueX + kPad, .y = kBodyTop},
                translator.text(MessageId::Queue),
                kScale,
                kHeading);

            auto top = kBodyTop + kLine + kPad;

            for (const auto &task : snapshot.queue)
            {
                const bool blocked = task.blocked;
                const auto line =
                    blocked ? worded(
                                  translator,
                                  MessageId::Blocked,
                                  task.label,
                                  task.waitingFor)
                            : worded(
                                  translator,
                                  MessageId::Queued,
                                  task.label,
                                  priorityText(task)); // GCOVR_EXCL_LINE

                renderer.drawText(
                    Point{.x = columns.queueX + kPad, .y = top},
                    line,
                    kScale,
                    blocked ? kBlockedInk : kInk);
                top += kRowHeight;
            }

            top += kLine;

            renderer.drawText(
                Point{.x = columns.queueX + kPad, .y = top},
                translator.text(MessageId::Completed),
                kScale,
                kHeading);
            top += kLine + kPad;

            for (const auto &task : snapshot.completed)
            {
                renderer.drawText(
                    Point{.x = columns.queueX + kPad, .y = top},
                    task.label,
                    kScale,
                    kDoneInk);
                top += kRowHeight;
            }
        }

    }

    PoolScene::PoolScene(const Translator &translator)
        : translator(translator)
    {
    }

    void PoolScene::draw(
        IRenderer &renderer,
        Size canvas,
        const PoolSnapshot &snapshot) const
    {
        renderer.clear(kBackground);

        if (canvas.width < kMinCanvasWidth
            || canvas.height < kMinCanvasHeight)
        {
            return;
        }

        const auto columns = columnsFor(canvas);

        drawHeader(renderer, translator, canvas, snapshot);
        drawPool(renderer, translator, columns, snapshot);
        drawQueue(renderer, translator, columns, canvas, snapshot);
    }

}
