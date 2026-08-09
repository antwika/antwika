#include "antwika/replay/DocumentDepth.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace antwika::replay
{

    bool nestsTooDeep(const nlohmann::json &document)
    {
        std::vector<const nlohmann::json *> level{&document};

        for (std::size_t depth = 0;
             depth <= kMaxDocumentDepth && !level.empty();
             ++depth)
        {
            std::vector<const nlohmann::json *> deeper;

            for (const auto *value : level)
            {
                if (!value->is_structured())
                {
                    continue;
                }

                for (const auto &child : *value)
                {
                    deeper.push_back(&child);
                }
            }

            level = std::move(deeper);
        }

        return !level.empty();
    } // GCOVR_EXCL_LINE

}
