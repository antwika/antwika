#include "ScenePainter.hpp"

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>

#include "antwika/game/AtlasSheets.hpp"
#include "antwika/game/AtlasTextures.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/Messages.hpp"
#include "Translators.hpp"

namespace antwika::game::preview
{

    antwika::gfx::Bitmap paintedScene(
        const SceneSnapshot &snapshot, const antwika::gfx::Size canvas)
    {
        const auto sheets = loadAtlasSheets(defaultAtlases());

        return antwika::app::preview::drawnPage(
            "Antwika City",
            canvas,
            [&](antwika::gfx::IRenderer &renderer)
            {
                const auto one =
                    renderer.createTexture(sheets.of(AtlasKind::OneByOne));
                const auto two =
                    renderer.createTexture(sheets.of(AtlasKind::TwoByTwo));
                const auto three = renderer.createTexture(
                    sheets.of(AtlasKind::ThreeByThree));
                const auto walker =
                    renderer.createTexture(sheets.walker);

                const AtlasTextures atlases{
                    *one, *two, *three, *walker, sheets.specs};

                const GridScene scene(tests::kTranslator);

                scene.draw(renderer, canvas, snapshot, atlases);
            });
    } // GCOVR_EXCL_LINE

}
