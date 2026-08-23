#pragma once

#include <cstddef>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/KeyFrame.hpp"
#include "antwika/animation/LoopMode.hpp"

namespace antwika::animation
{

    class Clip final
    {
    public:
        explicit Clip(
            std::vector<KeyFrame> keyFrames,
            LoopMode loop = LoopMode::Loop);

        [[nodiscard]] const std::vector<KeyFrame> &getFrames() const noexcept;

        [[nodiscard]] LoopMode getLoop() const noexcept;

        [[nodiscard]] time::Tick getDurationTicks() const noexcept;

    private:
        std::vector<KeyFrame> keyFrames;
        LoopMode loopMode;
        time::Tick durationTick;
    };

    [[nodiscard]] Clip getUniformClip(
        std::size_t firstIndex,
        std::size_t frameCount,
        time::Tick ticksPerFrame,
        LoopMode loop = LoopMode::Loop);

}
