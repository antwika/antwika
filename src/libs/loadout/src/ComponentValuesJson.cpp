#include "antwika/loadout/ComponentValuesJson.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <antwika/loadout/ComponentRow.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/Descriptors.hpp>
#include <antwika/loadout/FieldKind.hpp>
#include <antwika/loadout/FieldRow.hpp>
#include <antwika/loadout/Role.hpp>
#include <antwika/loadout/LoadoutError.hpp>

namespace antwika::loadout
{

    namespace
    {
        constexpr std::size_t kChannelCount = 4;

        [[nodiscard]] nlohmann::json getByteListShape()
        {
            nlohmann::json shape;

            shape["type"] = "array";
            shape["items"]["type"] = "integer";
            shape["items"]["minimum"] = 0;
            shape["items"]["maximum"] = 255;
            shape["minItems"] = kChannelCount;
            shape["maxItems"] = kChannelCount;

            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] nlohmann::json shapeOf(const FieldRow &field)
        {
            nlohmann::json shape;

            switch (field.kind)
            {
            case FieldKind::Flag:
                shape["type"] = "boolean";
                break;
            case FieldKind::Whole:
                shape["type"] = "integer";
                shape["minimum"] =
                    static_cast<std::int64_t>(field.least);
                shape["maximum"] =
                    static_cast<std::int64_t>(field.most);
                break;
            case FieldKind::Fixed:
                shape["type"] = "number";
                shape["minimum"] = field.least;
                shape["maximum"] = field.most;
                break;
            case FieldKind::Tint:
            case FieldKind::Slots:
                shape = getByteListShape();
                break;
            }

            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] const FieldRow &fieldOf(
            const ComponentRow &row, const std::string_view key)
        {
            const auto foundField = std::ranges::find(
                row.fields, key, &FieldRow::key);

            if (foundField == row.fields.end())
            {
                throw LoadoutError(
                    "antwika::loadout: \"" + std::string(row.name)
                    + "\" has no field \"" + std::string(key) + "\"");
            }

            return *foundField;
        }

        [[nodiscard]] const ComponentRow &rowOf(
            const std::string_view name)
        {
            const auto *const foundRow = getComponentRow(name);

            if (foundRow == nullptr)
            {
                throw LoadoutError(
                    "antwika::loadout: no component is named \""
                    + std::string(name) + "\"");
            }

            return *foundRow;
        }
    }

    nlohmann::json getComponentValuesShape()
    {
        nlohmann::json shape;

        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["properties"] = nlohmann::json::object();

        for (const auto &row : getComponentRows())
        {
            if (row.role != Role::Valued)
            {
                continue;
            }

            nlohmann::json rowShape;

            rowShape["type"] = "object";
            rowShape["additionalProperties"] = false;
            rowShape["properties"] = nlohmann::json::object();
            rowShape["required"] = nlohmann::json::array();

            for (const auto &field : row.fields)
            {
                const auto key = std::string(field.key);

                rowShape["properties"][key] = shapeOf(field);
                rowShape["required"].push_back(key);
            }

            shape["properties"][std::string(row.name)] = rowShape;
        }

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json getWrittenComponentValues(const ComponentValues &componentValues)
    {
        auto valuesJson = nlohmann::json::object();

        for (const auto &[name, value] : componentValues)
        {
            const auto &row = rowOf(name);

            if (row.fresh().index() != value.index())
            {
                throw LoadoutError(
                    "antwika::loadout: \"" + name
                    + "\" holds another component");
            }

            auto fieldsJson = nlohmann::json::object();

            for (const auto &field : row.fields)
            {
                fieldsJson[std::string(field.key)] =
                    field.valueOf(value);
            }

            valuesJson[name] = fieldsJson;
        }

        return valuesJson;
    } // GCOVR_EXCL_LINE

    ComponentValues getReadComponentValues(const nlohmann::json &json)
    {
        if (!json.is_object())
        {
            throw LoadoutError(
                "antwika::loadout: componentValues must be an object");
        }

        ComponentValues componentValues;

        for (const auto &[name, fieldsJson] : json.items())
        {
            const auto &row = rowOf(name);

            if (!fieldsJson.is_object())
            {
                throw LoadoutError(
                    "antwika::loadout: \"" + name
                    + "\" must hold an object of fields");
            }

            auto value = row.fresh();

            for (const auto &[key, fieldJson] : fieldsJson.items())
            {
                fieldOf(row, key).setFrom(value, fieldJson);
            }

            componentValues.insert_or_assign(name, value);
        }

        return componentValues;
    } // GCOVR_EXCL_LINE

}
