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

    /**
     * @brief Reads a word as a pitch that remembers where it was.
     *
     * notation::NumberWords under kNote, plus the word's own span
     * under kSpanBegin and kSpanLength -- which is what lets a note
     * that begins light the very characters it came from, however the
     * pattern algebra reordered, repeated or thinned it on the way.
     */
    class NoteWords final : public IWordReader
    {
    public:
        /**
         * @brief Build the reader.
         */
        NoteWords();

        /**
         * @brief Read one word as a pitch carrying its span.
         * @param word The word.
         * @param at Where it starts in the n("...") string.
         * @return The pitch under kNote and the span beside it.
         * @throws notation::NotationError If the word is not a number.
         */
        [[nodiscard]] Controls read(
            std::string_view word, std::size_t at) const override;

    private:
        notation::NumberWords numbers;
    };

} // namespace antwika::music_editor
