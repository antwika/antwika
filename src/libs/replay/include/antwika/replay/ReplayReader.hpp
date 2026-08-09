#pragma once

#include <istream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/CanvasCheck.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/ReplayMigrations.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    class ReplayReader final
    {
    public:
        explicit ReplayReader(
            CanvasCheck check = {},
            MigrationChain migrations = standardReplayMigrations());

        [[nodiscard]] std::vector<TickEvent> read(std::istream &in) const;

    private:
        CanvasCheck check;
        MigrationChain migrations;
    };

}
