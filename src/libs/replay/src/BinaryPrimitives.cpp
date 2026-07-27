#include "BinaryPrimitives.hpp"

#include <array>

#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay::detail
{

    void writeU32(std::uint32_t value, std::ostream &out)
    {
        std::array<char, 4> bytes{};
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            const auto shift = static_cast<unsigned>((bytes.size() - 1 - i) * 8);
            bytes[i] = static_cast<char>((value >> shift) & 0xFF);
        }
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    std::uint32_t readU32(std::istream &in)
    {
        std::array<char, 4> bytes{};
        in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!in)
        {
            throw ReplayFormatError("antwika::replay: unexpected end of stream while reading a 32-bit value");
        }
        std::uint32_t value{};
        for (auto byte : bytes)
        {
            value = static_cast<std::uint32_t>((value << 8) | static_cast<unsigned char>(byte));
        }
        return value;
    }

    void writeU64(std::uint64_t value, std::ostream &out)
    {
        std::array<char, 8> bytes{};
        for (std::size_t i = 0; i < bytes.size(); ++i)
        {
            const auto shift = static_cast<unsigned>((bytes.size() - 1 - i) * 8);
            bytes[i] = static_cast<char>((value >> shift) & 0xFF);
        }
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

    std::uint64_t readU64(std::istream &in)
    {
        std::array<char, 8> bytes{};
        in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!in)
        {
            throw ReplayFormatError("antwika::replay: unexpected end of stream while reading a 64-bit value");
        }
        std::uint64_t value{};
        for (auto byte : bytes)
        {
            value = (value << 8) | static_cast<unsigned char>(byte);
        }
        return value;
    }

    void writeString(const std::string &value, std::ostream &out)
    {
        writeU32(static_cast<std::uint32_t>(value.size()), out);
        if (!value.empty())
        {
            out.write(value.data(), static_cast<std::streamsize>(value.size()));
        }
    }

    std::string readString(std::istream &in)
    {
        const auto length = readU32(in);
        std::string value(length, '\0');
        if (length > 0)
        {
            in.read(value.data(), static_cast<std::streamsize>(length));
            if (!in)
            {
                throw ReplayFormatError("antwika::replay: unexpected end of stream while reading a string");
            }
        }
        return value;
    }

} // namespace antwika::replay::detail
