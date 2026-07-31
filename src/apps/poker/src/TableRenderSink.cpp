#include "antwika/poker/TableRenderSink.hpp"

#include <utility>

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
        const ITexture *atlas)
        : window(window),
          canvas(canvas),
          scene(scene),
          table(table),
          game(game),
          sleeper(sleeper),
          framePeriod(framePeriod),
          tableName(std::move(tableName)),
          atlas(atlas)
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

        auto &renderer = window.renderer();
        scene.draw(
            renderer,
            canvas,
            snapshotOf(table, game, tableName),
            atlas);
        renderer.present();

        // Poker at one step per tick is unwatchable without this.
        sleeper.sleep(framePeriod);
    }

} // namespace antwika::poker
