#include "antwika/music_editor/NoteWords.hpp"

#include <antwika/pattern/ParamValue.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::pattern::ParamValue;

    NoteWords::NoteWords() : numbers(kNote)
    {
    }

    Controls NoteWords::read(
        const std::string_view word, const std::size_t at) const
    {
        auto controls = numbers.read(word, at);

        controls.set(
            kSpanBegin, ParamValue(static_cast<std::int64_t>(at)));
        controls.set(
            kSpanLength,
            ParamValue(static_cast<std::int64_t>(word.size())));

        return controls;
    } // GCOVR_EXCL_LINE

}
