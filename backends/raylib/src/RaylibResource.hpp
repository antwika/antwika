#pragma once

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibResource
    {
    public:
        RaylibResource(const RaylibResource &) = delete;
        RaylibResource(RaylibResource &&) = delete;

        RaylibResource &operator=(const RaylibResource &) = delete;
        RaylibResource &operator=(RaylibResource &&) = delete;

        virtual ~RaylibResource();

        [[nodiscard]] bool isOwnedBy(
            const RaylibRenderer &candidateRenderer) const noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;

        void unload() noexcept;

        void untrackRenderer() noexcept;

    protected:
        explicit RaylibResource(RaylibRenderer &trackingRenderer);

    private:
        virtual void unloadHandle() noexcept;

        RaylibRenderer *ownerRenderer;
        bool loaded = true;
    };

}
