#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/PuzzleState.hpp"

namespace antwika::sudoku
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;

    class SudokuScene final
    {
    public:
        explicit SudokuScene(const Translator &translator);

        [[nodiscard]] Frame describe(
            Size canvas, Pointer pointer, const PuzzleState &state)
            const;

        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

}
