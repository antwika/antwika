#include "antwika/pattern/Combinators.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/IHapSink.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Patterns.hpp"
#include "antwika/pattern/Span.hpp"

namespace antwika::pattern
{

    namespace
    {
        class TimeMappedPattern final : public IPattern
        {
        public:
            TimeMappedPattern(Cycle by, Cycle then, Pattern wrapped)
                : factor(by), offset(then), inner(std::move(wrapped))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                MappingSink mapped(out, factor, offset);

                const Span asked(
                    window.begin() * factor + offset,
                    window.end() * factor + offset);

                inner.query(asked, mapped);
            }

        private:
            class MappingSink final : public IHapSink
            {
            public:
                MappingSink(IHapSink &wrapped, Cycle by, Cycle then)
                    : out(wrapped), factor(by), offset(then)
                {
                }

                void accept(const Hap &hap) override
                {
                    out.accept(
                        Hap{
                            .whole = hap.whole.has_value()
                                ? std::optional<Span>(back(*hap.whole))
                                : std::nullopt,
                            .part = back(hap.part),
                            .value = hap.value});
                }

            private:
                [[nodiscard]] Span back(const Span &span) const
                {
                    return Span(
                        (span.begin() - offset) / factor,
                        (span.end() - offset) / factor);
                }

                IHapSink &out;
                Cycle factor;
                Cycle offset;
            };

            Cycle factor;
            Cycle offset;
            Pattern inner;
        };

        class RevPattern final : public IPattern
        {
        public:
            explicit RevPattern(Pattern wrapped)
                : inner(std::move(wrapped))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                for (const auto &piece : window.spanCycles())
                {
                    const auto pivot =
                        piece.begin().sam() + piece.begin().nextSam();

                    ReflectingSink reflected(out, pivot);

                    inner.query(
                        Span(pivot - piece.end(), pivot - piece.begin()),
                        reflected);
                }
            }

        private:
            class ReflectingSink final : public IHapSink
            {
            public:
                ReflectingSink(IHapSink &wrapped, Cycle around)
                    : out(wrapped), pivot(around)
                {
                }

                void accept(const Hap &hap) override
                {
                    out.accept(
                        Hap{
                            .whole = hap.whole.has_value()
                                ? std::optional<Span>(back(*hap.whole))
                                : std::nullopt,
                            .part = back(hap.part),
                            .value = hap.value});
                }

            private:
                [[nodiscard]] Span back(const Span &span) const
                {
                    return Span(
                        pivot - span.end(), pivot - span.begin());
                }

                IHapSink &out;
                Cycle pivot;
            };

