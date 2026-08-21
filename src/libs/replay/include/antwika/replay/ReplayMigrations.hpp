#pragma once

#include <antwika/schema/MigrationChain.hpp>

namespace antwika::replay
{

    using schema::MigrationChain;

    [[nodiscard]] MigrationChain standardReplayMigrations();

}
