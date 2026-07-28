#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

namespace antwika::replay::detail
{

    // Explicit big-endian encode/decode helpers.
    // Shared by BinaryEventCodec and the binary replay writer/reader.
    // Never native struct layout.
    // That keeps the format identical regardless of host byte order.

    void writeU32(std::uint32_t value, std::ostream &out);
    [[nodiscard]] std::uint32_t readU32(std::istream &in);

    void writeU64(std::uint64_t value, std::ostream &out);
    [[nodiscard]] std::uint64_t readU64(std::istream &in);

    void writeString(const std::string &value, std::ostream &out);
    [[nodiscard]] std::string readString(std::istream &in);

} // namespace antwika::replay::detail
