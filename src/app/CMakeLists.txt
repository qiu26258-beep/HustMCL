add_library(hustmcl_desktop_core STATIC
    demo_controller.cpp
    demo_controller.h
    main_window.cpp
    main_window.h
)
target_include_directories(hustmcl_desktop_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hustmcl_desktop_core PUBLIC hustmcl_ui Qt6::Widgets)
target_compile_features(hustmcl_desktop_core PUBLIC cxx_std_20)
set_target_properties(hustmcl_desktop_core PROPERTIES
    CXX_EXTENSIONS OFF
    AUTOMOC ON
)
if(MSVC)
    target_compile_options(hustmcl_desktop_core PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_desktop_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(hustmcl_desktop main.cpp)
target_link_libraries(hustmcl_desktop PRIVATE hustmcl_desktop_core Qt6::Widgets)
target_compile_features(hustmcl_desktop PRIVATE cxx_std_20)
set_target_properties(hustmcl_desktop PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_desktop PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_desktop PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
