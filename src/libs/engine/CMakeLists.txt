add_library(engine
    src/Engine.cpp
)

add_library(antwika::engine ALIAS engine)

set_target_properties(engine PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

set(ENGINE_PUBLIC_HEADERS
    include/antwika/engine/Engine.hpp
    include/antwika/engine/IEngine.hpp
)

set_target_properties(engine PROPERTIES
    PUBLIC_HEADER "${ENGINE_PUBLIC_HEADERS}"
)

target_include_directories(engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(engine PRIVATE antwika::time)
target_link_libraries(engine PRIVATE antwika::log)
target_link_libraries(engine PRIVATE antwika::event)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS engine
    EXPORT engineTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

install(
    EXPORT engineTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/engine
)
