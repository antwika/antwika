#pragma once

#include <memory>
#include <vector>

#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    /**
     * @brief A function from a stretch of time to the events in it.
     *
     * **A window rather than a cursor**, which is the decision every
     * combinator depends on.
     * A cursor is stateful, so it cannot be shared between a sequencer
     * and a test, cannot be asked out of order, and cannot be
     * transformed without the transformation becoming stateful too.
     * A window composes: a transposition forwards it unchanged and
     * rewrites values, a stretch maps it backwards through the inverse
     * ratio before forwarding, and a reversal reflects it inside the
     * cycle it falls in.
     */
    class IPattern
    {
    public:
        virtual ~IPattern() = default;

        /**
         * @brief Find the events inside a stretch of time.
         * @param window What to look inside.
         * @param out Where every event found is handed.
         * @throws PatternError If the arithmetic will not fit.
         */
        virtual void query(const Span &window, IHapSink &out) const = 0;
    };

    /**
     * @brief A pattern, as a value.
     *
     * **Value semantics rather than a reference to an operand the caller
     * keeps alive**, and that is what makes the algebra usable:
     *
     * @code
     * const auto riff = fast(Cycle(2), euclid(3, 8, pure(kKick)));
     * @endcode
     *
     * Every combinator takes patterns by value and returns one, so an
     * expression like that owns everything inside it and nothing has a
     * lifetime rule written in a comment.
     * The shared pointer is what makes copying one cheap, and a pattern
     * is immutable so sharing is safe.
     *
     * The allocation happens where a pattern is *built*, which is set-up
     * or the tick path, and never where samples are written.
     * **query() itself allocates nothing** -- it walks the graph and
     * hands each event to the caller's sink.
     */
    class Pattern final
    {
    public:
        /**
         * @brief Wrap an implementation as a value.
         * @param impl What answers a query; must not be null.
         * @throws PatternError If it is null.
         */
        explicit Pattern(std::shared_ptr<const IPattern> impl);

        /**
         * @brief Find the events inside a stretch of time.
         * @param window What to look inside.
         * @param out Where every event found is handed.
         * @throws PatternError If the arithmetic will not fit.
         */
        void query(const Span &window, IHapSink &out) const;

        /**
         * @brief Find the events inside a stretch, as a vector.
         *
         * The convenience an assertion wants.
         * A caller on a path that runs forever uses a HapBuffer it
         * reserved once instead.
         *
         * @param window What to look inside.
         * @return The events, in the order they were found.
         * @throws PatternError If the arithmetic will not fit.
         */
        [[nodiscard]] std::vector<Hap> queryAll(const Span &window) const;

    private:
        std::shared_ptr<const IPattern> pattern;
    };

} // namespace antwika::pattern
