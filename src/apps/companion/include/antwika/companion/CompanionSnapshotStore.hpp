#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/console/IJsonSnapshotStore.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-companion-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    class CompanionSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<SaveFormatError>
    {
    public:
        CompanionSnapshotStore(Pet &pet, Lineage &lineage) noexcept;

        CompanionSnapshotStore(const CompanionSnapshotStore &) = delete;
        CompanionSnapshotStore(CompanionSnapshotStore &&) = delete;

        CompanionSnapshotStore &operator=(
            const CompanionSnapshotStore &) = delete;
        CompanionSnapshotStore &operator=(
            CompanionSnapshotStore &&) = delete;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        Pet &pet;
        Lineage &lineage;
    };

}
