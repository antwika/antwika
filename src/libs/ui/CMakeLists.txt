add_library(antwika_ui
    src/Context.cpp
    src/Flatten.cpp
    src/Layout.cpp
    src/LayoutTree.cpp
    src/Painter.cpp
    src/Resolve.cpp
    src/Scope.cpp
    src/Theme.cpp
)

add_library(antwika::ui ALIAS antwika_ui)

set_target_properties(antwika_ui PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_ui
    PUBLIC
        antwika::gfx
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_ui
    EXPORT antwika_uiTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_uiTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
