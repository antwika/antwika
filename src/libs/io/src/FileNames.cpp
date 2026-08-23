#include "antwika/io/FileNames.hpp"

#include <algorithm>

namespace antwika::io
{

    namespace
    {
        [[nodiscard]] bool endsInSuffix(
            const std::string_view name,
            const std::string_view suffix)
        {
            return name.size() > suffix.size()
                   && name.substr(name.size() - suffix.size())
                          == suffix;
        }
    }

    std::vector<std::string> getFilteredBySuffix(
        const std::span<const std::string> names,
        const std::string_view suffix,
        const std::size_t most)
    {
        std::vector<std::string> matchedNames;

        for (const auto &name : names)
        {
            if (endsInSuffix(name, suffix))
            {
                matchedNames.push_back(name);
            }
        }

        std::sort(matchedNames.begin(), matchedNames.end());

        if (matchedNames.size() > most)
        {
            matchedNames.resize(most);
        }

        return matchedNames;
    } // GCOVR_EXCL_LINE

    std::string getWithSuffix(
        const std::string_view name, const std::string_view suffix)
    {
        std::string safeName(name);

        if (!endsInSuffix(safeName, suffix))
        {
            safeName += suffix;
        }

        return safeName;
    } // GCOVR_EXCL_LINE

}
