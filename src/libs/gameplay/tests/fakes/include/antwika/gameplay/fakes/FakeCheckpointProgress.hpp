#pragma once

#include <string>
#include <utility>

#include <antwika/gameplay/CheckpointState.hpp>
#include <antwika/gameplay/ICheckpointProgress.hpp>
#include <antwika/map/PlayerProgress.hpp>

namespace antwika::gameplay::fakes
{

    class FakeCheckpointProgress final : public ICheckpointProgress
    {
    public:
        [[nodiscard]] const CheckpointState &getCheckpoint()
            const noexcept override
        {
            return checkpoint;
        }

        void setCheckpoint(CheckpointState checkpointGiven) noexcept
            override
        {
            checkpoint = std::move(checkpointGiven);
        }

        [[nodiscard]] map::Progress getProgress(
            std::string mapName) const override
        {
            static_cast<void>(mapName);

            return map::Progress{};
        }

        CheckpointState checkpoint;
    };

}
