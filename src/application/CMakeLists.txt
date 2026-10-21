add_library(hustmcl_application STATIC
    frontend_gateway.cpp
    include/hustmcl/application/frontend_gateway.h
    include/hustmcl/application/frontend_snapshot.h
)
target_include_directories(hustmcl_application PUBLIC include)
target_link_libraries(hustmcl_application PUBLIC Qt6::Core PRIVATE hustmcl_core)
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
