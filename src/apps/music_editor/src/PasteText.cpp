#include "antwika/music_editor/PasteText.hpp"

#include <algorithm>

namespace antwika::music_editor
{

    std::string pasteableTextOf(const std::string_view raw)
    {
        std::string kept;
        kept.reserve(std::min(raw.size(), kMaxPasteBytes));

        for (std::size_t at = 0;
             at < raw.size() && kept.size() < kMaxPasteBytes;
             ++at)
        {
            const auto byte = static_cast<unsigned char>(raw[at]);

            if (byte == '\r')
            {
                if (at + 1 < raw.size() && raw[at + 1] == '\n')
                {
                    ++at;
                }

                kept.push_back('\n');

                continue;
            }

            const auto printable = byte >= 0x20 && byte <= 0x7E;

            if (printable || byte == '\n' || byte == '\t')
            {
                kept.push_back(static_cast<char>(byte));
            }
        }

        return kept;
    } // GCOVR_EXCL_LINE

}
