#pragma once

#include <cstddef>
#include <string_view>

#include <antwika/notation/IWordReader.hpp>
#include <antwika/notation/NumberWords.hpp>
#include <antwika/pattern/Controls.hpp>

namespace antwika::music_editor
{

    using antwika::notation::IWordReader;
    using antwika::pattern::Controls;

    class NoteWords final : public IWordReader
    {
    public:
        NoteWords();

        [[nodiscard]] Controls read(
            std::string_view word, std::size_t at) const override;

    private:
        notation::NumberWords numbers;
    };

}
