#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "antwika/editor/view/INotices.hpp"

namespace antwika::editor::fakes
{

    class FakeNotices final : public INotices
    {
    public:
        void showStatus(
            const std::string &text,
            const bool warns,
            const std::uint32_t durationTicks) override
        {
            static_cast<void>(warns);
            static_cast<void>(durationTicks);
            statusTexts.push_back(text);
        }

        std::vector<std::string> statusTexts;
    };

}