            Pattern inner;
        };

        class DegradedPattern final : public IPattern
        {
        public:
            DegradedPattern(
                std::uint64_t threshold,
                std::uint64_t withSeed,
                Pattern wrapped)
                : cutoff(threshold),
                  seed(withSeed),
                  inner(std::move(wrapped))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                ThinningSink thinned(out, cutoff, seed);

                inner.query(window, thinned);
            }

        private:
            class ThinningSink final : public IHapSink
            {
            public:
                ThinningSink(
                    IHapSink &wrapped,
                    std::uint64_t threshold,
                    std::uint64_t withSeed)
                    : out(wrapped), cutoff(threshold), seed(withSeed)
                {
                }

                void accept(const Hap &hap) override
                {
                    if (!hap.whole.has_value())
                    {
                        out.accept(hap);

                        return;
                    }

                    const auto &at = hap.whole->begin();

                    constexpr std::uint64_t kSpreadTop
                        = 0x9E3779B97F4A7C15ULL;

                    constexpr std::uint64_t kSpreadBottom
                        = 0xC2B2AE3D27D4EB4FULL;

                    rng::SplitMix64Rng generator(
                        seed
                        ^ (static_cast<std::uint64_t>(at.numerator())
                           * kSpreadTop)
                        ^ (static_cast<std::uint64_t>(at.denominator())
                           * kSpreadBottom));

                    if ((generator.next() >> 32) < cutoff)
                    {
                        return;
                    }

                    out.accept(hap);
                }

            private:
                IHapSink &out;
                std::uint64_t cutoff;
                std::uint64_t seed;
            };

            std::uint64_t cutoff;
            std::uint64_t seed;
            Pattern inner;
        };

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

        class DuringPattern final : public IPattern
        {
        public:
            DuringPattern(
                std::int64_t every,
                std::vector<Span> when,
                Pattern wrapped)
                : period(every),
                  windows(std::move(when)),
                  inner(std::move(wrapped))
            {
            }

            void query(const Span &window, IHapSink &out) const override
            {
                const auto first =
                    floorDivide(window.begin().floorCycle(), period);

                for (auto turn = first;
                     Cycle(turn * period) < window.end();
                     ++turn)
                {
                    const Cycle base(turn * period);

                    for (const auto &slot : windows)
                    {
                        const auto open = base + slot.begin();
                        const auto close = base + slot.end();

                        const auto from =
                            std::max(open, window.begin());
                        const auto until =
                            std::min(close, window.end());

                        if (until <= from)
                        {
                            continue;
                        }

                        ShiftedSink shifted(out, open);

                        inner.query(
                            Span(from - open, until - open), shifted);
                    }
                }
            }

        private:
            class ShiftedSink final : public IHapSink
            {
            public:
                ShiftedSink(IHapSink &wrapped, Cycle by)
                    : out(wrapped), offset(by)
                {
                }

                void accept(const Hap &hap) override
                {
                    out.accept(
                        Hap{
                            .whole = hap.whole.has_value()
                                ? std::optional<Span>(
                                      forward(*hap.whole))
                                : std::nullopt,
                            .part = forward(hap.part),
                            .value = hap.value});
                }

            private:
                [[nodiscard]] Span forward(const Span &span) const
                {
                    return Span(
                        span.begin() + offset, span.end() + offset);
                }

                IHapSink &out;
                Cycle offset;
            };

            std::int64_t period;
            std::vector<Span> windows;
            Pattern inner;
        };

        [[nodiscard]] Pattern timeMapped(
            Cycle factor, Cycle offset, Pattern inner)
        {
            if (factor <= Cycle())
            {
                throw PatternError(
                    "antwika::pattern: a pattern cannot run at "
                    + std::to_string(factor.numerator()) + "/"
                    + std::to_string(factor.denominator())
                    + " of its speed");
            }

            return Pattern(
                std::make_shared<const TimeMappedPattern>(
                    factor, offset, std::move(inner)));
        }
    }

    Pattern fast(Cycle factor, Pattern inner)
    {
        return timeMapped(factor, Cycle(), std::move(inner));
    }

    Pattern slow(Cycle factor, Pattern inner)
    {
        if (factor <= Cycle())
        {
            throw PatternError(
                "antwika::pattern: a pattern cannot run at "
                + std::to_string(factor.numerator()) + "/"
                + std::to_string(factor.denominator())
                + " of its speed");
        }

        return timeMapped(Cycle(1) / factor, Cycle(), std::move(inner));
    }

    Pattern early(Cycle amount, Pattern inner)
    {
        return timeMapped(Cycle(1), amount, std::move(inner));
    }

    Pattern late(Cycle amount, Pattern inner)
    {
        return timeMapped(
            Cycle(1), Cycle() - amount, std::move(inner));
    }

    Pattern rev(Pattern inner)
    {
        return Pattern(
            std::make_shared<const RevPattern>(std::move(inner)));
    }

    Pattern euclid(std::int64_t pulses, std::int64_t steps, Pattern inner)
    {
        if (steps <= 0 || pulses < 0 || pulses > steps)
        {
            throw PatternError(
                "antwika::pattern: " + std::to_string(pulses)
                + " onsets do not spread across "
                + std::to_string(steps) + " steps");
        }

        std::vector<Pattern> slots;
        slots.reserve(static_cast<std::size_t>(steps));

        for (std::int64_t step = 0; step < steps; ++step)
        {
            const auto sounds = (step * pulses) % steps < pulses;

            slots.push_back(sounds ? inner : silence());
        }

        return fastcat(std::move(slots));
    }

    Pattern degradeBy(
        ParamValue chance, std::uint64_t seed, Pattern inner)
    {
        if (chance < ParamValue() || chance > ParamValue(1))
        {
            throw PatternError(
                "antwika::pattern: a chance of "
                + std::to_string(chance.approximate())
                + " is not between zero and one");
        }

        return Pattern(
            std::make_shared<const DegradedPattern>(
                static_cast<std::uint64_t>(chance.raw()),
                seed,
                std::move(inner)));
    }

    Pattern during(
        std::int64_t period, std::vector<Span> windows, Pattern inner)
    {
        if (period < 1)
        {
            throw PatternError(
                "antwika::pattern: a schedule cannot repeat every "
                + std::to_string(period) + " cycles");
        }

        if (windows.empty())
        {
            throw PatternError(
                "antwika::pattern: a schedule of no windows would "
                "never play");
        }

        auto reached = Cycle();

        for (const auto &slot : windows)
        {
            if (slot.begin() < reached || Cycle(period) < slot.end())
            {
                throw PatternError(
                    "antwika::pattern: a schedule's windows sit in "
                    "order, apart, and inside its period");
            }

            reached = slot.end();
        }

        return Pattern(
            std::make_shared<const DuringPattern>(
                period, std::move(windows), std::move(inner)));
    }

}
