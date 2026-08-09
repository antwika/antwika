#pragma once

#include <optional>
#include <string>

#include <antwika/app/FileSnapshotStore.hpp>

#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    class FilePetStore final : public IPetStore
    {
    public:
        explicit FilePetStore(std::string path);

        [[nodiscard]] std::optional<CompanionMemory> load() override;

        void save(const CompanionMemory &memory) override;

    private:
        antwika::app::FileSnapshotStore<CompanionMemory, SaveFormatError>
            file;
    };

}
