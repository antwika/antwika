#pragma once

#include <raylib.h>

#include <antwika/gfx/IShader.hpp>

#include "RaylibResource.hpp"

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibShader final : public IShader, public RaylibResource
    {
    public:
        RaylibShader(RaylibRenderer &ownerRenderer, ::Shader shader);

        ~RaylibShader() override;

        [[nodiscard]] bool isReady() const override;

        [[nodiscard]] const ::Shader &getRawHandle() const noexcept;

    private:
        void unloadHandle() noexcept override;

        ::Shader shader;
    };

}
