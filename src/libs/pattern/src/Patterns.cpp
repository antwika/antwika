#include "antwika/pattern/Patterns.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "antwika/pattern/Combinators.hpp"
#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    namespace
    {
        [[nodiscard]] std::int64_t floorDivide(
            std::int64_t top, std::int64_t bottom) noexcept
        {
            auto whole = top / bottom;

            if (top % bottom != 0 && (top < 0) != (bottom < 0))
            {
                --whole;
            }

            return whole;
        }

        class SilencePattern final : public IPattern
        {
        public:
            void query(const Span &window, IHapSink &out) const override
            {
                (void)window;
                (void)out;
            }
        };

        class PurePattern final : public IPattern
        {
        public:
            explicit PurePattern(Controls value)
                : carried(std::move(value))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                for (const auto &piece : window.spanCycles())
                {
                    // The whole is the cycle the piece falls in.
                    // A cut piece still reports the event it belongs to.
                    const Span whole(
                        piece.begin().sam(), piece.begin().nextSam());

                    out.accept(
                        Hap{
                            .whole = whole,
                            .part = piece,
                            .value = carried});
                }
            }

        private:
            Controls carried;
        };

        class SteadyPattern final : public IPattern
        {
        public:
            explicit SteadyPattern(Controls value)
                : carried(std::move(value))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                // No whole, so this never begins.
                // A sequencer reads it and never triggers it.
                out.accept(
                    Hap{
                        .whole = std::nullopt,
                        .part = window,
                        .value = carried});
            }

        private:
            Controls carried;
        };

        class StackPattern final : public IPattern
        {
        public:
            explicit StackPattern(std::vector<Pattern> layers)
                : stacked(std::move(layers))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                for (const auto &layer : stacked)
                {
                    layer.query(window, out);
                }
            }

        private:
            std::vector<Pattern> stacked;
        };

        class SlowcatPattern final : public IPattern
        {
        public:
            explicit SlowcatPattern(std::vector<Pattern> parts)
                : sequence(std::move(parts))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                const auto count =
                    static_cast<std::int64_t>(sequence.size());

                for (const auto &piece : window.spanCycles())
                {
                    const auto cycle = piece.begin().floorCycle();
                    const auto index = cycle - floorDivide(cycle, count)
                        * count;

                    // The chosen pattern is asked about its own cycle.
                    // Its own first cycle plays in outer cycle one.
                    const auto offset =
                        Cycle(cycle - floorDivide(cycle, count));

                    ShiftedSink shifted(out, offset);

                    const Span asked(
                        piece.begin() - offset, piece.end() - offset);

                    sequence[static_cast<std::size_t>(index)].query(
                        asked, shifted);
                }
            }

        private:
            class ShiftedSink final : public IHapSink
            {
            public:
                ShiftedSink(IHapSink &wrapped, Cycle by)
                    : out(wrapped), amount(by)
                {
                }

                void accept(const Hap &hap) override
                {
                    std::optional<Span> whole;

                    if (hap.whole.has_value())
                    {
                        whole = Span(
                            hap.whole->begin() + amount,
                            hap.whole->end() + amount);
                    }

                    const Span part(
                        hap.part.begin() + amount,
                        hap.part.end() + amount);

                    out.accept(
                        Hap{
                            .whole = whole,
                            .part = part,
                            .value = hap.value});
                }

            private:
                IHapSink &out;
                Cycle amount;
            };

            std::vector<Pattern> sequence;
        };

        /**
         * @brief One cycle shared by weight, fragments included.
         *
         * Each slice holds a run of the cycle as wide as its weight
         * says, and one cycle of its pattern is squeezed into it.
         * A window that sees only part of a slice gets the fragment
         * a query is owed -- the part it saw, inside the whole the
         * event covers -- which is what a stack of shifted silences
         * could not have said.
         */
        class TimecatPattern final : public IPattern
        {
        public:
            /**
             * @brief One slice, placed: its run of the cycle as
             * fractions, and the two scales between the cycle's time
             * and its own.
             */
            struct Placed
            {
                Cycle from;
                Cycle to;
                Cycle stretch;
                Cycle squeeze;
                Pattern part;
            };

            explicit TimecatPattern(std::vector<Placed> placed)
                : sequence(std::move(placed))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                for (const auto &piece : window.spanCycles())
                {
                    const Cycle base(piece.begin().floorCycle());

                    for (const auto &slice : sequence)
                    {
                        const Span held(
                            base + slice.from, base + slice.to);

                        const auto overlap = piece.intersect(held);

                        if (!overlap.has_value())
                        {
                            continue;
                        }

                        // Outer time into the slice's own cycle.
                        // The cycle number rides along unchanged.
                        // An alternation inside still turns per cycle.
                        const Span asked(
                            base
                                + (overlap->begin() - held.begin())
                                    * slice.stretch,
                            base
                                + (overlap->end() - held.begin())
                                    * slice.stretch);

                        MappedSink mapped(
                            out, base, held.begin(), slice.squeeze);

                        slice.part.query(asked, mapped);
                    }
                }
            }

        private:
            // The inverse of the query's mapping, on every hap.
            class MappedSink final : public IHapSink
            {
            public:
                MappedSink(
                    IHapSink &wrapped,
                    Cycle base,
                    Cycle from,
                    Cycle squeeze)
                    : out(wrapped),
                      base(base),
                      from(from),
                      squeeze(squeeze)
                {
                }

                void accept(const Hap &hap) override
                {
                    std::optional<Span> whole;

                    if (hap.whole.has_value())
                    {
                        whole = Span(
                            placed(hap.whole->begin()),
                            placed(hap.whole->end()));
                    }

                    out.accept(
                        Hap{
                            .whole = whole,
                            .part = Span(
                                placed(hap.part.begin()),
                                placed(hap.part.end())),
                            .value = hap.value});
                }

            private:
                [[nodiscard]] Cycle placed(const Cycle &inner) const
                {
                    return from + (inner - base) * squeeze;
                }

                IHapSink &out;
                Cycle base;
                Cycle from;
                Cycle squeeze;
            };

            std::vector<Placed> sequence;
        };
    } // namespace

    Pattern silence()
    {
        return Pattern(std::make_shared<const SilencePattern>());
    }

    Pattern pure(Controls value)
    {
        return Pattern(
            std::make_shared<const PurePattern>(std::move(value)));
    }

    Pattern steady(Controls value)
    {
        return Pattern(
            std::make_shared<const SteadyPattern>(std::move(value)));
    }

    Pattern stack(std::vector<Pattern> layers)
    {
        return Pattern(
            std::make_shared<const StackPattern>(std::move(layers)));
    }

    Pattern slowcat(std::vector<Pattern> parts)
    {
        if (parts.empty())
        {
            throw PatternError(
                "antwika::pattern: a sequence of no patterns has no "
                "cycle to give any of them");
        }

        return Pattern(
            std::make_shared<const SlowcatPattern>(std::move(parts)));
    }

    Pattern fastcat(std::vector<Pattern> parts)
    {
        const auto count = static_cast<std::int64_t>(parts.size());

        // Empty is refused by slowcat.
        // So the factor below is never zero by the time it is used.
        return fast(Cycle(count), slowcat(std::move(parts)));
    }

    Pattern timecat(std::vector<Slice> parts)
    {
        if (parts.empty())
        {
            throw PatternError(
                "antwika::pattern: a sequence of no slices has no "
                "cycle to share between them");
        }

        Cycle total(0);

        for (const auto &slice : parts)
        {
            if (!(slice.weight > Cycle(0)))
            {
                throw PatternError(
                    "antwika::pattern: a slice of no width at all "
                    "cannot hold a pattern");
            }

            total = total + slice.weight;
        }

        std::vector<TimecatPattern::Placed> placed;
        placed.reserve(parts.size());

        Cycle start(0);

        for (auto &slice : parts)
        {
            const auto from = start / total;

            start = start + slice.weight;

            placed.push_back(
                TimecatPattern::Placed{
                    .from = from,
                    .to = start / total,
                    .stretch = total / slice.weight,
                    .squeeze = slice.weight / total,
                    .part = std::move(slice.part)});
        }

        return Pattern(
            std::make_shared<const TimecatPattern>(std::move(placed)));
    }

} // namespace antwika::pattern
