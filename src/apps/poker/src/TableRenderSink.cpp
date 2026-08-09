#include "antwika/poker/TableRenderSink.hpp"

#include <functional>
#include <optional>
#include <utility>

#include <antwika/app/FramePresentation.hpp>
#include <antwika/engine/Events.hpp>

#include "antwika/poker/TableSnapshot.hpp"

namespace antwika::poker
{

    TableRenderSink::TableRenderSink(
        IWindow &window,
        Size canvas,
        const TableScene &scene,
        const Table &table,
        const CashGame &game,
        ISleeper &sleeper,
        std::chrono::milliseconds framePeriod,
        std::string tableName,
        OptionalAtlas atlas,
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consolePicture)
        : window(window),
          canvas(canvas),
          scene(scene),
          table(table),
          game(game),
          sleeper(sleeper),
          framePeriod(framePeriod),
          tableName(std::move(tableName)),
          atlas(std::move(atlas)),
          consolePicture(consolePicture)
    {
    }

    void TableRenderSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        render();
    }

    void TableRenderSink::render() const
    {
        if (!window.isOpen())
        {
            return;
        }

        antwika::app::presentFrame(
            window,
            consolePicture,
            [this](antwika::gfx::IRenderer &renderer)
            {
                scene.draw(
                    renderer,
                    canvas,
                    snapshotOf(table, game, tableName),
                    atlas);
            });

        sleeper.sleep(framePeriod);
    }

}
