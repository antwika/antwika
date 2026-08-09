#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/gfx_demo/DemoScene.hpp"

namespace antwika::gfx_demo
{

    using antwika::time::ISleeper;
    using antwika::gfx::Bitmap;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::WindowDesc;
    using antwika::input::IInputBackend;

    class DemoLoop final
    {
    public:
        DemoLoop(
            IGfxBackend &backend,
            IInputBackend &input,
            const DemoScene &scene,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        DemoLoop(const DemoLoop &) = delete;
        DemoLoop(DemoLoop &&) = delete;

        DemoLoop &operator=(const DemoLoop &) = delete;
        DemoLoop &operator=(DemoLoop &&) = delete;

        void run(
            const WindowDesc &desc,
            const Bitmap &logo,
            std::optional<std::uint32_t> maxFrames);

        [[nodiscard]] std::uint32_t clicks() const noexcept;

    private:
        IGfxBackend &backend;
        IInputBackend &input;
        const DemoScene &scene;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;

        std::uint32_t clickCount = 0;
    };

}
