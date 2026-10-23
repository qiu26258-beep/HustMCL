add_library(hustmcl_metadata STATIC
    version_metadata.cpp
    asset_index.cpp
    include/hustmcl/services/version_metadata.h
    include/hustmcl/services/asset_index.h
)
target_include_directories(hustmcl_metadata PUBLIC include)
target_link_libraries(hustmcl_metadata PUBLIC hustmcl_core Qt6::Core)
target_compile_features(hustmcl_metadata PUBLIC cxx_std_20)
set_target_properties(hustmcl_metadata PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(hustmcl_metadata PRIVATE /W4 /WX /utf-8 /permissive-)
else()
    target_compile_options(hustmcl_metadata PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(HUSTMCL_BUILD_DESKTOP OR HUSTMCL_BUILD_BACKEND)
    # The desktop executable is in a sibling directory; Qt's finalizer must
    # see the imported runtime dependency there as well.
    find_package(Qt6 6.10 REQUIRED COMPONENTS Network GLOBAL)
    add_library(hustmcl_miniz STATIC
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz.c
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz_zip.c)
    set_source_files_properties(
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz.c
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz_tinfl.c
        ${PROJECT_SOURCE_DIR}/third_party/miniz/miniz_zip.c PROPERTIES LANGUAGE CXX)
    target_include_directories(hustmcl_miniz PUBLIC ${PROJECT_SOURCE_DIR}/third_party/miniz)
    target_compile_definitions(hustmcl_miniz PUBLIC MINIZ_NO_DEFLATE_APIS MINIZ_NO_ZLIB_COMPATIBLE_NAMES)
    add_library(hustmcl_runtime STATIC
        java_probe.cpp
        launch_input_source.cpp
        install_plan_input.cpp
        local_game.cpp
        offline_session.cpp
        file_acquirer.cpp
        modrinth_content.cpp
        content_download_store.cpp
        modpack_archive.cpp
        modpack_io.cpp
        modpack_workspace.cpp
        include/hustmcl/services/java_probe.h
        include/hustmcl/services/launch_input_source.h
        include/hustmcl/services/install_plan_input.h
    )
    target_include_directories(hustmcl_runtime PUBLIC include)
    target_link_libraries(hustmcl_runtime PUBLIC hustmcl_application PRIVATE hustmcl_metadata Qt6::Core)
    target_link_libraries(hustmcl_runtime PRIVATE Qt6::Network)
    target_link_libraries(hustmcl_runtime PRIVATE hustmcl_miniz)
    target_compile_features(hustmcl_runtime PUBLIC cxx_std_20)
    set_target_properties(hustmcl_runtime PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(hustmcl_runtime PRIVATE /W4 /WX /utf-8 /permissive-)
    else()
        target_compile_options(hustmcl_runtime PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    if(BUILD_TESTING)
        add_executable(modpack_workspace_tests ${PROJECT_SOURCE_DIR}/tests/services/modpack_workspace_tests.cpp)
        target_link_libraries(modpack_workspace_tests PRIVATE hustmcl_runtime Qt6::Network)
        target_compile_definitions(modpack_workspace_tests PRIVATE HUSTMCL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/metadata")
        add_dependencies(modpack_workspace_tests java_probe_fixture)
        add_test(NAME services.modpack_workspace COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/services/modpack_workspace_tests.py $<TARGET_FILE:modpack_workspace_tests> $<TARGET_FILE:java_probe_fixture>)
        set_tests_properties(services.modpack_workspace PROPERTIES TIMEOUT 60)
        add_executable(modpack_io_tests ${PROJECT_SOURCE_DIR}/tests/services/modpack_io_tests.cpp)
        target_link_libraries(modpack_io_tests PRIVATE hustmcl_runtime Qt6::Network)
        add_test(NAME services.modpack_io COMMAND modpack_io_tests)
        set_tests_properties(services.modpack_io PROPERTIES TIMEOUT 10)
        add_executable(modpack_archive_tests ${PROJECT_SOURCE_DIR}/tests/services/modpack_archive_tests.cpp)
        target_link_libraries(modpack_archive_tests PRIVATE hustmcl_runtime)
        add_test(NAME services.modpack_archive COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tests/services/modpack_archive_tests.py $<TARGET_FILE:modpack_archive_tests>)
        set_tests_properties(services.modpack_archive PROPERTIES TIMEOUT 30)
        add_executable(content_services_tests ${PROJECT_SOURCE_DIR}/tests/services/content_services_tests.cpp)
        target_link_libraries(content_services_tests PRIVATE hustmcl_runtime Qt6::Network)
        add_test(NAME services.content_market COMMAND content_services_tests)
        set_tests_properties(services.content_market PROPERTIES TIMEOUT 30)
        add_executable(game_process_fixture ${PROJECT_SOURCE_DIR}/tests/services/game_process_fixture.cpp)
        target_link_libraries(game_process_fixture PRIVATE Qt6::Core)
        add_executable(offline_session_tests ${PROJECT_SOURCE_DIR}/tests/services/offline_session_tests.cpp)
        target_link_libraries(offline_session_tests PRIVATE hustmcl_runtime)
        add_test(NAME services.offline_session COMMAND offline_session_tests $<TARGET_FILE:game_process_fixture>)
        set_tests_properties(services.offline_session PROPERTIES TIMEOUT 15)
        add_executable(local_game_tests ${PROJECT_SOURCE_DIR}/tests/services/local_game_tests.cpp)
        target_link_libraries(local_game_tests PRIVATE hustmcl_runtime)
        target_compile_definitions(local_game_tests PRIVATE HUSTMCL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/metadata")
        add_test(NAME services.local_game COMMAND local_game_tests)
        set_tests_properties(services.local_game PROPERTIES TIMEOUT 30)
        add_executable(download_session_tests ${PROJECT_SOURCE_DIR}/tests/services/download_session_tests.cpp)
        target_link_libraries(download_session_tests PRIVATE hustmcl_runtime Qt6::Network)
        add_test(NAME services.download_session COMMAND download_session_tests)
        set_tests_properties(services.download_session PROPERTIES TIMEOUT 30)
        add_executable(file_acquisition_tests ${PROJECT_SOURCE_DIR}/tests/services/file_acquisition_tests.cpp)
        target_link_libraries(file_acquisition_tests PRIVATE hustmcl_runtime Qt6::Network advapi32)
        add_test(NAME services.file_acquisition COMMAND file_acquisition_tests)
        set_tests_properties(services.file_acquisition PROPERTIES TIMEOUT 30)
        add_executable(java_probe_fixture ${PROJECT_SOURCE_DIR}/tests/services/java_probe_fixture.cpp)
        target_compile_features(java_probe_fixture PRIVATE cxx_std_20)
        add_executable(runtime_services_tests ${PROJECT_SOURCE_DIR}/tests/services/runtime_services_tests.cpp)
        target_link_libraries(runtime_services_tests PRIVATE hustmcl_runtime)
        target_compile_definitions(runtime_services_tests PRIVATE
            HUSTMCL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/metadata")
        foreach(test_target java_probe_fixture runtime_services_tests)
            if(MSVC)
                target_compile_options(${test_target} PRIVATE /W4 /WX /utf-8 /permissive-)
            else()
                target_compile_options(${test_target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
            endif()
        endforeach()
        add_test(NAME services.runtime COMMAND runtime_services_tests $<TARGET_FILE:java_probe_fixture>)
        set_tests_properties(services.runtime PROPERTIES TIMEOUT 30)
    endif()
endif()

if(BUILD_TESTING)
    add_executable(version_metadata_tests ${PROJECT_SOURCE_DIR}/tests/services/version_metadata_tests.cpp)
    target_link_libraries(version_metadata_tests PRIVATE hustmcl_metadata)
    target_compile_definitions(version_metadata_tests PRIVATE
        HUSTMCL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/metadata")
    if(MSVC)
        target_compile_options(version_metadata_tests PRIVATE /W4 /WX /utf-8 /permissive-)
    else()
        target_compile_options(version_metadata_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME services.version_metadata COMMAND version_metadata_tests)
    set_tests_properties(services.version_metadata PROPERTIES TIMEOUT 30)

    add_executable(asset_index_tests ${PROJECT_SOURCE_DIR}/tests/services/asset_index_tests.cpp)
    target_link_libraries(asset_index_tests PRIVATE hustmcl_metadata)
    target_compile_definitions(asset_index_tests PRIVATE HUSTMCL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures/metadata")
    if(MSVC)
        target_compile_options(asset_index_tests PRIVATE /W4 /WX /utf-8 /permissive-)
    else()
        target_compile_options(asset_index_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME services.asset_index COMMAND asset_index_tests)
    set_tests_properties(services.asset_index PROPERTIES TIMEOUT 30)
endif()
