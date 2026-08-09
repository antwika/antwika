#include "antwika/input/InputEventCodec.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <variant>

#include <antwika/replay/PayloadJson.hpp>

#include "antwika/input/Events.hpp"
#include "antwika/input/InputError.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{
    namespace
    {
        constexpr std::int32_t kMinCoordinate =
            std::numeric_limits<std::int32_t>::min();

        constexpr std::int32_t kMaxCoordinate =
            std::numeric_limits<std::int32_t>::max();

        void writeModifiers(
            nlohmann::json &payload, const KeyModifiers &modifiers)
        {
            payload["shift"] = modifiers.shift;
            payload["control"] = modifiers.control;
            payload["alt"] = modifiers.alt;
            payload["super"] = modifiers.super;
        }

        [[nodiscard]] KeyModifiers readModifiers(const nlohmann::json &payload)
        {
            return KeyModifiers{
                .shift = payload.at("shift").get<bool>(),
                .control = payload.at("control").get<bool>(),
                .alt = payload.at("alt").get<bool>(),
                .super = payload.at("super").get<bool>()};
        }

        void writePosition(nlohmann::json &payload, Position position)
        {
            payload["x"] = position.x;
            payload["y"] = position.y;
        }

        [[nodiscard]] Position readPosition(const nlohmann::json &payload)
        {
            return Position{
                .x = payload.at("x").get<std::int32_t>(),
                .y = payload.at("y").get<std::int32_t>()};
        }

        [[nodiscard]] nlohmann::json objectSchema(const char *title)
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = title;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            return schema;
        } // GCOVR_EXCL_LINE

        void requireBoolean(nlohmann::json &schema, const char *field)
        {
            schema["properties"][field]["type"] = "boolean";
            schema["required"].push_back(field);
        }

        void requireModifiers(nlohmann::json &schema)
        {
            for (const char *field : {"shift", "control", "alt", "super"})
            {
                requireBoolean(schema, field);
            }
        }

        void requireInteger(nlohmann::json &schema, const char *field)
        {
            schema["properties"][field]["type"] = "integer";
            schema["properties"][field]["minimum"] = kMinCoordinate;
            schema["properties"][field]["maximum"] = kMaxCoordinate;
            schema["required"].push_back(field);
        }

        void requireString(nlohmann::json &schema, const char *field)
        {
            schema["properties"][field]["type"] = "string";
            schema["required"].push_back(field);
        }

        void requirePosition(nlohmann::json &schema)
        {
            requireInteger(schema, "x");
            requireInteger(schema, "y");
        }

        [[nodiscard]] nlohmann::json keyDownSchema()
        {
            auto schema = objectSchema("input.key_down payload");
            requireString(schema, "key");
            requireModifiers(schema);
            requireBoolean(schema, "repeat");
            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json keyUpSchema()
        {
            auto schema = objectSchema("input.key_up payload");
            requireString(schema, "key");
            requireModifiers(schema);
            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json pointerMoveSchema()
        {
            auto schema = objectSchema("input.pointer_move payload");
            requirePosition(schema);
            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json pointerButtonSchema()
        {
            auto schema = objectSchema("input pointer button payload");
            requireString(schema, "button");
            requirePosition(schema);
            requireModifiers(schema);
            return schema;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json pointerScrollSchema()
        {
            auto schema = objectSchema("input.pointer_scroll payload");
            requireInteger(schema, "horizontal");
            requireInteger(schema, "vertical");
            return schema;
        } // GCOVR_EXCL_LINE

        using Validator = nlohmann::json_schema::json_validator;

        template <nlohmann::json (*Build)()>
        const Validator &validatorFor()
        {
            static const Validator validator(Build()); // GCOVR_EXCL_LINE
            return validator;
        }

        [[nodiscard]] nlohmann::json parsed(
            const Event &event, const Validator &validator)
        {
            return antwika::replay::parseAndValidatePayload<InputError>(
                event.payload,
                validator,
                "InputEventCodec: " + event.name + " payload");
        }

        template <typename Edge>
        [[nodiscard]] Edge buttonEdge(const nlohmann::json &payload)
        {
            return Edge{
                .button = mouseButtonFromString(
                    payload.at("button").get<std::string>()),
                .position = readPosition(payload),
                .modifiers = readModifiers(payload)};
        }

        struct Encoder final
        {
            [[nodiscard]] Event operator()(const KeyPressed &event) const
            {
                nlohmann::json payload;
                payload["key"] = std::string(toString(event.key));
                writeModifiers(payload, event.modifiers);
                payload["repeat"] = event.repeat;

                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kKeyDown, .payload = payload.dump()};
            }

            [[nodiscard]] Event operator()(const KeyReleased &event) const
            {
                nlohmann::json payload;
                payload["key"] = std::string(toString(event.key));
                writeModifiers(payload, event.modifiers);

                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kKeyUp, .payload = payload.dump()};
            }

            [[nodiscard]] Event operator()(const PointerMoved &event) const
            {
                nlohmann::json payload;
                writePosition(payload, event.position);

                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kPointerMove, // GCOVR_EXCL_LINE
                    .payload = payload.dump()};
            }

            [[nodiscard]] Event operator()(
                const PointerButtonPressed &event) const
            {
                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kPointerDown,
                    .payload = buttonPayload(
                        event.button, event.position, event.modifiers)};
            }

            [[nodiscard]] Event operator()(
                const PointerButtonReleased &event) const
            {
                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kPointerUp,
                    .payload = buttonPayload(
                        event.button, event.position, event.modifiers)};
            }

            [[nodiscard]] Event operator()(const PointerScrolled &event) const
            {
                nlohmann::json payload;
                payload["horizontal"] = event.horizontal;
                payload["vertical"] = event.vertical;

                return Event{ // GCOVR_EXCL_LINE
                    .name = events::kPointerScroll, // GCOVR_EXCL_LINE
                    .payload = payload.dump()};
            }

        private:
            [[nodiscard]] static std::string buttonPayload(
                MouseButton button,
                Position position,
                const KeyModifiers &modifiers)
            {
                nlohmann::json payload;
                payload["button"] = std::string(toString(button));
                writePosition(payload, position);
                writeModifiers(payload, modifiers);

                return payload.dump();
            }
        };
    }

    Event InputEventCodec::encode(const InputEvent &event) const
    {
        return std::visit(Encoder{}, event);
    }

    std::optional<InputEvent> InputEventCodec::decode(const Event &event) const
    {
        if (event.name == events::kKeyDown)
        {
            const auto payload =
                parsed(event, validatorFor<keyDownSchema>());

            return KeyPressed{
                .key = keyFromString(payload.at("key").get<std::string>()),
                .modifiers = readModifiers(payload),
                .repeat = payload.at("repeat").get<bool>()};
        }

        if (event.name == events::kKeyUp)
        {
            const auto payload =
                parsed(event, validatorFor<keyUpSchema>());

            return KeyReleased{
                .key = keyFromString(payload.at("key").get<std::string>()),
                .modifiers = readModifiers(payload)};
        }

        if (event.name == events::kPointerMove)
        {
            const auto payload =
                parsed(event, validatorFor<pointerMoveSchema>());

            return PointerMoved{.position = readPosition(payload)};
        }

        if (event.name == events::kPointerDown)
        {
            return buttonEdge<PointerButtonPressed>(
                parsed(event, validatorFor<pointerButtonSchema>()));
        }

        if (event.name == events::kPointerUp)
        {
            return buttonEdge<PointerButtonReleased>(
                parsed(event, validatorFor<pointerButtonSchema>()));
        }

        if (event.name == events::kPointerScroll)
        {
            const auto payload =
                parsed(event, validatorFor<pointerScrollSchema>());

            return PointerScrolled{ // GCOVR_EXCL_LINE
                .horizontal = payload.at("horizontal").get<std::int32_t>(),
                .vertical = payload.at("vertical").get<std::int32_t>()};
        }

        return std::nullopt;
    }

}
