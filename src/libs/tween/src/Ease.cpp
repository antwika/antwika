#include "antwika/tween/Ease.hpp"

#include <array>
#include <cstddef>
#include <limits>

#include <antwika/time/Tick.hpp>

#include "antwika/tween/Easing.hpp"
#include "antwika/tween/TweenError.hpp"

namespace antwika::tween
{

    namespace
    {
        using antwika::time::Tick;

        constexpr Tick kMaxTick = std::numeric_limits<Tick>::max();

        [[noreturn]] void refuse()
        {
            throw TweenError(
                "antwika::tween: an easing's arithmetic does not fit in "
                "a tick, so its denominator is too large for the curve "
                "it was asked for");
        }

        // The guard is a division rather than the product it checks.
        // So the check cannot itself overflow the thing it is checking.
        [[nodiscard]] Tick mul(Tick left, Tick right)
        {
            if (left != 0 && right > kMaxTick / left)
            {
                refuse();
            }

            return left * right;
        }

        // A subtraction for mul()'s reason, read the other way round.
        [[nodiscard]] Tick add(Tick left, Tick right)
        {
            if (left > kMaxTick - right)
            {
                refuse();
            }

            return left + right;
        }

        [[nodiscard]] Tick square(Tick value)
        {
            return mul(value, value);
        }

        [[nodiscard]] Tick power(Tick base, Tick exponent)
        {
            Tick result = 1;

            for (Tick step = 0; step < exponent; ++step)
            {
                result = mul(result, base);
            }

            return result;
        }

        // Both ends are unsigned and either may be the larger.
        // Which it is depends on where in the curve the fraction fell.
        // So the difference is taken by hand rather than by subtraction.
        // One the wrong way round would wrap rather than go negative.
        [[nodiscard]] Tick distance(Tick left, Tick right)
        {
            return left >= right ? left - right : right - left;
        }

        // f(t) = t^k
        [[nodiscard]] Progress powerIn(Progress progress, Tick exponent)
        {
            return Progress(
                power(progress.numerator(), exponent),
                power(progress.denominator(), exponent));
        }

        // f(t) = 1 - (1 - t)^k
        [[nodiscard]] Progress powerOut(Progress progress, Tick exponent)
        {
            const Tick whole = power(progress.denominator(), exponent);
            const Tick rest = power(
                progress.denominator() - progress.numerator(), exponent);

            return Progress(whole - rest, whole);
        }

        // Below the midpoint this is 2^(k-1) t^k.
        // At or above it, 1 - 2^(k-1) (1 - t)^k.
        // The two halves meet exactly at one half, whatever the power.
        [[nodiscard]] Progress powerInOut(Progress progress, Tick exponent)
        {
            const Tick num = progress.numerator();
            const Tick den = progress.denominator();
            const Tick whole = power(den, exponent);
            const Tick scale = power(2, exponent - 1);

            // Written as a product so the comparison stays exact.
            if (mul(num, 2) < den)
            {
                return Progress(
                    mul(scale, power(num, exponent)), whole);
            }

            return Progress(
                whole - mul(scale, power(den - num, exponent)), whole);
        }

        // f(t) = 121/16 (t - a)^2 + c, in four pieces.
        //
        // The 121 cancels in every piece.
        // Each piece's offset is over an eleventh or a twenty-second.
        // The square of that denominator carries the 121 with it.
        // That is the whole reason a bounce is expressible here at all.
        // It looks transcendental and is four parabolas.
        //
        // Every piece is put over 64 d^2 rather than each over its own.
        // So which piece answered is not readable off the denominator.
        [[nodiscard]] Progress bounceOut(Progress progress, Tick)
        {
            const Tick num = progress.numerator();
            const Tick den = progress.denominator();
            const Tick whole = mul(64, square(den));
            const Tick eleven = mul(11, num);
            const Tick squared = square(den);

            if (eleven < mul(4, den))
            {
                return Progress(mul(484, square(num)), whole);
            }

            if (eleven < mul(8, den))
            {
                return Progress(
                    add(mul(4, square(distance(eleven, mul(6, den)))),
                        mul(48, squared)),
                    whole);
            }

            if (eleven < mul(10, den))
            {
                return Progress(
                    add(mul(4, square(distance(eleven, mul(9, den)))),
                        mul(60, squared)),
                    whole);
            }

            return Progress(
                add(square(distance(mul(22, num), mul(21, den))),
                    mul(63, squared)),
                whole);
        }

        // f(t) = 1 - bounceOut(1 - t)
        [[nodiscard]] Progress bounceIn(Progress progress, Tick)
        {
            const Tick den = progress.denominator();
            const Progress flipped = bounceOut(
                Progress(den - progress.numerator(), den), 0);

            return Progress(
                flipped.denominator() - flipped.numerator(),
                flipped.denominator());
        }

        // Half of bounceIn over the first half of the span.
        // Then half of bounceOut, lifted by a half, over the second.
        [[nodiscard]] Progress bounceInOut(Progress progress, Tick)
        {
            const Tick den = progress.denominator();
            const Tick doubled = mul(progress.numerator(), 2);

            if (doubled < den)
            {
                const Progress first =
                    bounceIn(Progress(doubled, den), 0);

                return Progress(
                    first.numerator(), mul(first.denominator(), 2));
            }

            const Progress second =
                bounceOut(Progress(doubled - den, den), 0);

            // Named rather than nested in the call below.
            // The order two arguments are evaluated in is unspecified.
            // Both of these can refuse.
            // Which one a caller is told of should not depend on that.
            const Tick raised =
                add(second.numerator(), second.denominator());

            return Progress(raised, mul(second.denominator(), 2));
        }

        using Shaper = Progress (*)(Progress, Tick);

        struct Shape
        {
            Shaper shaper;
            Tick power;
        };

        // Indexed by easingIndex(), so the order is the enumeration's.
        // A table rather than a switch.
        // So a new easing is a row rather than an arm one can forget.
        // And so there is no default arm no input could ever reach.
        // A bounce takes no power, which is why its rows carry zero.
        constexpr std::array<Shape, kEasingCount> kShapes{{
            {powerIn, 1},

            {powerIn, 2},
            {powerOut, 2},
            {powerInOut, 2},

            {powerIn, 3},
            {powerOut, 3},
            {powerInOut, 3},

            {powerIn, 4},
            {powerOut, 4},
            {powerInOut, 4},

            {powerIn, 5},
            {powerOut, 5},
            {powerInOut, 5},

            {bounceIn, 0},
            {bounceOut, 0},
            {bounceInOut, 0},
        }};
    } // namespace

    Progress ease(Easing easing, Progress progress)
    {
        const std::size_t index = easingIndex(easing);

        // Easing is a std::uint8_t.
        // So a caller can cast a number no enumerator has into one.
        // Refused rather than indexed with, which would read off the end.
        if (index >= kEasingCount)
        {
            throw TweenError(
                "antwika::tween: no easing has that value");
        }

        const Shape shape = kShapes[index];

        return shape.shaper(progress, shape.power);
    }

} // namespace antwika::tween
