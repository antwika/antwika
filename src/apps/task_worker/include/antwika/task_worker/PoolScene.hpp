#pragma once

#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"

namespace antwika::task_worker
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    class PoolScene final
    {
    public:
        explicit PoolScene(const Translator &translator);

        void draw(
            IRenderer &renderer,
            Size canvas,
            const PoolSnapshot &snapshot) const;

    private:
        const Translator &translator;
    };

    inline constexpr std::uint32_t kMinCanvasWidth = 480;

    inline constexpr std::uint32_t kMinCanvasHeight = 240;

}
