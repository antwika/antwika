#pragma once

#include <cstdint>
#include <span>

namespace antwika::font::detail
{

    void requireReadableDirectory(std::span<const std::uint8_t> bytes);

}
