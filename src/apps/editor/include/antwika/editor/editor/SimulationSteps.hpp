#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/component/Item.hpp>
#include <antwika/gameplay/ICheckpointProgress.hpp>
#include <antwika/gameplay/IWorldAccess.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/editor/EditorDocument.hpp"
#include "antwika/editor/editor/state/Caption.hpp"
#include "antwika/editor/view/IEditSteps.hpp"

namespace antwika::editor
{

    class SimulationSteps final
    {
    public:
        SimulationSteps(
            EditorDocument &document,
            IEditSteps &editSteps,
            const std::uint32_t &tick) noexcept;

        Caption caption;

        /**
         * @brief Takes the checkpoint the pad system reported and says so.
         */
        void sayCheckpointReport(
            gameplay::ICheckpointProgress &checkpointProgress,
            gameplay::IWorldAccess &worldAccess);

        void consumeItem(
            gameplay::IWorldAccess &worldAccess,
            component::ItemKind kind);

        void sayConsumeReport(gameplay::IWorldAccess &worldAccess);

        void sayDialogueLine(gameplay::IWorldAccess &worldAccess);

        void interact(gameplay::IWorldAccess &worldAccess);

        void sayCaption(
            const std::string &name,
            const std::string &line,
            std::optional<std::size_t> speaker = std::nullopt);

    private:
        EditorDocument &document;

        IEditSteps &editSteps;

        const std::uint32_t &tick;
    };

}
