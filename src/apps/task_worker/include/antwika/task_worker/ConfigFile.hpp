#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/task_worker/TaskWorkerConfig.hpp"

namespace antwika::task_worker
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kConfigMagic =
        "antwika-task-worker-config";

    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    [[nodiscard]] MigrationChain standardConfigMigrations();

    [[nodiscard]] nlohmann::json configToJson(const TaskWorkerConfig &config);

    [[nodiscard]] TaskWorkerConfig configFromJson(
        const nlohmann::json &document);

    void writeConfig(const TaskWorkerConfig &config, std::ostream &out);

    [[nodiscard]] TaskWorkerConfig readConfig(std::istream &in);

    [[nodiscard]] TaskWorkerConfig loadConfigFileOrDefaults(
        const std::string &path);

}
