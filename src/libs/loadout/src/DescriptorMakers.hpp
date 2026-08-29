#pragma once

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <antwika/component/Inventory.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/FieldKind.hpp>
#include <antwika/loadout/FieldRow.hpp>
#include <antwika/loadout/LoadoutError.hpp>

namespace antwika::loadout
{

    template <typename Record, typename Value>
    Value heldBy(Value Record::*);

    template <auto Member>
    using Held = decltype(heldBy(Member));

    template <typename T, auto Member>
    [[nodiscard]] const Held<Member> &memberOf(
        const ComponentValue &value)
    {
        return std::get<T>(value).*Member;
    }

    template <typename T, auto Member>
    [[nodiscard]] Held<Member> &memberIn(ComponentValue &value)
    {
        return std::get<T>(value).*Member;
    }

    inline constexpr std::int64_t kLeastByte = 0;

    inline constexpr std::int64_t kMostByte = 255;

    inline constexpr std::size_t kChannelCount = 4;

    inline constexpr std::size_t kTintTextLength = 6;

    [[nodiscard]] inline std::optional<std::int64_t> wholeNumberOf(
        const std::string_view text)
    {
        std::int64_t number = 0;

        const auto *const begin = text.data();
        const auto *const end = begin + text.size();

        const auto [rest, mistake] =
            std::from_chars(begin, end, number);

        if (mistake != std::errc{} || rest != end || text.empty())
        {
            return std::nullopt;
        }

        return number;
    }

    [[nodiscard]] inline std::optional<float> fixedNumberOf(
        const std::string_view text)
    {
        float number = 0.0F;

        const auto *const begin = text.data();
        const auto *const end = begin + text.size();

        const auto [rest, mistake] =
            std::from_chars(begin, end, number);

        if (mistake != std::errc{} || rest != end || text.empty())
        {
            return std::nullopt;
        }

        return number;
    }

    [[nodiscard]] inline std::uint8_t byteOf(const nlohmann::json &json)
    {
        if (!json.is_number_integer())
        {
            throw LoadoutError(
                "antwika::loadout: a byte must be a whole number");
        }

        return static_cast<std::uint8_t>(std::clamp(
            json.get<std::int64_t>(), kLeastByte, kMostByte));
    }

    using SlotNumbers =
        std::array<std::uint8_t, component::kInventorySlots>;

    [[nodiscard]] inline std::optional<SlotNumbers> slotNumbersOf(
        std::string_view text)
    {
        SlotNumbers numbers{};

        for (std::size_t slot = 0; slot < numbers.size(); ++slot)
        {
            const auto cut = text.find(' ');

            const bool lastSlot = slot + 1 == numbers.size();

            if (lastSlot != (cut == std::string_view::npos))
            {
                return std::nullopt;
            }

            const auto number = wholeNumberOf(text.substr(0, cut));

            if (!number)
            {
                return std::nullopt;
            }

            numbers[slot] = static_cast<std::uint8_t>(
                std::clamp(*number, kLeastByte, kMostByte));

            text.remove_prefix(lastSlot ? text.size() : cut + 1);
        }

        return numbers;
    }

    template <typename T, auto Member>
    [[nodiscard]] constexpr FieldRow flagField(
        const std::string_view key)
    {
        return FieldRow{
            .key = key,
            .kind = FieldKind::Flag,
            .least = 0.0,
            .most = 1.0,
            .valueOf = [](const ComponentValue &value)
            { return nlohmann::json(memberOf<T, Member>(value)); },
            .setFrom = [](ComponentValue &value,
                          const nlohmann::json &json)
            {
                if (!json.is_boolean())
                {
                    throw LoadoutError(
                        "antwika::loadout: a flag must be a boolean");
                }

                memberIn<T, Member>(value) = json.get<bool>();
            },
            .textOf = [](const ComponentValue &value)
            {
                return std::string(
                    memberOf<T, Member>(value) ? "true" : "false");
            },
            .setFromText = [](ComponentValue &value,
                              const std::string_view text)
            {
                if (text != "true" && text != "false")
                {
                    return false;
                }

                memberIn<T, Member>(value) = text == "true";

                return true;
            }};
    }

    template <typename T, auto Member, std::int64_t Least,
              std::int64_t Most>
    [[nodiscard]] constexpr FieldRow wholeField(
        const std::string_view key)
    {
        return FieldRow{
            .key = key,
            .kind = FieldKind::Whole,
            .least = static_cast<double>(Least),
            .most = static_cast<double>(Most),
            .valueOf = [](const ComponentValue &value)
            { return nlohmann::json(memberOf<T, Member>(value)); },
            .setFrom = [](ComponentValue &value,
                          const nlohmann::json &json)
            {
                if (!json.is_number_integer())
                {
                    throw LoadoutError(
                        "antwika::loadout: a whole field must be a "
                        "whole number");
                }

                memberIn<T, Member>(value) =
                    static_cast<Held<Member>>(std::clamp(
                        json.get<std::int64_t>(), Least, Most));
            },
            .textOf = [](const ComponentValue &value)
            { return std::to_string(memberOf<T, Member>(value)); },
            .setFromText = [](ComponentValue &value,
                              const std::string_view text)
            {
                const auto number = wholeNumberOf(text);

                if (!number)
                {
                    return false;
                }

                memberIn<T, Member>(value) = static_cast<Held<Member>>(
                    std::clamp(*number, Least, Most));

                return true;
            }};
    }

