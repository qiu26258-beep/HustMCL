set(HUSTMCL_BRAND_DIR "${PROJECT_SOURCE_DIR}/resources/brand")
set(HUSTMCL_WINDOWS_ICON_RC "${HUSTMCL_BRAND_DIR}/hustmcl.rc")

add_executable(hustmcl_file_probe file_probe_main.cpp)
add_executable(hustmcl_content_probe content_probe_main.cpp)
add_executable(hustmcl_modpack_probe modpack_probe_main.cpp)
target_link_libraries(hustmcl_modpack_probe PRIVATE hustmcl_runtime)
target_link_libraries(hustmcl_content_probe PRIVATE hustmcl_runtime)
target_compile_features(hustmcl_content_probe PRIVATE cxx_std_20)
add_executable(hustmcl_offline offline_main.cpp)
target_link_libraries(hustmcl_offline PRIVATE hustmcl_runtime)
if(WIN32)
    target_sources(hustmcl_offline PRIVATE "${HUSTMCL_WINDOWS_ICON_RC}")
    target_include_directories(hustmcl_offline PRIVATE "${HUSTMCL_BRAND_DIR}")
endif()
target_compile_features(hustmcl_offline PRIVATE cxx_std_20)
set_target_properties(hustmcl_offline PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_offline PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_offline PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(hustmcl_local_demo_probe local_demo_probe_main.cpp)
target_link_libraries(hustmcl_local_demo_probe PRIVATE hustmcl_runtime)
target_compile_features(hustmcl_local_demo_probe PRIVATE cxx_std_20)
set_target_properties(hustmcl_local_demo_probe PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_local_demo_probe PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_local_demo_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_executable(hustmcl_batch_probe batch_probe_main.cpp)
target_link_libraries(hustmcl_batch_probe PRIVATE hustmcl_runtime)
target_link_libraries(hustmcl_file_probe PRIVATE hustmcl_runtime)
add_executable(hustmcl_prepare_probe prepare_probe_main.cpp)
target_link_libraries(hustmcl_prepare_probe PRIVATE hustmcl_runtime)
target_compile_features(hustmcl_prepare_probe PRIVATE cxx_std_20)
set_target_properties(hustmcl_prepare_probe PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_prepare_probe PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_prepare_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(hustmcl_install_plan_probe install_plan_probe_main.cpp)
target_link_libraries(hustmcl_install_plan_probe PRIVATE hustmcl_runtime)
target_compile_features(hustmcl_install_plan_probe PRIVATE cxx_std_20)
set_target_properties(hustmcl_install_plan_probe PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_install_plan_probe PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_install_plan_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(NOT HUSTMCL_BUILD_DESKTOP)
    return()
endif()

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

# WIN32 gives the delivered launcher the Windows GUI subsystem (no console
# flashing on double-click). Qt's executable wrapper supplies the WinMain
# entry point while keeping the normal main()/stdout/stderr code below.
qt_add_executable(hustmcl_desktop WIN32 main.cpp)
target_link_libraries(hustmcl_desktop PRIVATE
    hustmcl_desktop_core
    hustmcl_runtime
    Qt6::Widgets
)
if(WIN32)
    target_sources(hustmcl_desktop PRIVATE "${HUSTMCL_WINDOWS_ICON_RC}")
    target_include_directories(hustmcl_desktop PRIVATE "${HUSTMCL_BRAND_DIR}")
endif()
target_compile_features(hustmcl_desktop PRIVATE cxx_std_20)
set_target_properties(hustmcl_desktop PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_desktop PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_desktop PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
