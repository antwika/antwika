#pragma once

namespace antwika::gfx
{

    class ISurfaceRenderer;

    class ClipScope final
    {
    public:
        ~ClipScope();

        ClipScope(const ClipScope &) = delete;
        ClipScope(ClipScope &&) = delete;

        ClipScope &operator=(const ClipScope &) = delete;
        ClipScope &operator=(ClipScope &&) = delete;

    private:
        friend class ISurfaceRenderer;

        explicit ClipScope(ISurfaceRenderer &renderer) noexcept;

        ISurfaceRenderer &renderer;
    };

}
