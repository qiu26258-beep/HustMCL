add_executable(java_runtime_tests core/java_runtime_tests.cpp)
add_executable(content_tests core/content_tests.cpp)
target_link_libraries(content_tests PRIVATE hustmcl_core)
add_test(NAME core.content COMMAND content_tests)
target_link_libraries(java_runtime_tests PRIVATE hustmcl_core)
if(MSVC)
    target_compile_options(java_runtime_tests PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(java_runtime_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME core.java_runtime COMMAND java_runtime_tests)

add_executable(launch_plan_tests core/launch_plan_tests.cpp)
target_link_libraries(launch_plan_tests PRIVATE hustmcl_core)
if(MSVC)
    target_compile_options(launch_plan_tests PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(launch_plan_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME core.launch_plan COMMAND launch_plan_tests)
set_tests_properties(core.launch_plan PROPERTIES TIMEOUT 30)

add_executable(install_plan_tests core/install_plan_tests.cpp)
target_link_libraries(install_plan_tests PRIVATE hustmcl_core)
if(MSVC)
    target_compile_options(install_plan_tests PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(install_plan_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME core.install_plan COMMAND install_plan_tests)
set_tests_properties(core.install_plan PROPERTIES TIMEOUT 30)

add_test(NAME architecture.core_includes
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tools/check_core_includes.py
        ${PROJECT_SOURCE_DIR}/src/core)
add_test(NAME architecture.include_guard_regression
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/architecture/include_guard_tests.py)
add_test(NAME tools.auth_probe
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/auth_probe_tests.py)
set_tests_properties(tools.auth_probe PROPERTIES TIMEOUT 30)
add_test(NAME tools.package_setup
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/package_setup_tests.py)
set_tests_properties(tools.package_setup PROPERTIES TIMEOUT 30)
add_test(NAME architecture.forbidden_link
    COMMAND ${CMAKE_COMMAND}
        -DFIXTURE=${CMAKE_CURRENT_SOURCE_DIR}/architecture/forbidden_link
        -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/forbidden_link
        -DARCHITECTURE_MODULE=${PROJECT_SOURCE_DIR}/cmake/Architecture.cmake
        -P ${CMAKE_CURRENT_SOURCE_DIR}/architecture/verify_rejected.cmake)
set_tests_properties(core.java_runtime architecture.core_includes
    architecture.include_guard_regression architecture.forbidden_link PROPERTIES TIMEOUT 30)
add_executable(modpack_tests core/modpack_tests.cpp)
target_link_libraries(modpack_tests PRIVATE hustmcl_core)
add_test(NAME core.modpack COMMAND modpack_tests)
