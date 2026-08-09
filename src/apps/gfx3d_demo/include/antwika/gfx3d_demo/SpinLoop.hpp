#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <antwika/time/ISleeper.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/WindowDesc.hpp>

#include "antwika/gfx3d_demo/SpinScene.hpp"

namespace antwika::gfx3d_demo
{

    using antwika::time::ISleeper;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::MeshData;
    using antwika::gfx::WindowDesc;

    class SpinLoop final
    {
    public:
        SpinLoop(
            IGfxBackend &backend,
            const SpinScene &scene,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        SpinLoop(const SpinLoop &) = delete;
        SpinLoop(SpinLoop &&) = delete;

        SpinLoop &operator=(const SpinLoop &) = delete;
        SpinLoop &operator=(SpinLoop &&) = delete;

        void run(
            const WindowDesc &desc,
            const MeshData &cube,
            std::optional<std::uint32_t> maxFrames);

        [[nodiscard]] std::uint64_t ticks() const noexcept;

    private:
        IGfxBackend &backend;
        const SpinScene &scene;

        std::uint64_t tickCount = 0;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
    };

}
