#include "BinaryPrimitives.hpp"

#include <array>

#include <antwika/replay/ReplayFormatError.hpp>

namespace antwika::replay::detail
{

    namespace
    {
        // Big-endian read/write, generic over the unsigned integer width.
        // writeU32/readU32/writeU64/readU64 below are thin, named wrappers.
        // They exist so call sites don't need to spell out a template argument.
        template <typename T>
        void writeBigEndian(T value, std::ostream &out)
        {
            std::array<char, sizeof(T)> bytes{};
            for (std::size_t i = 0; i < bytes.size(); ++i)
            {
                const auto shift =
                    static_cast<unsigned>((bytes.size() - 1 - i) * 8);
                bytes[i] = static_cast<char>((value >> shift) & 0xFF);
            }
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }

        template <typename T>
        T readBigEndian(std::istream &in)
        {
            std::array<char, sizeof(T)> bytes{};
            in.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            if (!in)
            {
                throw ReplayFormatError(
                    "antwika::replay: unexpected end of stream while "
                    "reading a fixed-size value");
            }
            T value{};
            for (auto byte : bytes)
            {
                value = static_cast<T>(
                    (value << 8) | static_cast<unsigned char>(byte));
            }
            return value;
        }
    } // namespace

    void writeU32(std::uint32_t value, std::ostream &out)
    {
        writeBigEndian(value, out);
    }

    std::uint32_t readU32(std::istream &in)
    {
        return readBigEndian<std::uint32_t>(in);
    }

    void writeU64(std::uint64_t value, std::ostream &out)
    {
        writeBigEndian(value, out);
    }

    std::uint64_t readU64(std::istream &in)
    {
        return readBigEndian<std::uint64_t>(in);
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
                throw ReplayFormatError(
                    "antwika::replay: unexpected end of stream while "
                    "reading a string");
            }
        }
        return value;
    }

} // namespace antwika::replay::detail
