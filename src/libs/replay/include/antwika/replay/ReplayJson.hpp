#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/replay/ReplayDocument.hpp>
#include <antwika/replay/ReplayHeader.hpp>

namespace antwika::replay
{

    using schema::MigrationChain;

    [[nodiscard]] nlohmann::json getReplayHeaderToJson(
        const ReplayHeader &header);

    [[nodiscard]] ReplayHeader getReplayHeaderFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

    [[nodiscard]] std::vector<event::TickEvent> getReplayRecordsFromJson(
        const nlohmann::json &records,
        std::uint32_t version,
        const MigrationChain &migrations);

    [[nodiscard]] ReplayDocument getReplayFromJson(
        const nlohmann::json &j, const MigrationChain &migrations);

}
