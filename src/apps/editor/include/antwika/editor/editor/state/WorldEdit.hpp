#pragma once

#include <cstdint>

#include <antwika/solver/VoxelWeave.hpp>

namespace antwika::editor
{

    class WorldEdit final
    {
    public:
        [[nodiscard]] std::int32_t getEditLevel() const noexcept;

        void setEditLevel(std::int32_t level) noexcept;

        void stepLevelUp() noexcept;

        void stepLevelDown() noexcept;

        [[nodiscard]] solver::CornerSeams getCornerJoining() const noexcept;

        [[nodiscard]] bool isCornerJoiningOn() const noexcept;

        void toggleCornerJoining() noexcept;

        void setCornerJoining(bool cornersJoined) noexcept;

        [[nodiscard]] float getRiseAxis() const noexcept;

        [[nodiscard]] bool isRiseHeld() const noexcept;

        void setAscendHeld(bool held) noexcept;

        void setDescendHeld(bool held) noexcept;

        bool lowerSight = true;

        bool lowerLight = true;

    private:
        std::int32_t editLevel = 0;

        solver::CornerSeams cornerJoining = solver::CornerSeams::Ignored;

        bool ascendHeld = false;

        bool descendHeld = false;
    };

}
