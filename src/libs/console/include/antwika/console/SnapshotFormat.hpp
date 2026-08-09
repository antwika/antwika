#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/console/SnapshotError.hpp"

namespace antwika::console
{

    struct Snapshot final
    {
        std::vector<std::string> console;

        nlohmann::json state = nlohmann::json::object();

        [[nodiscard]] bool operator==(
            const Snapshot &other) const = default;
    };

    class SnapshotFormat final
    {
    public:
        SnapshotFormat(
            antwika::config::Format format,
            std::string_view title,
            antwika::replay::MigrationChain (*migrations)());

        [[nodiscard]] nlohmann::json toJson(
            const Snapshot &snapshot) const;

        [[nodiscard]] Snapshot fromJson(
            const nlohmann::json &document) const;

        void write(
            const Snapshot &snapshot, const std::string &path) const;

        [[nodiscard]] Snapshot read(const std::string &path) const;

    private:
        antwika::config::FileFormat<Snapshot, SnapshotError> format;
    };

}
