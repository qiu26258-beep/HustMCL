add_library(hustmcl_application STATIC
    frontend_gateway.cpp
    account.cpp
    authenticated_launch.cpp
    include/hustmcl/application/account.h
    content_market.cpp
    modpack_import.cpp
    include/hustmcl/application/modpack_import.h
    include/hustmcl/application/content_market.h
    launch_preparation.cpp
    file_acquisition.cpp
    batch_acquisition.cpp
    offline_launch.cpp
    offline_controller.cpp
    include/hustmcl/application/offline_controller.h
    include/hustmcl/application/batch_acquisition.h
    include/hustmcl/application/frontend_gateway.h
    include/hustmcl/application/frontend_snapshot.h
    include/hustmcl/application/launch_preparation.h
)
target_include_directories(hustmcl_application PUBLIC include)
target_link_libraries(hustmcl_application PUBLIC Qt6::Core hustmcl_core)
target_compile_features(hustmcl_application PUBLIC cxx_std_20)
set_target_properties(hustmcl_application PROPERTIES
    CXX_EXTENSIONS OFF
    AUTOMOC ON
)
if(MSVC)
    target_compile_options(hustmcl_application PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_application PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING)
    add_executable(account_tests ${PROJECT_SOURCE_DIR}/tests/application/account_tests.cpp)
    target_link_libraries(account_tests PRIVATE hustmcl_application)
    add_test(NAME application.account COMMAND account_tests)
    add_executable(modpack_import_tests ${PROJECT_SOURCE_DIR}/tests/application/modpack_import_tests.cpp)
    target_link_libraries(modpack_import_tests PRIVATE hustmcl_application)
    add_test(NAME application.modpack_import COMMAND modpack_import_tests)
    add_executable(content_market_tests ${PROJECT_SOURCE_DIR}/tests/application/content_market_tests.cpp)
    target_link_libraries(content_market_tests PRIVATE hustmcl_application)
    add_test(NAME application.content_market COMMAND content_market_tests)
    add_executable(offline_controller_tests ${PROJECT_SOURCE_DIR}/tests/application/offline_controller_tests.cpp)
    target_link_libraries(offline_controller_tests PRIVATE hustmcl_application)
    add_test(NAME application.offline_controller COMMAND offline_controller_tests)
    add_executable(offline_launch_tests ${PROJECT_SOURCE_DIR}/tests/application/offline_launch_tests.cpp)
    target_link_libraries(offline_launch_tests PRIVATE hustmcl_application)
    add_test(NAME application.offline_launch COMMAND offline_launch_tests)
    add_executable(batch_acquisition_tests ${PROJECT_SOURCE_DIR}/tests/application/batch_acquisition_tests.cpp)
    target_link_libraries(batch_acquisition_tests PRIVATE hustmcl_application)
    add_test(NAME application.batch_acquisition COMMAND batch_acquisition_tests)
    set_tests_properties(application.batch_acquisition PROPERTIES TIMEOUT 15)
    add_executable(launch_preparation_tests ${PROJECT_SOURCE_DIR}/tests/application/launch_preparation_tests.cpp)
    target_link_libraries(launch_preparation_tests PRIVATE hustmcl_application)
    if(MSVC)
        target_compile_options(launch_preparation_tests PRIVATE /W4 /WX /utf-8 /permissive-)
    else()
        target_compile_options(launch_preparation_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME application.launch_preparation COMMAND launch_preparation_tests)
    set_tests_properties(application.launch_preparation PROPERTIES TIMEOUT 15)
endif()
