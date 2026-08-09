#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace antwika::music_editor
{

    inline constexpr std::size_t kMaxPasteBytes = 65536;

    [[nodiscard]] std::string pasteableTextOf(std::string_view raw);

}
