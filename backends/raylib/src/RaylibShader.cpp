#include "RaylibShader.hpp"

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    RaylibShader::RaylibShader(RaylibRenderer &ownerRenderer, ::Shader shader)
        : RaylibResource(ownerRenderer),
          shader(shader)
    {
    }

    RaylibShader::~RaylibShader()
    {
        unload();
    }

    bool RaylibShader::isReady() const
    {
        return isLoaded();
    }

    const ::Shader &RaylibShader::getRawHandle() const noexcept
    {
        return shader;
    }

    void RaylibShader::unloadHandle() noexcept
    {
        UnloadShader(shader);
    }

}
