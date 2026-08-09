#pragma once

#include <chrono>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::poker
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::Size;
    using antwika::time::ISleeper;

    struct WindowSetup final
    {
        IGfxBackend &backend;

        ISleeper &sleeper;

        std::chrono::milliseconds framePeriod{};

        bool holdFinalFrame{false};

        const Bitmap *atlas = nullptr;

        Size size{.width = 1024, .height = 640};
    };

}
