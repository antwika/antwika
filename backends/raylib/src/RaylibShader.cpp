#include "RaylibShader.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibShader::RaylibShader(RaylibRenderer &ownerRenderer, ::Shader shader)
        : owner(&ownerRenderer),
          shader(shader)
    {
        ownerRenderer.trackShader(*this);
    }

    RaylibShader::~RaylibShader()
    {
        if (owner != nullptr)
        {
            owner->untrackShader(*this);
        }

        if (loaded)
        {
            UnloadShader(shader);
        }
    }

    bool RaylibShader::isReady() const
    {
        return loaded;
    }

    bool RaylibShader::isOwnedBy(
        const RaylibRenderer &candidateRenderer) const noexcept
    {
        return owner == &candidateRenderer;
    }

    const ::Shader &RaylibShader::raw() const noexcept
    {
        return shader;
    }

    bool RaylibShader::isLoaded() const noexcept
    {
        return loaded;
    }

    void RaylibShader::untrackRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;
    }

}
