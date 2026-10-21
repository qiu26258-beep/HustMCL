execute_process(
    COMMAND ${CMAKE_COMMAND} -S ${FIXTURE} -B ${OUTPUT_DIR}
        -DARCHITECTURE_MODULE=${ARCHITECTURE_MODULE}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(result EQUAL 0 OR NOT "${output}${error}" MATCHES "forbidden core dependency: Qt6::Widgets")
    message(FATAL_ERROR "Expected dependency rejection; got ${result}: ${output}${error}")
endif()
message(STATUS "PASS forbidden core link was rejected")
