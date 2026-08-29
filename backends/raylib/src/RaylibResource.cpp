#include "RaylibResource.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibResource::RaylibResource(RaylibRenderer &trackingRenderer)
        : ownerRenderer(&trackingRenderer)
    {
        trackingRenderer.trackResource(*this);
    }

    RaylibResource::~RaylibResource()
    {
        if (ownerRenderer != nullptr)
        {
            ownerRenderer->untrackResource(*this);
        }
    }

    bool RaylibResource::isOwnedBy(
        const RaylibRenderer &candidateRenderer) const noexcept
    {
        return ownerRenderer == &candidateRenderer;
    }

    bool RaylibResource::isLoaded() const noexcept
    {
        return loaded;
    }

    void RaylibResource::unload() noexcept
    {
        if (!loaded)
        {
            return;
        }

        loaded = false;

        unloadHandle();
    }

    void RaylibResource::unloadHandle() noexcept
    {
    }

    void RaylibResource::untrackRenderer() noexcept
    {
        ownerRenderer = nullptr;
        loaded = false;
    }

}
