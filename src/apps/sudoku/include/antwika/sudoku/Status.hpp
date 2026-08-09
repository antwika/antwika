#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/sudoku/MessageId.hpp"

namespace antwika::sudoku
{

    enum class Status : std::uint8_t
    {
        Playing = 0,

        Solved,

        Complete,

        Unsolvable,

        LimitExceeded,

        GivenLocked,
    };

    [[nodiscard]] constexpr Status enumBound(Status) noexcept
    {
        return Status::GivenLocked;
    }

    inline constexpr std::size_t kStatusCount =
        antwika::enums::kCount<Status>;

    [[nodiscard]] constexpr MessageId statusNameId(
        const Status status) noexcept
    {
        constexpr std::array<MessageId, kStatusCount>
            ids{
                MessageId::Hint,
                MessageId::Solved,
                MessageId::Complete,
                MessageId::NoSolution,
                MessageId::LimitExceeded,
                MessageId::GivenLocked};

        return ids[static_cast<std::size_t>(status) % kStatusCount];
    }

}
