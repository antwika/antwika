add_library(antwika_input
    src/Key.cpp
    src/MouseButton.cpp
    src/NullInputBackend.cpp
)

add_library(antwika::input ALIAS antwika_input)

set_target_properties(antwika_input PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_input
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_input
    PUBLIC
        antwika::log
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_input
    EXPORT antwika_inputTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_inputTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