    template <typename T, auto Member, double Least, double Most>
    [[nodiscard]] constexpr FieldRow fixedField(
        const std::string_view key)
    {
        return FieldRow{
            .key = key,
            .kind = FieldKind::Fixed,
            .least = Least,
            .most = Most,
            .valueOf = [](const ComponentValue &value)
            { return nlohmann::json(memberOf<T, Member>(value)); },
            .setFrom = [](ComponentValue &value,
                          const nlohmann::json &json)
            {
                if (!json.is_number())
                {
                    throw LoadoutError(
                        "antwika::loadout: a fixed field must be a "
                        "number");
                }

                memberIn<T, Member>(value) = static_cast<float>(
                    std::clamp(json.get<double>(), Least, Most));
            },
            .textOf = [](const ComponentValue &value)
            {
                return std::format(
                    "{:g}", memberOf<T, Member>(value));
            },
            .setFromText = [](ComponentValue &value,
                              const std::string_view text)
            {
                const auto number = fixedNumberOf(text);

                if (!number)
                {
                    return false;
                }

                memberIn<T, Member>(value) =
                    static_cast<float>(std::clamp(
                        static_cast<double>(*number), Least, Most));

                return true;
            }};
    }

    template <typename T, auto Member>
    [[nodiscard]] constexpr FieldRow tintField(
        const std::string_view key)
    {
        return FieldRow{
            .key = key,
            .kind = FieldKind::Tint,
            .least = static_cast<double>(kLeastByte),
            .most = static_cast<double>(kMostByte),
            .valueOf = [](const ComponentValue &value)
            {
                const auto tint = memberOf<T, Member>(value);

                return nlohmann::json::array(
                    {tint.red, tint.green, tint.blue, tint.alpha});
            },
            .setFrom = [](ComponentValue &value,
                          const nlohmann::json &json)
            {
                if (!json.is_array() || json.size() != kChannelCount)
                {
                    throw LoadoutError(
                        "antwika::loadout: a tint must be an array "
                        "of 4 channels");
                }

                memberIn<T, Member>(value) = gfx::Color{
                    .red = byteOf(json[0]),
                    .green = byteOf(json[1]),
                    .blue = byteOf(json[2]),
                    .alpha = byteOf(json[3])};
            },
            .textOf = [](const ComponentValue &value)
            {
                const auto tint = memberOf<T, Member>(value);

                return std::format(
                    "{:02X}{:02X}{:02X}",
                    tint.red,
                    tint.green,
                    tint.blue);
            },
            .setFromText = [](ComponentValue &value,
                              const std::string_view text)
            {
                if (text.size() != kTintTextLength)
                {
                    return false;
                }

                std::uint32_t number = 0;

                const auto *const begin = text.data();
                const auto *const end = begin + text.size();

                const auto [rest, mistake] =
                    std::from_chars(begin, end, number, 16);

                if (mistake != std::errc{} || rest != end)
                {
                    return false;
                }

                auto &tint = memberIn<T, Member>(value);

                tint.red = static_cast<std::uint8_t>(number >> 16U);
                tint.green = static_cast<std::uint8_t>(number >> 8U);
                tint.blue = static_cast<std::uint8_t>(number);

                return true;
            }};
    }

    template <typename T, auto Member>
    [[nodiscard]] constexpr FieldRow slotsField(
        const std::string_view key)
    {
        return FieldRow{
            .key = key,
            .kind = FieldKind::Slots,
            .least = static_cast<double>(kLeastByte),
            .most = static_cast<double>(kMostByte),
            .valueOf = [](const ComponentValue &value)
            { return nlohmann::json(memberOf<T, Member>(value)); },
            .setFrom = [](ComponentValue &value,
                          const nlohmann::json &json)
            {
                auto &slots = memberIn<T, Member>(value);

                if (!json.is_array() || json.size() != slots.size())
                {
                    throw LoadoutError(
                        "antwika::loadout: slots must be an array "
                        "of 4 items");
                }

                for (std::size_t slot = 0; slot < slots.size(); ++slot)
                {
                    slots[slot] = byteOf(json[slot]);
                }
            },
            .textOf = [](const ComponentValue &value)
            {
                std::string text;

                for (const auto number : memberOf<T, Member>(value))
                {
                    if (!text.empty())
                    {
                        text += ' ';
                    }

                    text += std::to_string(number);
                }

                return text;
            },
            .setFromText = [](ComponentValue &value,
                              const std::string_view text)
            {
                const auto numbers = slotNumbersOf(text);

                if (!numbers)
                {
                    return false;
                }

                memberIn<T, Member>(value) = *numbers;

                return true;
            }};
    }

}
