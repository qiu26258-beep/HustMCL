add_library(hustmcl_core STATIC
    java_runtime.cpp
    include/hustmcl/core/java_runtime.h
)
target_include_directories(hustmcl_core PUBLIC include)
target_compile_features(hustmcl_core PUBLIC cxx_std_20)
set_target_properties(hustmcl_core PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_core PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
