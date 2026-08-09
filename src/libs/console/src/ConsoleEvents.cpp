#include "antwika/console/ConsoleEvents.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace antwika::console
{

    std::size_t ConsoleEvents::open()
    {
        taken.push_back(queued.size());

        return taken.size() - 1;
    }

    void ConsoleEvents::send(Event event)
    {
        queued.push_back(std::move(event));
    }

    std::vector<Event> ConsoleEvents::take(const std::size_t reader)
    {
        const auto from = static_cast<std::ptrdiff_t>(taken[reader]);

        std::vector<Event> due(
            std::next(queued.begin(), from), queued.end());

        taken[reader] = queued.size();

        forget();

        return due;
    } // GCOVR_EXCL_LINE

    void ConsoleEvents::forget()
    {
        const auto seen = *std::ranges::min_element(taken);

        if (seen == 0)
        {
            return;
        }

        queued.erase(
            queued.begin(),
            std::next(queued.begin(), static_cast<std::ptrdiff_t>(seen)));

        for (auto &cursor : taken)
        {
            cursor -= seen;
        }
    }

    std::size_t ConsoleEvents::pending() const noexcept
    {
        return queued.size();
    }

    void ConsoleEvents::refuse(
        const std::string &name, const std::string &reason)
    {
        refused.push_back(name + " refused: " + reason);
    }

    std::vector<std::string> ConsoleEvents::takeRefusals()
    {
        auto gathered = std::move(refused);
        refused.clear();

        return gathered;
    } // GCOVR_EXCL_LINE

}
