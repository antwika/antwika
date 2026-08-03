#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace antwika::music_editor
{

    /**
     * @brief The most bytes one paste may type.
     *
     * A score is a page of text, and a clipboard can hold a file.
     * An uncapped paste is applied inside one tick, and the pane
     * lays out every line of what it produced on every frame after,
     * so one accidental paste of megabytes wedges the editor for
     * good -- and rides into the recording, where a replay repeats
     * it.
     * The cap refuses nothing a person would type or copy from a
     * score, for the same reason the notation bounds refuse nothing
     * musical.
     */
    inline constexpr std::size_t kMaxPasteBytes = 65536;

    /**
     * @brief What a clipboard's bytes are allowed to paste.
     *
     * The grammar, the pane and the layout tables all speak ASCII,
     * and the replay format demands valid UTF-8 of every payload:
     * a Latin-1 clipboard fed to the recorder unfiltered used to
     * end a `--record` run mid-dispatch.
     * So a paste keeps printable ASCII, tabs and newlines, reads
     * CRLF and a lone CR as one newline each, drops every other
     * byte, and stops at kMaxPasteBytes.
     * Filtering sits here, upstream of the recorder, so a live run
     * and its replay type the same answer.
     *
     * @param raw The clipboard's bytes, exactly as handed over.
     * @return What may be typed into the document.
     */
    [[nodiscard]] std::string pasteableTextOf(std::string_view raw);

} // namespace antwika::music_editor
