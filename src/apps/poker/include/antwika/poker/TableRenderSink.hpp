#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/console/ConsolePicture.hpp>

#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/TableScene.hpp"

namespace antwika::poker
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::ITexture;

    using antwika::gfx::IWindow;
    using antwika::gfx::Size;
    using antwika::holdem::Table;
    using antwika::time::ISleeper;

    class TableRenderSink final : public ITickEventSink
    {
    public:
        TableRenderSink(
            IWindow &window,
            Size canvas,
            const TableScene &scene,
            const Table &table,
            const CashGame &game,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod,
            std::string tableName,
            OptionalAtlas atlas = std::nullopt,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                consolePicture = std::nullopt);

        TableRenderSink(const TableRenderSink &) = delete;
        TableRenderSink(TableRenderSink &&) = delete;

        TableRenderSink &operator=(const TableRenderSink &) = delete;
        TableRenderSink &operator=(TableRenderSink &&) = delete;

        void handle(const TickEvent &event) override;

        void render() const;

    private:
        IWindow &window;
        Size canvas;
        const TableScene &scene;
        const Table &table;
        const CashGame &game;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
        std::string tableName;
        OptionalAtlas atlas;
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consolePicture;
    };

}
