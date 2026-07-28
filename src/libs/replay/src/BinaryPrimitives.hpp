#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

/**
 * @file
 * @brief Explicit big-endian encode/decode helpers, shared by
 * BinaryEventCodec and the binary replay writer/reader.
 *
 * Never native struct layout, which keeps the format identical regardless
 * of host byte order.
 */
namespace antwika::replay::detail
{

    /**
     * @brief Write a 32-bit unsigned integer in big-endian byte order.
     * @param value The value to write.
     * @param out The stream to write to.
     */
    void writeU32(std::uint32_t value, std::ostream &out);

    /**
     * @brief Read a 32-bit unsigned integer in big-endian byte order.
     * @param in The stream to read from.
     * @return The decoded value.
     */
    [[nodiscard]] std::uint32_t readU32(std::istream &in);

    /**
     * @brief Write a 64-bit unsigned integer in big-endian byte order.
     * @param value The value to write.
     * @param out The stream to write to.
     */
    void writeU64(std::uint64_t value, std::ostream &out);

    /**
     * @brief Read a 64-bit unsigned integer in big-endian byte order.
     * @param in The stream to read from.
     * @return The decoded value.
     */
    [[nodiscard]] std::uint64_t readU64(std::istream &in);

    /**
     * @brief Write a length-prefixed string.
     * @param value The string to write.
     * @param out The stream to write to.
     */
    void writeString(const std::string &value, std::ostream &out);

    /**
     * @brief Read a length-prefixed string.
     * @param in The stream to read from.
     * @return The decoded string.
     */
    [[nodiscard]] std::string readString(std::istream &in);

} // namespace antwika::replay::detail
