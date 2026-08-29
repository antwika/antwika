#pragma once

#include <raylib.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

#include "RaylibResource.hpp"

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibTexture final : public ITexture, public RaylibResource
    {
    public:
        RaylibTexture(
            RaylibRenderer &ownerRenderer,
            ::Texture2D texture,
            Size size,
            bool owns = true);

        ~RaylibTexture() override;

        [[nodiscard]] Size getSize() const override;

        [[nodiscard]] const ::Texture2D &getRawHandle() const noexcept;

    private:
        void unloadHandle() noexcept override;

        ::Texture2D texture;
        Size textureSize;
        bool owned = true;
    };

}
