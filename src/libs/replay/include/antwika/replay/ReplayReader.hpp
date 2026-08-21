#pragma once

#include <istream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/CanvasCheckOptions.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/replay/ReplayMigrations.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    class ReplayReader final
    {
    public:
        explicit ReplayReader(
            CanvasCheckOptions check = {},
            MigrationChain migrations = standardReplayMigrations());

        [[nodiscard]] std::vector<TickEvent> read(
            std::istream &inputStream) const;

    private:
        CanvasCheckOptions check;
        MigrationChain migrations;
    };

}
