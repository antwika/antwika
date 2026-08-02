#pragma once

namespace antwika::music_editor::events
{

    /**
     * @brief A paste: the clipboard's characters, as an event.
     *
     * The one event this application defines, and it exists because a
     * paste is the one thing here that cannot be worked out again.
     * Every other edit is derived from key edges the recording already
     * carries, but what the window system's clipboard held came from
     * outside the run -- so PasteSource reads it upstream of the
     * recorder and says what it found, and a replay takes the answer
     * from the file instead of from the replaying machine.
     *
     * The payload is the pasted characters, verbatim.
     */
    inline constexpr const char *kPaste = "music.paste";

} // namespace antwika::music_editor::events
