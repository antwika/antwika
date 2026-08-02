#include "antwika/replay/DocumentDepth.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace antwika::replay
{

    bool nestsTooDeep(const nlohmann::json &document)
    {
        // One level at a time, breadth first.
        // The stack this spends is a vector's, never the call stack's.
        std::vector<const nlohmann::json *> level{&document};

        for (std::size_t depth = 0;
             depth <= kMaxDocumentDepth && !level.empty();
             ++depth)
        {
            std::vector<const nlohmann::json *> deeper;

            for (const auto *value : level)
            {
                // Iterating a primitive would yield the value itself.
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
        // Only an unwind destroys level at this brace.
    } // GCOVR_EXCL_LINE

} // namespace antwika::replay
