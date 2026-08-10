#pragma once

#include <cstddef>

#include "antwika/ecs/Component.hpp"

namespace antwika::ecs::detail
{

    /**
     * @brief Hands out the next unused component id.
     *
     * Ensures: every call returns a value one greater than the last,
     * counting from zero, for the lifetime of the process.
     */
    [[nodiscard]] std::size_t nextComponentId() noexcept;

    /**
     * @brief The id standing for one component type.
     *
     * Ensures: the same type answers with the same id for the lifetime
     * of the process, and two types never share one.
     */
    template <Component T>
    [[nodiscard]] std::size_t componentId() noexcept
    {
        static const std::size_t id = nextComponentId();
        return id;
    }

}
