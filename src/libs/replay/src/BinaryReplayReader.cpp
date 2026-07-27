#include "antwika/replay/BinaryReplayReader.hpp"

#include <array>
#include <format>

#include <antwika/replay/ReplayFormatError.hpp>

#include "BinaryPrimitives.hpp"
#include "ReplayFormat.hpp"

namespace antwika::replay
{

    BinaryReplayReader::BinaryReplayReader(const IEventCodec &codec) : codec(codec)
    {
    }

    std::vector<TimedEvent> BinaryReplayReader::read(std::istream &in) const
    {
        std::array<char, detail::kReplayMagic.size()> magic{};
        in.read(magic.data(), static_cast<std::streamsize>(magic.size()));
        if (!in || magic != detail::kReplayMagic)
        {
            throw ReplayFormatError("antwika::replay: not a valid replay stream (bad magic bytes)");
        }

        const auto version = detail::readU32(in);
        if (version != detail::kReplayFormatVersion)
        {
            throw ReplayFormatError(std::format("antwika::replay: unsupported replay format version {}", version)); // GCOVR_EXCL_LINE
        }

        const auto count = detail::readU32(in);
        std::vector<TimedEvent> events;
        events.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            events.push_back(codec.decode(in));
        }
        return events;
    }

} // namespace antwika::replay
