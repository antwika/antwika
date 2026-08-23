#include "antwika/schema/Step.hpp"

#include <string_view>
#include <utility>

namespace antwika::schema
{

    namespace
    {
        class Step final : public IMigration
        {
        public:
            Step(const std::uint32_t fromVersion,
                 const std::uint32_t toVersion,
                 std::string name,
                 Apply apply)
                : reads(fromVersion),
                  writes(toVersion),
                  stepName(std::move(name)),
                  appliesApply(std::move(apply))
            {
            }

            [[nodiscard]] std::uint32_t getFromVersion() const noexcept
                override
            {
                return reads;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return writes;
            }

            [[nodiscard]] std::string_view getName() const noexcept override
            {
                return stepName;
            }

            void apply(nlohmann::json &document) const override
            {
                appliesApply(document);
            }

        private:
            std::uint32_t reads;
            std::uint32_t writes;
            std::string stepName;
            Apply appliesApply;
        };
    }

    std::shared_ptr<const IMigration> getStep(
        const std::uint32_t fromVersion,
        const std::uint32_t toVersion,
        std::string name,
        Apply apply)
    {
        return std::make_shared<const Step>(
            fromVersion, toVersion, std::move(name), std::move(apply));
    }

}
