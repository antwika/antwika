#pragma once

namespace antwika::gfx
{

    class IRenderer;

    class TargetScope final
    {
    public:
        ~TargetScope();

        TargetScope(const TargetScope &) = delete;
        TargetScope(TargetScope &&) = delete;

        TargetScope &operator=(const TargetScope &) = delete;
        TargetScope &operator=(TargetScope &&) = delete;

    private:
        friend class IRenderer;

        explicit TargetScope(IRenderer &renderer) noexcept;

        IRenderer &renderer;
    };

}
