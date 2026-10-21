add_library(hustmcl_ui STATIC
    campus_artwork.cpp
    campus_artwork.h
    copy.cpp
    copy.h
    elided_label.cpp
    elided_label.h
    home_page.cpp
    home_page.h
    install_page.cpp
    install_page.h
    install_stages.cpp
    install_stages.h
    leaf_motif.cpp
    leaf_motif.h
    page_header.cpp
    page_header.h
    page_scaffold.cpp
    page_scaffold.h
    primary_action_bar.cpp
    primary_action_bar.h
    sidebar.cpp
    sidebar.h
    stage_list.cpp
    stage_list.h
    stage_row.cpp
    stage_row.h
    status_row.cpp
    status_row.h
    summary_row.cpp
    summary_row.h
    theme.cpp
    theme.h
    ${PROJECT_SOURCE_DIR}/resources/resources.qrc
)
target_include_directories(hustmcl_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hustmcl_ui PUBLIC hustmcl_application Qt6::Widgets)
target_compile_features(hustmcl_ui PUBLIC cxx_std_20)
set_target_properties(hustmcl_ui PROPERTIES
    CXX_EXTENSIONS OFF
    AUTOMOC ON
    AUTORCC ON
)
if(MSVC)
    target_compile_options(hustmcl_ui PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_ui PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
