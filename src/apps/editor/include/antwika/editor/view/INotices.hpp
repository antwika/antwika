#pragma once

#include <cstdint>
#include <string>

namespace antwika::editor
{

    inline constexpr std::uint32_t kNoticeTicks = 240;

    class INotices
    {
    public:
        INotices() = default;

        INotices(const INotices &) = delete;
        INotices(INotices &&) = delete;

        INotices &operator=(const INotices &) = delete;
        INotices &operator=(INotices &&) = delete;

        virtual ~INotices() = default;

        virtual void showStatus(
            const std::string &text,
            bool warns = false,
            std::uint32_t durationTicks = kNoticeTicks)
            = 0;
    };

}
