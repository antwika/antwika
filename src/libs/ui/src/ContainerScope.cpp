#include "antwika/ui/ContainerScope.hpp"

#include "antwika/ui/Context.hpp"

namespace antwika::ui
{

    ContainerScope::ContainerScope(Context &context) noexcept : context{context}
    {
    }

    ContainerScope::~ContainerScope()
    {
        context.closeContainer();
    }

}
