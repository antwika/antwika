#pragma once

#include <raylib.h>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibTexture final : public ITexture
    {
    public:
        RaylibTexture(
            RaylibRenderer &owner, ::Texture2D texture, Size size);

        RaylibTexture(const RaylibTexture &) = delete;
        RaylibTexture(RaylibTexture &&) = delete;

        RaylibTexture &operator=(const RaylibTexture &) = delete;
        RaylibTexture &operator=(RaylibTexture &&) = delete;

        ~RaylibTexture() override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] bool belongsTo(
            const RaylibRenderer &candidate) const noexcept;

        [[nodiscard]] const ::Texture2D &raw() const noexcept;

        [[nodiscard]] bool isLoaded() const noexcept;

        void forgetRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        ::Texture2D texture;
        Size textureSize;
        bool loaded = true;
    };

}
