#pragma once

#include <cstdint>

#include <antwika/animation/Clip.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/companion/Messages.hpp"
#include "antwika/companion/PetLayout.hpp"
#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    class PetScene final
    {
    public:
        explicit PetScene(const Translator &translator);

        void draw(
            IRenderer &renderer,
            Size canvas,
            const PetSnapshot &snapshot) const;

    private:
        const Translator &translator;
        antwika::animation::Clip breathe;
        antwika::animation::Clip blink;
        antwika::animation::Clip drowse;
    };

}
