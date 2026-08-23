add_library(antwika_widget INTERFACE)

add_library(antwika::widget ALIAS antwika_widget)

target_include_directories(antwika_widget
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_widget
    EXPORT antwika_widgetTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_widgetTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
