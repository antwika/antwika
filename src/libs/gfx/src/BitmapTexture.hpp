#pragma once

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx::detail
{

    class BitmapTexture final : public ITexture
    {
    public:
        explicit BitmapTexture(Bitmap pixels);

        [[nodiscard]] Size size() const override;

        [[nodiscard]] const Bitmap &image() const noexcept;

    private:
        Bitmap held;
    };

}
