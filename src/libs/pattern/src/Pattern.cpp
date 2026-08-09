#include "antwika/pattern/Pattern.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/HapBuffer.hpp"
#include "antwika/pattern/IHapSink.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    Pattern::Pattern(std::shared_ptr<const IPattern> impl)
        : pattern(std::move(impl))
    {
        if (pattern == nullptr)
        {
            throw PatternError(
                "antwika::pattern: a pattern with nothing behind it "
                "could never be queried");
        }
    }

    void Pattern::query(const Span &window, IHapSink &out) const
    {
        pattern->query(window, out);
    }

    std::vector<Hap> Pattern::queryAll(const Span &window) const
    {
        HapBuffer buffer;

        pattern->query(window, buffer);

        return buffer.haps();

    } // GCOVR_EXCL_LINE

}
