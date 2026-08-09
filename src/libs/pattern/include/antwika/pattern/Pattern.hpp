#pragma once

#include <memory>
#include <vector>

#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    class IPattern
    {
    public:
        virtual ~IPattern() = default;

        virtual void query(const Span &window, IHapSink &out) const = 0;
    };

    class Pattern final
    {
    public:
        explicit Pattern(std::shared_ptr<const IPattern> impl);

        void query(const Span &window, IHapSink &out) const;

        [[nodiscard]] std::vector<Hap> queryAll(const Span &window) const;

    private:
        std::shared_ptr<const IPattern> pattern;
    };

}
