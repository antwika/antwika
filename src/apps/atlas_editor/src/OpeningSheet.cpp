#include "antwika/atlas_editor/OpeningSheet.hpp"

#include <string>
#include <utility>

#include <antwika/event/ITickEventSource.hpp>

namespace antwika::atlas_editor
{

    using antwika::event::ITickEventSource;

    namespace
    {
        constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
        constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

        void fold(std::uint64_t &hash, const std::uint64_t value) noexcept
        {
            for (std::size_t byte = 0; byte < sizeof(value); ++byte)
            {
                hash ^= (value >> (byte * 8)) & 0xFFU;
                hash *= kFnvPrime;
            }
        }
    }

    std::uint64_t fingerprintOf(
        const antwika::gfx::Bitmap &image) noexcept
    {
        auto hash = kFnvOffset;

        fold(hash, image.size.width);
        fold(hash, image.size.height);

        for (const auto byte : image.pixels)
        {
            hash ^= byte;
            hash *= kFnvPrime;
        }

        return hash;
    }

    Event openingSheetEvent(const Canvas &canvas)
    {
        const auto size = canvas.size();

        // GCOVR_EXCL_START
        return Event{
            .name = std::string(events::kOpeningSheet),
            .payload = std::to_string(fingerprintOf(canvas.bitmap()))
                       + " " + std::to_string(size.width) + "x"
                       + std::to_string(size.height)};
    } // GCOVR_EXCL_STOP

    OpeningSheetSource::OpeningSheetSource(
        ITickEventSource &inner, std::optional<Event> announcement)
        : inner(inner), announcement(std::move(announcement))
    {
    }

    std::vector<Event> OpeningSheetSource::eventsFor(
        const antwika::time::Tick tick)
    {
        auto events = inner.eventsFor(tick);

        if (announcement.has_value())
        {
            // GCOVR_EXCL_START
            events.insert(
                events.begin(), std::move(*announcement));
            // GCOVR_EXCL_STOP
            announcement.reset();
        }

        return events;
    } // GCOVR_EXCL_LINE

}
