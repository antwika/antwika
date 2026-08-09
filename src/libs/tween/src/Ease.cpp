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

        constexpr Tick kMaxTick = static_cast<Tick>(
            std::numeric_limits<std::int64_t>::max());

        [[noreturn]] void refuse()
        {
            throw TweenError(
                "antwika::tween: an easing's arithmetic does not fit in "
                "a tick, so its denominator is too large for the curve "
                "it was asked for");
        }

        [[nodiscard]] Tick mul(Tick left, Tick right)
        {
            if (left != 0 && right > kMaxTick / left)
            {
                refuse();
            }

            return left * right;
        }

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

        [[nodiscard]] Tick distance(Tick left, Tick right)
        {
            return left >= right ? left - right : right - left;
        }

        [[nodiscard]] Progress powerIn(Progress progress, Tick exponent)
        {
            return Progress(
                power(progress.numerator(), exponent),
                power(progress.denominator(), exponent));
        }

        [[nodiscard]] Progress powerOut(Progress progress, Tick exponent)
        {
            const Tick whole = power(progress.denominator(), exponent);
            const Tick rest = power(
                progress.denominator() - progress.numerator(), exponent);

            return Progress(whole - rest, whole);
        }

        [[nodiscard]] Progress powerInOut(Progress progress, Tick exponent)
        {
            const Tick num = progress.numerator();
            const Tick den = progress.denominator();
            const Tick whole = power(den, exponent);
            const Tick scale = power(2, exponent - 1);

            if (mul(num, 2) < den)
            {
                return Progress(
                    mul(scale, power(num, exponent)), whole);
            }

            return Progress(
                whole - mul(scale, power(den - num, exponent)), whole);
        }

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

        [[nodiscard]] Progress bounceIn(Progress progress, Tick)
        {
            const Tick den = progress.denominator();
            const Progress flipped = bounceOut(
                Progress(den - progress.numerator(), den), 0);

            return Progress(
                flipped.denominator() - flipped.numerator(),
                flipped.denominator());
        }

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

            const Tick raised =
                add(second.numerator(), second.denominator());

            return Progress(raised, mul(second.denominator(), 2));
        }

        using Shaper = Progress (*)(Progress, Tick);

        struct Shape final
        {
            Shaper shaper;
            Tick power;
        };

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
    }

    Progress ease(Easing easing, Progress progress)
    {
        const std::size_t index = easingIndex(easing);

        if (index >= kEasingCount)
        {
            throw TweenError(
                "antwika::tween: no easing has that value");
        }

        const Shape shape = kShapes[index];

        return shape.shaper(progress, shape.power);
    }

}
