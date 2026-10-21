add_executable(hustmcl_ui_tests test_ui.cpp)
target_link_libraries(hustmcl_ui_tests PRIVATE hustmcl_desktop_core Qt6::Test Qt6::Widgets)
target_compile_features(hustmcl_ui_tests PRIVATE cxx_std_20)
set_target_properties(hustmcl_ui_tests PROPERTIES
    CXX_EXTENSIONS OFF
    AUTOMOC ON
)
if(MSVC)
    target_compile_options(hustmcl_ui_tests PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_ui_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME desktop.ui_tests COMMAND hustmcl_ui_tests)
set_tests_properties(desktop.ui_tests PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
