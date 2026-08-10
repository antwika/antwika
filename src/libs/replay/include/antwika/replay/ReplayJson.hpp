#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/ReplayDocument.hpp>
#include <antwika/replay/ReplayHeader.hpp>

namespace antwika::replay
{

    [[nodiscard]] nlohmann::json replayHeaderToJson(
        const ReplayHeader &header);

    [[nodiscard]] ReplayHeader replayHeaderFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

    [[nodiscard]] std::vector<event::TickEvent> replayRecordsFromJson(
        const nlohmann::json &records,
        std::uint32_t version,
        const MigrationChain &migrations);

    [[nodiscard]] ReplayDocument replayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

}
