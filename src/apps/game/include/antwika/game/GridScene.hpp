#pragma once

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/AtlasTextures.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    class GridScene final
    {
    public:
        explicit GridScene(const Translator &translator);

        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress subTick = Progress()) const;

    private:
        void drawTerrain(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress phase,
            bool withWalkers) const;

        void drawWalker(
            IRenderer &renderer,
            Size canvas,
            const WalkerSprite &walker,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases,
            Progress phase) const;

        void drawPlan(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawGhost(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawOverlay(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawOverlayValues(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const AtlasTextures &atlases) const;

        void drawReadout(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot) const;

        [[nodiscard]] static bool onCanvas(
            const AtlasSpecs &specs,
            Cell cell,
            Size canvas,
            const SceneSnapshot &snapshot);

        const Translator &translator;
    };

}
