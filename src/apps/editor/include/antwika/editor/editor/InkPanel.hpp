#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/editor/editor/EditorDocument.hpp"
#include "antwika/editor/editor/state/InkPicker.hpp"
#include "antwika/editor/editor/state/PointerTrack.hpp"
#include "antwika/editor/ui/CharacterSheetView.hpp"
#include "antwika/editor/view/IEditSteps.hpp"

namespace antwika::editor
{

    class InkPanel final
    {
    public:
        InkPanel(
            EditorDocument &document,
            render::AtlasSheets &atlasSheets,
            CharacterSheetView &characterView,
            render::CharacterSkins &characterSkins,
            gfx::ViewportRenderer &viewportRenderer,
            IEditSteps &editSteps) noexcept;

        InkPicker inkPicker;

        [[nodiscard]] std::uint8_t glowOf(std::size_t ink) const;

        void carryInk();

        void recolorInk(gfx::Color wantedColor);

        [[nodiscard]] bool consumePaletteWidgets(
            const ui::Interactions &interactions,
            PointerTrack &pointer,
            std::uint32_t tick);

        [[nodiscard]] bool consumePickerPress(
            const input::PointerButtonPressed &downPressed,
            PointerTrack &pointer,
            float railWidth);

    private:
        EditorDocument &document;

        render::AtlasSheets &atlasSheets;

        CharacterSheetView &characterView;

        render::CharacterSkins &characterSkins;

        gfx::ViewportRenderer &viewportRenderer;

        IEditSteps &editSteps;
    };

}
