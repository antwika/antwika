#pragma once

#include <raylib.h>

#include <antwika/gfx/Color.hpp>

namespace antwika::gfx::raylib
{

    class RaylibMaterial final
    {
    public:
        RaylibMaterial();

        RaylibMaterial(const RaylibMaterial &) = delete;
        RaylibMaterial(RaylibMaterial &&) = delete;

        RaylibMaterial &operator=(const RaylibMaterial &) = delete;
        RaylibMaterial &operator=(RaylibMaterial &&) = delete;

        ~RaylibMaterial();

        void setTint(Color tint) noexcept;

        [[nodiscard]] const ::Material &raw() const noexcept;

    private:
        ::Material material;
    };

}
