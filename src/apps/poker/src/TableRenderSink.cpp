#include "antwika/poker/TableRenderSink.hpp"

#include <antwika/ui/Painter.hpp>

#include <functional>
#include <optional>
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

        auto &renderer = window.renderer();
        scene.draw(
            renderer,
            canvas,
            snapshotOf(table, game, tableName),
            atlas);

        // The console last of all, when one is mounted.
        // Described in the tick path; painted here only.
        if (consolePicture.has_value())
        {
            antwika::ui::paint(
                renderer, consolePicture->get().commands());
        }

        renderer.present();

        // Poker at one step per tick is unwatchable without this.
        sleeper.sleep(framePeriod);
    }

} // namespace antwika::poker
